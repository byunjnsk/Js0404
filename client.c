#include <stdio.h>
#include <conio.h>
#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "menu.h"
#include "admin.h"
#include "handlequestion.h"

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 33336

SOCKET sock;

void flushInputBuffer();
void sendCommand(const char* cmd);
void loginMenu();
void studentMenu();
void adminMenu();
void solvePracticeQuestions();
void solveExamQuestions();

char tempId[32];

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    WSADATA wsa;
    struct sockaddr_in serverAddr;

    WSAStartup(MAKEWORD(2, 2), &wsa);
    sock = socket(AF_INET, SOCK_STREAM, 0);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serverAddr.sin_port = htons(SERVER_PORT);

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("서버 연결 실패\n");
        return 1;
    }

    printf("서버 연결 성공!\n");
    loginMenu();

    closesocket(sock);
    WSACleanup();
    return 0;
}

void flushInputBuffer() {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF);
}

void sendCommand(const char* cmd) {
    send(sock, cmd, strlen(cmd), 0);
}

void receiveResponse() {
    char buffer[4096];
    int len = recv(sock, buffer, sizeof(buffer) - 1, 0);
    buffer[len] = '\0';
    printf("%s\n", buffer);
}

void loginMenu() {
    const char* loginOpts[] = { "학생 로그인", "관리자 로그인", "종료" };
    int sel = displayMenu("로그인 메뉴", loginOpts, 3);

    if (sel == 2) exit(0);

    char id[32], pw[32], buffer[1024];
    printf("ID: "); scanf("%s", id);
    printf("PW: "); scanf("%s", pw);
    flushInputBuffer();

    sprintf(buffer, "LOGIN|%s|%s", id, pw);
    sendCommand(buffer);

    int len = recv(sock, buffer, sizeof(buffer) - 1, 0);
    buffer[len] = '\0';

    if (strcmp(buffer, "LOGIN_OK_ADMIN") == 0) {
        adminMenu();
    } else if (strcmp(buffer, "LOGIN_OK_USER") == 0) {
        studentMenu();
    } else {
        printf("로그인 실패. 다시 시도하세요.\n\n");
        system("pause");
        loginMenu();
    }
}

void studentMenu() {
    const char* menu[] = { "연습 문제 풀기", "시험방 입장 및 시작", "종료" };
    while (1) {
        int choice = displayMenu("수험자 메뉴", menu, 3);
        switch (choice) {
            case 0: solvePracticeQuestions(); break;
            case 1: solveExamQuestions(); break;
            case 2: exit(0);
        }
    }
}

void adminMenu() {
    const char* mainMenu[] = {
        "시험방 열기 및 시작",
        "관리자 관리",
        "문제/시험 관리",
        "카테고리 관리",
        "로그아웃"
    };

    while (1) {
        int choice = displayMenu("관리자 메뉴", mainMenu, 5);
        switch (choice) {
            case 0: {
                sendCommand("GET_CATEGORIES");
                receiveResponse();

                int cat, low, mid, high;
                printf("카테고리 번호 : "); scanf("%d", &cat);
                printf("Low 난이도 문제 수: "); scanf("%d", &low);
                printf("Mid 난이도 문제 수: "); scanf("%d", &mid);
                printf("High 난이도 문제 수: "); scanf("%d", &high);
                flushInputBuffer();

                char cmd[256];
                sprintf(cmd, "OPEN_ROOM|%d|%d|%d|%d", cat, low, mid, high);
                sendCommand(cmd);
                receiveResponse();

                printf("\n[안내] 수험자들이 입장할 시간을 주세요.\n");
                printf("준비가 되면 Enter 키를 눌러 시험을 시작하세요...");
                getchar();

                sendCommand("START_EXAM");
                receiveResponse();
                system("pause");
                break;
            }
            case 1: {
                const char* adminMenu[] = { "관리자 추가", "관리자 삭제", "관리자 목록", "뒤로가기" };
                int sel = displayMenu("관리자 관리 메뉴", adminMenu, 4);
                if (sel == 0) {
                    char id[32], pw[32], cmd[128];
                    printf("ID: "); scanf("%s", id);
                    printf("PW: "); scanf("%s", pw);
                    flushInputBuffer();
                    sprintf(cmd, "ADD_ADMIN|%s|%s", id, pw);
                    sendCommand(cmd);
                } else if (sel == 1) {
                    char id[32], cmd[64];
                    printf("삭제할 ID: "); scanf("%s", id);
                    flushInputBuffer();
                    sprintf(cmd, "DEL_ADMIN|%s", id);
                    sendCommand(cmd);
                } else if (sel == 2) {
                    sendCommand("LIST_ADMIN");
                }
                else{
                    break;
                }
                receiveResponse();
                printf("계속하려면 Enter 키를 누르세요...");
                getchar();
                break;
            }
            case 2: {
                const char* testMenu[] = { "문제 추가", "문제 삭제", "문제 목록", "시험 설정", "문제 저장", "뒤로가기" };
                int sel = displayMenu("문제/시험 관리 메뉴", testMenu, 6);
                if (sel == 0) {
                    int id, cat, answer, score;
                    char question[256], options[4][128], cmd[1024];
                    printf("문제 ID: "); scanf("%d", &id);

                    sendCommand("GET_CATEGORIES");
                    receiveResponse();

                    printf("카테고리 번호: "); scanf("%d", &cat);
                    flushInputBuffer();
                    printf("문제 내용: "); fgets(question, sizeof(question), stdin); strtok(question, "\n");
                    for (int i = 0; i < 4; i++) {
                        printf("옵션 %d: ", i + 1); fgets(options[i], sizeof(options[i]), stdin); strtok(options[i], "\n");
                    }
                    printf("정답 인덱스 (0~3): "); scanf("%d", &answer);
                    printf("점수: "); scanf("%d", &score);
                    flushInputBuffer();
                    sprintf(cmd, "ADD_QUESTION|%d|%d|%s|%s|%s|%s|%s|%d|%d",
                            id, cat, question,
                            options[0], options[1], options[2], options[3],
                            answer, score);
                    sendCommand(cmd);
                } else if (sel == 1) {
                    int id;
                    printf("삭제할 문제 ID: "); scanf("%d", &id);
                    flushInputBuffer();
                    char cmd[64];
                    sprintf(cmd, "DEL_QUESTION|%d", id);
                    sendCommand(cmd);
                } else if (sel == 2) {
                    sendCommand("LIST_QUESTIONS");
                    
                } else if (sel == 3) {
                    int q, o, m, s;
                    printf("문제 랜덤 출력 여부 (1/0): "); scanf("%d", &q);
                    printf("문항 랜덤 출력 여부 (1/0): "); scanf("%d", &o);
                    printf("제한시간 (분 초): "); scanf("%d %d", &m, &s);
                    flushInputBuffer();
                    char cmd[64];
                    sprintf(cmd, "SET_CONFIG|%d|%d|%d|%d", q, o, m, s);
                    sendCommand(cmd);
                } else if (sel == 4) {
                    sendCommand("RELOAD_QUESTIONS");
                }
                else{
                    break;
                }
                receiveResponse();
                printf("계속하려면 Enter 키를 누르세요...");
                getchar();
                break;
            }
            case 3: {
                const char* catMenu[] = { "카테고리 추가", "카테고리 삭제", "카테고리 목록", "뒤로가기" };
                int sel = displayMenu("카테고리 관리 메뉴", catMenu, 4);
                if (sel == 0) {
                    char name[32];
                    printf("추가할 이름: "); scanf("%s", name);
                    flushInputBuffer();
                    char cmd[64];
                    sprintf(cmd, "ADD_CATEGORY|%s", name);
                    sendCommand(cmd);
                } else if (sel == 1) {
                    char name[32];
                    printf("삭제할 이름: "); scanf("%s", name);
                    flushInputBuffer();
                    char cmd[64];
                    sprintf(cmd, "DEL_CATEGORY|%s", name);
                    sendCommand(cmd);
                } else if (sel == 2) {
                    sendCommand("GET_CATEGORIES");
                }
                else{
                    break;
                }
                receiveResponse();
                printf("계속하려면 Enter 키를 누르세요...");
                getchar();
                break;
            }
            case 4: return;
        }
    }
}


void solvePracticeQuestions() {
    char buffer[2048];
    int len, cat, diff, cnt;
    //카테고리 송수신
    sendCommand("GET_CATEGORIES");
    receiveResponse();

    printf("카테고리 번호 : "); scanf("%d", &cat);
    printf("난이도 선택 (하:0 중:1, 상:2) : "); scanf("%d", &diff);
    printf("풀고 싶은 문제 수 : "); scanf("%d", &cnt);

    sprintf(buffer, "PRACTICE|%d|%d|%d", cat, diff, cnt);
    sendCommand(buffer);

    printf("시험을 시작합니다.");
    Sleep(1000);
    system("cls");

    char question[256], options[4][128];
    int id, answerIndex, score, choice, answerCnt;

    for(int i=0;i<cnt;i++){
        len = recv(sock, buffer, sizeof(buffer) - 1, 0);
        buffer[len] = '\0';
        sscanf(buffer, "%d|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%d|%d",
            &id, question,
            options[0], options[1], options[2], options[3],
            &answerIndex, &score);

        printf("[문제 %d] id: %d\n%s\n\n", i+1, id, question);
        for(int j=0;j<MAX_OPTION;j++){
            printf("    %d) %s\n", j+1, options[j]);
        }
        printf("\n정답 입력 (1~4): ");
        scanf("%d", &choice);

        sprintf(buffer, "ANSWER|%d|%d", id, choice - 1);
        sendCommand(buffer);
        len = recv(sock, buffer, sizeof(buffer) - 1, 0);
        buffer[len] = '\0';
        if (strncmp(buffer, "OK", 2) == 0) answerCnt++;
        printf("%s\n", buffer);
        printf("계속하려면 Enter 키를 누르세요...");
        getchar();
        system("cls");
    }
    printf("시험이 종료되었습니다.\n");
    printf("맞은 문제 : %d/%d\n", answerCnt, cnt);
    printf("계속하려면 Enter 키를 누르세요...");
    getchar();
}

void solveExamQuestions() {
    char buffer[2048];
    sprintf(buffer, "JOIN_EXAMtemp|%s", tempId);
    sendCommand(buffer);
    int len = recv(sock, buffer, sizeof(buffer) - 1, 0);
    buffer[len] = '\0';

    if (strncmp(buffer, "NO|", 3) == 0) {
        printf("%s\n", buffer + 3);
        printf("계속하려면 Enter 키를 누르세요..."); getchar();
        return;
    }

    printf("[안내] 시험 시작 대기중입니다...\n");

    len = recv(sock, buffer, sizeof(buffer) - 1, 0);
    buffer[len] = '\0';
    printf("%s\n", buffer);

    int answerCnt = 0;

    while (1) {
        sendCommand("NEXT_QUESTION");

        len = recv(sock, buffer, sizeof(buffer) - 1, 0);
        buffer[len] = '\0';

        if (strcmp(buffer, "END") == 0) {
        printf("시험이 종료되었습니다.\n맞은 문제 수: %d\n", answerCnt);

        sendCommand("END");  // ✅ 서버에게 시험 종료 알림 전송

        printf("계속하려면 Enter 키를 누르세요..."); getchar();
        return;
    }

    if (strcmp(buffer, "TIMEOUT") == 0) {
        printf("시험 시간이 종료되었습니다.\n");

        sendCommand("END");  // ✅ 시간 초과 시도 종료도 서버에 알림

        printf("계속하려면 Enter 키를 누르세요..."); getchar();
        return;
    }

        char question[256], options[4][128];
        int id, answerIndex, score, choice;

        sscanf(buffer, "%d|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%d|%d",
               &id, question,
               options[0], options[1], options[2], options[3],
               &answerIndex, &score);

        printf("\n[시험 문제] ID: %d\n%s\n", id, question);
        for (int i = 0; i < 4; i++) {
            printf("    %d) %s\n", i + 1, options[i]);
        }

        printf("정답 번호 (1~4): ");
        scanf("%d", &choice);
        flushInputBuffer();

        sprintf(buffer, "ANSWER|%d|%d", id, choice - 1);
        sendCommand(buffer);

        len = recv(sock, buffer, sizeof(buffer) - 1, 0);
        buffer[len] = '\0';
        printf("%s\n", buffer);
        if (strncmp(buffer, "OK", 2) == 0) answerCnt++;

        printf("계속하려면 Enter 키를 누르세요..."); getchar();
        system("cls");
    }
}