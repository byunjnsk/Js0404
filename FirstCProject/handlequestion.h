// handlequestion.h

#ifndef HANDLEQUESTION_H
#define HANDLEQUESTION_H

#define MAX_OPTION 4
#define MAX_LINE 512
#define MAX_DIFFICULTY 3
#define MAX_CATEGORIES 10
#define MAX_CATEGORY_NAME 10
#define MAX_EXAM_QUESTIONS 100
#define MAX_PARTICIPANTS 100


extern char categoryNames[MAX_CATEGORIES][MAX_CATEGORY_NAME];
extern int categoryCount;

// 퀴즈 구조체
typedef struct Question {
    int id;
    int category;
    char question[256];
    char options[MAX_OPTION][128];
    int answer_index;
    int score;
    struct Question* next;
} Question;

// 퀴즈 배열용 구조체
typedef struct {
    Question* head;
    int count;
} QuestionList;

//시험용 구조체
typedef struct {
    int category;
    int numLow;
    int numMid;
    int numHigh;
} ExamSetting;

typedef struct {
    char userId[32];
    Question* questions[MAX_EXAM_QUESTIONS];
    int optionShuffle[MAX_EXAM_QUESTIONS][4];  // 각 문제 문항 순서
    int total;
    int currentIndex;
} ExamSession;

typedef struct {
    char userId[32];
    int score;
    int total;
    Question* current;
    time_t startTime;
    int correctIds[100];
    int incorrectIds[100];
    int correctCount;
    int incorrectCount;
    ExamSession exam;
} SessionState;

// 전역 문제 데이터베이스
extern QuestionList questionBank[MAX_CATEGORIES][MAX_DIFFICULTY];


//문제
int getDifficultyFromScore(int score);
void addQuestionToBank(Question* q);
void loadQuestions(const char* filename);
void saveQuestions(const char* filename);
Question* getRandomQuestion(int category, int difficulty);
int isDuplicate(int id, int* selectedIds, int count);
void freeAllQuestions();

//케이스 구분
int identifyCommand(const char* buffer);

#define CMD_LOGIN 01

#define CMD_PRACTICE 10
#define CMD_NEXT_QUESTION 11
#define CMD_ANSWER 12
#define CMD_END 13

#define CMD_OPEN_ROOM 14
#define CMD_JOIN_EXAM 15
#define CMD_START_EXAM 16

#define CMD_ADD_ADMIN 20
#define CMD_DEL_ADMIN 21
#define CMD_LIST_ADMIN 22
#define CMD_SET_CONFIG 23
#define CMD_RELOAD 24

#define CMD_ADD_CATEGORY 30
#define CMD_DEL_CATEGORY 31
#define CMD_GET_CATEGORIES 32

#define CMD_ADD_QUESTION 40
#define CMD_DEL_QUESTION 41
#define CMD_LIST_QUESTIONS 42

#endif
