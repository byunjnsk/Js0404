// admin.c

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "admin.h"
#include "handlequestion.h"

int examRoomOpen = 0;
int examStarted = 0;

int randomQuestionOrder = 0;
int randomOptionOrder = 0;
int examTimeLimitSec = 60;

AdminAccount adminList[MAX_ADMIN];
int adminCount = 0;


//1.시험 관리
//시험방 열기
void openExamRoom() {
    examRoomOpen = 1;
    examStarted = 0;
    printf("시험방이 열렸습니다.\n");
}
//시험 시작
void startExam() {
    if (examRoomOpen) {
        examStarted = 1;
        printf("시험이 시작되었습니다.\n");
    } else {
        printf("시험방이 열려 있지 않습니다.\n");
    }
}


//2. 관리자 관리
// 관리자 설정 파일 읽기/쓰기
void loadAdminConfig() {
    FILE* file = fopen(ADMIN_FILE, "r");
    if (!file) {
        perror("admin.txt 읽기 실패");
        return;
    }
    fscanf(file, "%d %d %d", &randomQuestionOrder, &randomOptionOrder, &examTimeLimitSec);
    fclose(file);
}
void saveAdminConfig() {
    FILE* file = fopen(ADMIN_FILE, "w");
    if (!file) {
        perror("admin.txt 쓰기 실패");
        return;
    }
    printf("[서버] admin.txt 저장: %d %d %d\n", randomQuestionOrder, randomOptionOrder, examTimeLimitSec);
    fprintf(file, "%d %d %d\n", randomQuestionOrder, randomOptionOrder, examTimeLimitSec);
    fclose(file);
}
// 관리자 계정 읽기/쓰기
void loadAdminAccounts() {
    FILE* file = fopen("admin_accounts.txt", "r");
    if (!file) {
        perror("admin_accounts.txt 읽기 실패");
        return;
    }

    adminCount = 0;
    while (fscanf(file, "%s %s", adminList[adminCount].id, adminList[adminCount].pw) == 2) {
        adminCount++;
        if (adminCount >= MAX_ADMIN) break;
    }

    fclose(file);
}
void saveAdminAccounts() {
    FILE* file = fopen("admin_accounts.txt", "w");
    if (!file) return;

    for (int i = 0; i < adminCount; i++) {
        fprintf(file, "%s %s\n", adminList[i].id, adminList[i].pw);
    }

    fclose(file);
}
// 관리자 로그인
int loginAsAdmin(const char* id, const char* pw) {
    for (int i = 0; i < adminCount; i++) {
        if (strcmp(adminList[i].id, id) == 0 && strcmp(adminList[i].pw, pw) == 0) {
            return 1; // 로그인 성공
        }
    }
    return 0; // 실패
}
// 관리자 계정 추가
void addAdmin(const char* id, const char* pw) {
    if (adminCount >= MAX_ADMIN) {
        printf("최대 관리자 수 초과\n");
        return;
    }

    for (int i = 0; i < adminCount; i++) {
        if (strcmp(adminList[i].id, id) == 0) {
            printf("이미 존재하는 ID입니다.\n");
            return;
        }
    }

    strncpy(adminList[adminCount].id, id, MAX_ID);
    strncpy(adminList[adminCount].pw, pw, MAX_PW);
    adminCount++;
    saveAdminAccounts();
    printf("관리자 추가 완료\n");
}
// 관리자 삭제
void deleteAdmin(const char* id) {
    for (int i = 0; i < adminCount; i++) {
        if (strcmp(adminList[i].id, id) == 0) {
            for (int j = i; j < adminCount - 1; j++) {
                adminList[j] = adminList[j + 1];
            }
            adminCount--;
            saveAdminAccounts();
            printf("관리자 삭제 완료\n");
            return;
        }
    }
    printf("해당 ID를 찾을 수 없습니다.\n");
}
// 관리자 목록 출력
void listAdmins() {
    printf("현재 등록된 관리자 계정:\n");
    for (int i = 0; i < adminCount; i++) {
        printf("[%d] ID: %s\n", i + 1, adminList[i].id);
    }
}


//3.카테고리 관리
void trimNewline(char* str) {
    int len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[--len] = '\0';
    }
}
// 불러오기
void loadCategoryList(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        perror("category.txt 열기 실패");
        return;
    }

    categoryCount = 0;
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        trimNewline(line);
        if (categoryCount < MAX_CATEGORIES) {
            strncpy(categoryNames[categoryCount++], line, MAX_CATEGORY_NAME);
        }
    }

    fclose(f);
}
// 이름으로 인덱스 찾기
int getCategoryIndexByName(const char* name) {
    for (int i = 0; i < categoryCount; i++) {
        if (strcmp(categoryNames[i], name) == 0) return i;
    }
    return -1;
}
// 카테고리 추가
int addCategory(const char* name) {
    if (categoryCount >= MAX_CATEGORIES) return -1;
    if (getCategoryIndexByName(name) != -1) return -2; // 중복

    strncpy(categoryNames[categoryCount++], name, MAX_CATEGORY_NAME);

    FILE* f = fopen("category.txt", "a");
    if (f) {
        fprintf(f, "%s\n", name);
        fclose(f);
    }
    return 0;
}
// 카테고리 삭제
int deleteCategory(const char* name) {
    int idx = getCategoryIndexByName(name);
    if (idx == -1) return -1;

    for (int i = idx; i < categoryCount - 1; i++) {
        strcpy(categoryNames[i], categoryNames[i + 1]);
    }
    categoryCount--;

    FILE* f = fopen("category.txt", "w");
    if (f) {
        for (int i = 0; i < categoryCount; i++) {
            fprintf(f, "%s\n", categoryNames[i]);
        }
        fclose(f);
    }
    return 0;
}
//카테고리 출력
void printCategoryList() {
    if (categoryCount == 0) {
        printf("등록된 카테고리가 없습니다.\n");
        return;
    }

    printf("카테고리 목록 (%d개):\n", categoryCount);
    for (int i = 0; i < categoryCount; i++) {
        printf("%d. %s\n", i + 1, categoryNames[i]);
    }
}