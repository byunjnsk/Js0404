#include <stdio.h>
#include <winsock2.h>
#include <process.h>
#include <windows.h>
#include <string.h>
#include <time.h>
#include "handlequestion.h"
#include "admin.h"

#pragma comment(lib, "ws2_32.lib")

#define PORT 33336

HANDLE hMutex;

ExamSetting currentExamSetting;
Question* problemPool[MAX_EXAM_QUESTIONS];
SOCKET examClients[MAX_PARTICIPANTS];
SOCKET adminSocket = INVALID_SOCKET;
SessionState examSessions[MAX_PARTICIPANTS];
int examClientCount = 0;
int problemPoolCount = 0;
int finishedClientCount = 0;

void saveExamResultsToFile();
SessionState* getSessionBySocket(SOCKET s);

unsigned __stdcall handleClient(void* arg) {
    SOCKET client = *(SOCKET*)arg;
    char buffer[1024];
    SessionState* session = NULL;
    for (int i = 0; i < examClientCount; i++) {
        if (examClients[i] == client) {
            session = &examSessions[i];
            break;
        }
    }
    if (!session && examClientCount < MAX_PARTICIPANTS) {
        session = &examSessions[examClientCount];
        examClients[examClientCount] = client;
        examClientCount++;
    }
    if (session) {
        memset(session, 0, sizeof(SessionState));
        session->startTime = 0;
    }

    while (1) {
        int len = recv(client, buffer, sizeof(buffer) - 1, 0);
        if (len <= 0) break;
        buffer[len] = '\0';

        WaitForSingleObject(hMutex, INFINITE);

        int cmd = identifyCommand(buffer);
        char id[32], pw[32];

        switch (cmd){
            //1. 로그인
            case CMD_LOGIN:
                sscanf(buffer + 6, "%[^|]|%s", id, pw);
                strncpy(session->userId, id, sizeof(session->userId));
                if (loginAsAdmin(id, pw)) {
                    send(client, "LOGIN_OK_ADMIN", 15, 0);
                } else {
                    send(client, "LOGIN_OK_USER", 14, 0);
                }
                break;

            //2. 문제
            case CMD_PRACTICE: {
                int cat, diff, cnt;
                sscanf(buffer + 9, "%d|%d|%d", &cat, &diff, &cnt);

                int selectedIds[cnt];  // 중복 방지용 배열
                int count = 0, attempts = 0;

                while (count < cnt && attempts++ < 1000) {
                    Question* q = getRandomQuestion(cat, diff);
                    if (!q) break;

                    if (isDuplicate(q->id, selectedIds, count)) continue;

                    selectedIds[count++] = q->id;

                    snprintf(buffer, sizeof(buffer), "%d|%s|%s|%s|%s|%s|%d|%d",
                            q->id, q->question,
                            q->options[0], q->options[1], q->options[2], q->options[3],
                            q->answer_index, q->score);
                    send(client, buffer, strlen(buffer), 0);

                    // 정답 수신
                    int len = recv(client, buffer, sizeof(buffer) - 1, 0);
                    buffer[len] = '\0';

                    int recvId, recvAnswer;
                    sscanf(buffer + 7, "%d|%d", &recvId, &recvAnswer);

                    if (recvId == q->id && recvAnswer == q->answer_index) {
                        send(client, "OK", 2, 0);
                    } else {
                        send(client, "WRONG", 5, 0);
                    }
                }

                break;
            }

            case CMD_NEXT_QUESTION: {
                SessionState* s = getSessionBySocket(client);
                if (!s || !examStarted) {
                    send(client, "END", 3, 0);
                    break;
                }

                int i = s->exam.currentIndex;
                if (i >= s->exam.total) {
                    send(client, "END", 3, 0);
                    break;
                }

                Question* q = s->exam.questions[i];
                int* shuf = s->exam.optionShuffle[i];

                // 정답 인덱스 섞인 기준으로 다시 계산
                int shuffledCorrectIndex = -1;
                for (int j = 0; j < 4; j++) {
                    if (shuf[j] == q->answer_index) {
                        shuffledCorrectIndex = j;
                        break;
                    }
                }

                // ✅ 현재 문제로 설정
                s->current = q;

                char buffer[2048];
                sprintf(buffer, "%d|%s|%s|%s|%s|%s|%d|%d",
                    q->id, q->question,
                    q->options[shuf[0]], q->options[shuf[1]], q->options[shuf[2]], q->options[shuf[3]],
                    shuffledCorrectIndex, q->score);

                send(client, buffer, strlen(buffer), 0);
                s->exam.currentIndex++;
                break;
            }

            case CMD_ANSWER: {
                int id, choice;
                sscanf(buffer + 7, "%d|%d", &id, &choice);

                SessionState* session = getSessionBySocket(client);
                if (!session) break;

                Question* q = session->current;
                if (!q) break;

                int questionIndex = session->exam.currentIndex - 1;  // 바로 이전에 출제한 문제
                int selectedOriginalIndex = session->exam.optionShuffle[questionIndex][choice];

                if (selectedOriginalIndex == q->answer_index) {
                    session->score += q->score;
                    session->correctIds[session->correctCount++] = q->id;
                    send(client, "OK! 정답입니다.", 32, 0);
                } else {
                    session->incorrectIds[session->incorrectCount++] = q->id;
                    send(client, "틀렸습니다.", 32, 0);
                }

                break;
            }

            case CMD_END: {
                finishedClientCount++;
                if (finishedClientCount >= examClientCount-1) {
                    printf("[서버] 모든 수험자가 시험을 마쳤습니다.\n");
                    saveExamResultsToFile();

                    examStarted = 0;
                    examRoomOpen = 0;
                    examClientCount = 0;
                    finishedClientCount = 0;
                }

                break;
            }


            //3. 시험
            case CMD_OPEN_ROOM: {
                adminSocket = client;  // 현재 명령 보낸 클라이언트를 관리자 소켓으로 저장
                SOCKET examClients[MAX_PARTICIPANTS];
                int cat, low, mid, high;
                sscanf(buffer + 10, "%d|%d|%d|%d", &cat, &low, &mid, &high);
                currentExamSetting.category = cat;
                currentExamSetting.numLow = low;
                currentExamSetting.numMid = mid;
                currentExamSetting.numHigh = high;

                problemPoolCount = 0;
                int selectedIds[MAX_EXAM_QUESTIONS] = {0};

                // LOW 문제 선택
                int attempt = 0;
                while (low > 0 && attempt++ < 1000) {
                    Question* q = getRandomQuestion(cat, 0);
                    if (q && !isDuplicate(q->id, selectedIds, problemPoolCount)) {
                        problemPool[problemPoolCount] = q;
                        selectedIds[problemPoolCount] = q->id;
                        problemPoolCount++;
                        low--;
                    }
                }

                // MID 문제 선택
                attempt = 0;
                while (mid > 0 && attempt++ < 1000) {
                    Question* q = getRandomQuestion(cat, 1);
                    if (q && !isDuplicate(q->id, selectedIds, problemPoolCount)) {
                        problemPool[problemPoolCount] = q;
                        selectedIds[problemPoolCount] = q->id;
                        problemPoolCount++;
                        mid--;
                    }
                }

                // HIGH 문제 선택
                attempt = 0;
                while (high > 0 && attempt++ < 1000) {
                    Question* q = getRandomQuestion(cat, 2);
                    if (q && !isDuplicate(q->id, selectedIds, problemPoolCount)) {
                        problemPool[problemPoolCount] = q;
                        selectedIds[problemPoolCount] = q->id;
                        problemPoolCount++;
                        high--;
                    }
                }

                int totalRequested = currentExamSetting.numLow + currentExamSetting.numMid + currentExamSetting.numHigh;
                if (problemPoolCount == totalRequested) {
                    examRoomOpen = 1;
                    examStarted = 0;
                    send(client, "시험방이 열렸습니다. 문제 준비 완료.", 64, 0);
                } else {
                    send(client, "문제 수 부족. 시험방 생성 실패.", 64, 0);
                }

                break;
            }
            case CMD_JOIN_EXAM: {
                char userId[32] = "";
                sscanf(buffer + 10, "%31s", userId);

                if (!examRoomOpen) {
                    send(client, "NO|시험방이 아직 열리지 않았습니다.", 64, 0);
                    break;
                }

                SessionState* target = NULL;
                for (int i = 0; i < examClientCount; i++) {
                    if (examClients[i] == client) {
                        target = &examSessions[i];
                        strncpy(target->userId, userId, 32);
                        break;
                    }
                }
                if (!target && examClientCount < MAX_PARTICIPANTS) {
                    examClients[examClientCount] = client;
                    target = &examSessions[examClientCount];
                    examClientCount++;
                }

                if (target) {
                    memset(target, 0, sizeof(SessionState));
                    target->startTime = 0;
                }

                send(client, "OK|시험방에 입장했습니다. 관리자 시작 대기 중...", 64, 0);
                break;
            }
            case CMD_START_EXAM: {
                
                examStarted = 1;

                // 모든 수험자에 대해 시험지 생성 및 송신
                for (int i = 0; i < examClientCount; i++) {
                    SessionState* s = &examSessions[i];
                    s->exam.total = problemPoolCount;
                    s->exam.currentIndex = 0;

                    // 문제 인덱스 섞기
                    int indexList[MAX_EXAM_QUESTIONS];
                    for (int k = 0; k < problemPoolCount; k++) indexList[k] = k;

                    if (randomQuestionOrder) {
                        for (int j = problemPoolCount - 1; j > 0; j--) {
                            int l = rand() % (j + 1);
                            int temp = indexList[j];
                            indexList[j] = indexList[l];
                            indexList[l] = temp;
                        }
                    }

                    // 문제 복사 및 옵션 셔플
                    for (int q = 0; q < problemPoolCount; q++) {
                        s->exam.questions[q] = problemPool[indexList[q]];
                        for (int j = 0; j < 4; j++) s->exam.optionShuffle[q][j] = j;
                        if (randomOptionOrder) {
                            for (int j = 3; j > 0; j--) {
                                int k = rand() % (j + 1);
                                int temp = s->exam.optionShuffle[q][j];
                                s->exam.optionShuffle[q][j] = s->exam.optionShuffle[q][k];
                                s->exam.optionShuffle[q][k] = temp;
                            }
                        }
                    }

                    s->startTime = time(NULL);

                    // 수험자에게 시험 시작 메시지 전송
                    send(examClients[i], "시험이 시작되었습니다. 문제를 푸세요.", 64, 0);
                }

                // 관리자에게 수험자 목록 전송
                if (adminSocket != INVALID_SOCKET) {
                    char buffer[1024] = "[현재 시험방 접속 수험자 목록]\n";
                    for (int i = 0; i < examClientCount; i++) {
                        char line[64];
                        snprintf(line, sizeof(line), "%d. %s\n", i + 1, examSessions[i].userId);
                        strcat(buffer, line);
                    }
                    send(adminSocket, buffer, strlen(buffer), 0);
                }

                break;
            }
            //4. 관리자
            case CMD_ADD_ADMIN:
                sscanf(buffer + 10, "%[^|]|%s", id, pw);
                addAdmin(id, pw);
                send(client, "관리자 추가 완료", 32, 0);
                break;
            case CMD_DEL_ADMIN:
                sscanf(buffer + 10, "%s", id);
                deleteAdmin(id);
                send(client, "관리자 삭제 완료", 32, 0);
                break;
            case CMD_LIST_ADMIN:
                char list[1024] = "관리자 목록:\n";
                for (int i = 0; i < adminCount; i++) {
                    strcat(list, adminList[i].id);
                    strcat(list, "\n");
                }
                send(client, list, strlen(list), 0);
                break;
            case CMD_SET_CONFIG:
                int q, o, m, s;
                sscanf(buffer + 11, "%d|%d|%d|%d", &q, &o, &m, &s); 
                randomQuestionOrder = q;
                randomOptionOrder = o;
                examTimeLimitSec = m * 60 + s;
                saveAdminConfig();
                send(client, "출제 경향 + 제한시간 설정 완료", 64, 0);
                break;
            case CMD_RELOAD:
                freeAllQuestions();
                loadQuestions("quiz.txt");
                send(client, "문제 재로딩 완료", 64, 0);
                break;
            
            //5. 카테고리
            case CMD_ADD_CATEGORY: {
                char name[64];
                sscanf(buffer + 13, "%s", name);

                int result = addCategory(name);
                if (result == 0)
                    send(client, "카테고리 추가 완료", 32, 0);
                else if (result == -2)
                    send(client, "이미 존재하는 카테고리입니다.", 64, 0);
                else
                    send(client, "카테고리 추가 실패", 32, 0);
                break;
            }
            case CMD_DEL_CATEGORY: {
                char name[64];
                sscanf(buffer + 13, "%s", name);

                int result = deleteCategory(name);
                if (result == 0)
                    send(client, "카테고리 삭제 완료", 32, 0);
                else
                    send(client, "해당 카테고리를 찾을 수 없습니다.", 64, 0);
                break;
            }
            case CMD_GET_CATEGORIES: {
                char response[1024] = "[카테고리 목록]\n";
                for (int i = 0; i < categoryCount; i++) {
                    char line[64];
                    snprintf(line, sizeof(line), "%d. %s\n", i, categoryNames[i]);
                    strcat(response, line);
                }
                send(client, response, strlen(response), 0);
                break;
            }
            case CMD_ADD_QUESTION: {
                Question* q = (Question*)malloc(sizeof(Question));
                if (!q) {
                    send(client, "메모리 할당 실패", 32, 0);
                    break;
                }

                // 형식: ADD_QUESTION|id|cat|문제|옵션1|옵션2|옵션3|옵션4|정답|점수
                sscanf(buffer + 13, "%d|%d|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%d|%d",
                    &q->id, &q->category, q->question,
                    q->options[0], q->options[1], q->options[2], q->options[3],
                    &q->answer_index, &q->score);

                q->next = NULL;
                addQuestionToBank(q);
                saveQuestions("quiz.txt");
                send(client, "문제 추가 완료", 64, 0);
                break;
            }
            case CMD_DEL_QUESTION: {
                int id;
                sscanf(buffer + 13, "%d", &id);

                int found = 0;
                for (int i = 0; i < MAX_CATEGORIES; i++) {
                    for (int j = 0; j < MAX_DIFFICULTY; j++) {
                        Question* prev = NULL;
                        Question* curr = questionBank[i][j].head;
                        while (curr) {
                            if (curr->id == id) {
                                if (prev) prev->next = curr->next;
                                else questionBank[i][j].head = curr->next;
                                free(curr);
                                questionBank[i][j].count--;
                                found = 1;
                                break;
                            }
                            prev = curr;
                            curr = curr->next;
                        }
                    }
                }

                if (found) {
                    saveQuestions("quiz.txt");
                    send(client, "문제 삭제 완료", 64, 0);
                } else {
                    send(client, "해당 문제 ID를 찾을 수 없습니다.", 64, 0);
                }
                break;
            }
            case CMD_LIST_QUESTIONS: {
                char response[4096] = "";
                for (int i = 0; i < MAX_CATEGORIES; i++) {
                    for (int j = 0; j < MAX_DIFFICULTY; j++) {
                        Question* q = questionBank[i][j].head;
                        while (q) {
                            char line[512];
                            snprintf(line, sizeof(line), "ID:%d [%s-%d] %s\n", q->id, categoryNames[q->category], q->score, q->question);
                            strcat(response, line);
                            q = q->next;
                        }
                    }
                }
                send(client, response, strlen(response), 0);
                break;
            }

            default:
                send(client, "알 수 없는 명령입니다.", 32, 0);
        }
        ReleaseMutex(hMutex);
    }
    closesocket(client);
    _endthreadex(0);
    return 0;
}


int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    loadQuestions("quiz.txt");
    loadAdminConfig();
    loadAdminAccounts();
    loadCategoryList("category.txt");

    WSADATA wsa;
    SOCKET serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    int addrSize = sizeof(clientAddr);

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup 실패\n");
        system("pause");
        return 1;
    }

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        printf("소켓 생성 실패: %d\n", WSAGetLastError());
        system("pause");
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("bind 실패: %d\n", WSAGetLastError());
        system("pause");
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("listen 실패: %d\n", WSAGetLastError());
        system("pause");
        return 1;
    }

    printf("서버 실행 중... 포트 %d\n", PORT);
    hMutex = CreateMutex(NULL, FALSE, NULL);

    while (1) {
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrSize);
        if (clientSocket == INVALID_SOCKET) {
            printf("accept 실패: %d\n", WSAGetLastError());
            continue;
        }

        SOCKET* pClient = malloc(sizeof(SOCKET));
        *pClient = clientSocket;
        _beginthreadex(NULL, 0, handleClient, pClient, 0, NULL);
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}

void saveExamResultsToFile() {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);

    char filename[64];
    strftime(filename, sizeof(filename), "exam_result_%Y%m%d_%H%M%S.txt", t);

    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("시험 결과 파일 생성 실패");
        return;
    }

    fprintf(file, "[시험 결과]\n");
    for (int i = 0; i < examClientCount; i++) {
        SessionState* s = &examSessions[i];
        int total = s->exam.total;
        int correct = s->correctCount;
        fprintf(file, "%d. ID: %s  점수: %d / %d\n", i + 1, s->userId, correct, total);
    }

    fclose(file);
    printf("[서버] 시험 결과를 파일로 저장했습니다: %s\n", filename);
}

SessionState* getSessionBySocket(SOCKET s) {
    for (int i = 0; i < examClientCount; i++) {
        if (examClients[i] == s) {
            return &examSessions[i];
        }
    }
    return NULL;
}
