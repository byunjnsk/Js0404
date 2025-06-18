// handleqeustion.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "handlequestion.h"

QuestionList questionBank[MAX_CATEGORIES][MAX_DIFFICULTY];

char categoryNames[MAX_CATEGORIES][MAX_CATEGORY_NAME];
int categoryCount = MAX_CATEGORIES;

//문제
int getDifficultyFromScore(int score) {
    if (score < 5) return 0;      // low
    else if (score < 10) return 1; // mid
    else return 2;                // high
}
void addQuestionToBank(Question* q) {
    int category = q->category;
    int difficulty = getDifficultyFromScore(q->score);

    q->next = questionBank[category][difficulty].head;
    questionBank[category][difficulty].head = q;
    questionBank[category][difficulty].count++;
}

//문제 파일 읽기/쓰기
void loadQuestions(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("퀴즈 파일 열기 실패");
        exit(1);
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), file)) {
        Question* q = (Question*)malloc(sizeof(Question));
        if (!q) {
            perror("메모리 할당 실패");
            continue;
        }

        char* token = strtok(line, "|");
        q->id = atoi(token);

        token = strtok(NULL, "|");
        q->category = atoi(token);

        token = strtok(NULL, "|");
        strncpy(q->question, token, sizeof(q->question));

        for (int i = 0; i < MAX_OPTION; i++) {
            token = strtok(NULL, "|");
            strncpy(q->options[i], token, sizeof(q->options[i]));
        }

        token = strtok(NULL, "|");
        q->answer_index = atoi(token);

        token = strtok(NULL, "|");
        q->score = atoi(token);

        q->next = NULL;

        addQuestionToBank(q);
    }

    fclose(file);
    srand((unsigned int)time(NULL)); // 랜덤 시드 초기화
}
void saveQuestions(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("퀴즈 파일 열기 실패");
        exit(1);
    }

    for (int i = 0; i < categoryCount; i++) { // MAX_CATEGORIES까지 순회
        for (int j = 0; j < MAX_DIFFICULTY; j++) { // MAX_DIFFICULTIES까지 순회
            Question* current = questionBank[i][j].head;
            while (current != NULL) {
                // 각 질문의 데이터를 '|'로 구분하여 파일에 씁니다.
                fprintf(file, "%d|%d|%s",
                        current->id,
                        current->category,
                        current->question);

                // 옵션들을 추가합니다.
                for (int i = 0; i < MAX_OPTION; i++) {
                    fprintf(file, "|%s", current->options[i]);
                }

                // 정답 인덱스와 점수를 추가하고 마지막에 개행 문자를 넣습니다.
                fprintf(file, "|%d|%d\n",
                        current->answer_index,
                        current->score);

                current = current->next; // 다음 질문으로 이동
            }
        }
    }

    fclose(file);
}

//문제 랜덤 추출
Question* getRandomQuestion(int category, int difficulty) {
    if (category < 0 || category >= MAX_CATEGORIES || difficulty < 0 || difficulty >= MAX_DIFFICULTY)
        return NULL;

    QuestionList* list = &questionBank[category][difficulty]; 
    if (list->count == 0) return NULL;

    int index = rand() % list->count;
    Question* current = list->head;

    for (int i = 0; i < index; i++) {
        current = current->next;
    }

    return current;
}

//문제 겹치지 않기 위한 함수
int isDuplicate(int id, int* selectedIds, int count) {
    for (int i = 0; i < count; i++) {
        if (selectedIds[i] == id) return 1;
    }
    return 0;
}


//free
void freeAllQuestions() {
    for (int i = 0; i < MAX_CATEGORIES; i++) {
        for (int j = 0; j < MAX_DIFFICULTY; j++) {
            Question* q = questionBank[i][j].head;
            while (q) {
                Question* tmp = q;
                q = q->next;
                free(tmp);
            }
            questionBank[i][j].head = NULL;
            questionBank[i][j].count = 0;
        }
    }
}


int identifyCommand(const char* buffer) {
    if (strncmp(buffer, "LOGIN|", 6) == 0) return CMD_LOGIN;

    if (strncmp(buffer, "PRACTICE|", 9) == 0) return CMD_PRACTICE;
    if (strcmp(buffer, "NEXT_QUESTION") == 0) return CMD_NEXT_QUESTION;
    if (strncmp(buffer, "ANSWER|", 7) == 0) return CMD_ANSWER;
    if (strcmp(buffer, "END") == 0) return CMD_END;

    if (strncmp(buffer, "OPEN_ROOM|", 10) == 0) return CMD_OPEN_ROOM;
    if (strncmp(buffer, "JOIN_EXAM|", 10) == 0) return CMD_JOIN_EXAM;
    if (strcmp(buffer, "START_EXAM") == 0) return CMD_START_EXAM;

    if (strncmp(buffer, "ADD_ADMIN|", 10) == 0) return CMD_ADD_ADMIN;
    if (strncmp(buffer, "DEL_ADMIN|", 10) == 0) return CMD_DEL_ADMIN;
    if (strcmp(buffer, "LIST_ADMIN") == 0) return CMD_LIST_ADMIN;
    if (strncmp(buffer, "SET_CONFIG|", 11) == 0) return CMD_SET_CONFIG;
    if (strcmp(buffer, "RELOAD_QUESTIONS") == 0) return CMD_RELOAD;

    if (strncmp(buffer, "ADD_CATEGORY|", 13) == 0) return CMD_ADD_CATEGORY;
    if (strncmp(buffer, "DEL_CATEGORY|", 13) == 0) return CMD_DEL_CATEGORY;
    if (strncmp(buffer, "GET_CATEGORIES", 14) == 0) return CMD_GET_CATEGORIES;
    
    if (strncmp(buffer, "ADD_QUESTION|", 13) == 0) return CMD_ADD_QUESTION;
    if (strncmp(buffer, "DEL_QUESTION|", 13) == 0) return CMD_DEL_QUESTION;
    if (strcmp(buffer, "LIST_QUESTIONS") == 0) return CMD_LIST_QUESTIONS;
}