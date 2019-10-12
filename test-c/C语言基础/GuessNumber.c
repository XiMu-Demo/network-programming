//
//  GuessNumber.c
//  test-c
//
//  Created by sheng wang on 2019/9/12.
//  Copyright © 2019 feisu. All rights reserved.
//

#include "GuessNumber.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_NUMBER 100

int secret_number;

void init_number_generator(void);
void choose_new_secret_number(void);
void read_guesses(void);

void guess()
{
    char  command;
    
    printf("猜出秘密1到%d中间的秘密数字\n\n", MAX_NUMBER);
    init_number_generator();
    do {
        choose_new_secret_number();
        printf("你已经选择了一个秘密数字，试着猜出它");
        read_guesses();
        printf("在玩一次(Y/N) ");
        scanf(" %c", &command);
        printf("\n");
    } while (command == 'y' || command == 'Y');
}


void init_number_generator(void)
{
    srand((unsigned)time(NULL));
}

void choose_new_secret_number(void)
{
    secret_number = rand() % MAX_NUMBER + 1;
}

void read_guesses(void)
{
    int guess, num_guesses = 0;
    
    for (; ;) {
        num_guesses ++;
        printf("输入你的数字：");
        scanf("%d", &guess);
        if (guess == secret_number) {
            printf("你猜对啦，秘密数字是%d", secret_number);
            break;
        }
        else if (guess < secret_number){
            printf("你猜的数字太小了，再来一次吧");
        }
        else{
            printf("你猜的数字太大了，再来一次吧");
        }

    }
}


