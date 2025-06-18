// menu.c

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>
#include "menu.h"

#define UP 72
#define DOWN 80
#define ENTER 13

int displayMenu(const char *title, const char *menu[], int menuSize) {
    int choice = 0;
    int key;

    while (1) {
        system("cls");

        printf("===== %s =====\n\n", title);

        for (int i = 0; i < menuSize; i++) {
            if (i == choice) {
                printf("-> %s\n", menu[i]);
            } else {
                printf("   %s\n", menu[i]);
            }
        }

        key = getch();

        if (key == 0 || key == 224) {
            key = getch();
            switch (key) {
                case UP:
                    choice = (choice - 1 + menuSize) % menuSize;
                    break;
                case DOWN:
                    choice = (choice + 1) % menuSize;
                    break;
            }
        } else if (key == ENTER) {
            return choice;
        }
    }
}
