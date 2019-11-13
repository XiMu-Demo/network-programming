//
//  FileIO.c
//  test-c
//
//  Created by sheng wang on 2019/9/23.
//  Copyright © 2019 feisu. All rights reserved.
//

#include <stdio.h>

int main(int argc, const char * argv[]) {
    FILE *fp;
    printf("sdsdsdsdsdsd \n");
    printf("参数个数: %d \n", argc);
    for (int i = 0; i < argc; i++) {
        printf("参数：%s \n",argv[i]);
    }
    if (argc != 2) {
        printf("usage:canopen filename \n");
        return 2;
    }
    
    if ((fp = fopen(argv[1], "r")) == NULL) {
        printf("%s can't be open \n",argv[1]);
    }
    
    printf("%s can be opend \n",argv[1]);
    fclose(fp);
    return 0;
}


/*
 gcc -Wall -g -o fileIO  /Users/shengwang/学习/test-c/test-c/FileIO.c
 ./fileIO 11 11的副本
 */
