//
//  main.m
//  test-c
//
//  Created by sheng wang on 2019/9/2.
//  Copyright © 2019 feisu. All rights reserved.
//

#import <Foundation/Foundation.h>
#include <stdio.h>
#include "GuessNumber.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <math.h>
#include <setjmp.h>
#include <signal.h>


#define PRINT_STR(S) printf("%s \n",S)

#define WARN_IF(EXP)\
do{ if (EXP)\
    fprintf(stderr, "Warning:  "#EXP" \n"); }\
while(0)

#define PIRNT_INT(X) printf(""#X" = %f \n",X)

#define GENERIC_MAX(type)\
type type##_max(type x, type y)\
{\
    return x > y ? x : y;\
}

#define ECHO(s)\
do {\
gets(s);\
puts(s);\
}while(0)

#define COMPILED_LOG printf("compiled on %s at %s \n", __DATE__, __TIME__)

#define ECHO1(S) {gets(S); puts(S);}

void test(char);
void strCopy(void);
size_t strlength(const char *s);
char *strCat1(char *src, const char* dst);
char *concat_str(const char *s1, const char *s2);
void test_tabulate(void);

typedef struct {
    int num;
    char name[100];
    int on_hand;
}Part;
void print_part (Part p);


struct dialing_code
{
    char *country;
    int code;
};

const struct dialing_code country_codes[] = {
    {"china", 86},  {"egypt", 20},
    {"briza", 1},   {"colombia", 57},
    {"germany", 251},  {"france", 33},
    {"india", 91},  {"iran", 98}
};

union {
    int i;
    float j;
}Number;

typedef enum {
    CLUBS = 20,
    DIAMONDS,
    HEARTS,
    SPAEDS
} Suit;

int compare_ints(const void *p, const void *q);
void fileIO(void);
static jmp_buf env;
void f1(void);
void f2(void);
int max_int(int n, ...);


////////////////////////////////////////////////////////////////////////////////////

void (*funP)(int);
 void (*funA)(int);
void myFun(int x);
typedef void (*funB)(int);//函数指针
typedef void funC (int);//函数类型


void myFun(int x)
{
    printf("myFun: %d\n",x);
}

static void test_fun_point()
{
    funB funb;
    funC *func; //funC是函数类型，所以func是指向此函数类型的指针

    myFun(100);
    
    //myFun与funP的类型关系类似于int 与int *的关系。
    funP=&myFun;  //将myFun函数的地址赋给funP变量
    (*funP)(200);  //通过函数指针变量来调用函数
    
    //myFun与funA的类型关系类似于int 与int 的关系。
    funA=myFun;
    funA(300);
    
    funb = funA;
    funb(500);
    
    func = funA;
    func(550);
    
    //三个貌似错乱的调用
    funP(600);
    (*funA)(700);
    (*myFun)(800);
}


int main(int argc, const char * argv[]) {
    @autoreleasepool {
        test_fun_point();

    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////



//n是表示后面参数的个数
int max_int(int n, ...)
{
    va_list ap;
    int i, current, largest;
    
    va_start(ap, n);
    largest =  va_arg(ap, int);
    
    for (i = 0; i < n; i++) {
        current = va_arg(ap, int);
        if (current > largest) {
            largest = current;
        }
    }
    va_end(ap);
    return largest;
}

int test_jmp(void)
{
    int ret;
    ret = setjmp(env);
    printf("setjmp return %d \n", ret);
    if (ret != 0) {
        printf("progrom terminates: longjmp called \n");
        return 0;
    }
    f1();
    printf("progrom terminates normally \n");
    return 0;
}

void f1(void)
{
    printf("f1 begins \n");
    f2();
    printf("f1 returns \n");
}

void f2(void)
{
    printf("f2 begins \n");
    longjmp(env, 0);
    printf("f2 returns \n");
}

void fileIO(void)
{
    char filename[L_tmpnam];
    tmpnam(filename);
    char *filename1;
    filename1 = tmpnam(NULL);
    
    FILE *fp;
    fp = fopen("/Users/shengwang/学习/test-c/test-c/File", "a+");
    fprintf(fp, "sdsdsdsdsdds \n");
    fclose(fp);
    
    FILE *fp1;
    FILE *fp2;
    fp1= fopen("/Users/shengwang/学习/test-c/test-c/File", "r");
    fp2 = fopen("/Users/shengwang/学习/test-c/test-c/File1", "a+");
    int c = 0;
    while ((c = getc(fp1)) != EOF) {
        putc(c, fp2);
        putchar(c);
    }
    fclose(fp1);
    fclose(fp2);
    
    
    FILE *fp3;
    char str[1000];
    fp3 = fopen("/Users/shengwang/学习/test-c/test-c/File", "r");
    while (fgets(str, sizeof(str), fp3)) {
        puts(str);
    }
}



int compare_ints(const void *p, const void *q)
{
    return strcmp((char *)p, (char *)p);
}

void  tabulate(double (*f)(double), double first, double last, double incr)
{
    double x;
    int i, num_intervals;
    
    num_intervals = ceil((last - first) / incr);
    for (i = 0; i < num_intervals; i++) {
        x = first + i*incr;
        printf("%10.5f %10.5f \n", x, f(x));
    }
}

void test_tabulate()
{
    double final, incrment, initial;
    
    printf("输入初始值：");
    scanf("%lf", &initial);
    
    printf("输入最终值：");
    scanf("%lf", &final);
    
    printf("输入增长值：");
    scanf("%lf", &incrment);
    
    printf("\n   x       cos(x)\n"
           "________   ___________\n");
    tabulate(cos, initial, final, incrment);
}



char *concat_str(const char *s1, const char *s2)
{
    char *result;
    
    result = malloc(strlen(s1) + strlen(s2) + 1);
    if (result) {
        strcpy(result, s1);
        strcat(result, s2);
    }
    return result;
}


void print_part (Part p)
{
    printf("%d~%s~%d \n", p.num,p.name,p.on_hand);
}

void diff_point_arr_str()
{
    char *p = "sfdfsdfdfd";
    p = "33333";
    char str[100] =  "sdfsfsssss";
    str[1] = '4';
    printf("%s \n",p);
    printf("%s \n",str);
}


char *strCat1(char *dst, const char* src)
{
    char *p = dst;
    
    while (*p) {
        p++;
    }
    while ((*p++ = *src++));
        
    return dst;
}


size_t strlength(const char *s)
{
    const char *p = s;
    
    while (*s) {
        s++;
    }
    return s - p;
}

void strCopy(void)
{
    char *str1 = "sfsfsfsdf";
    char str2[100] = "yfsfsfsdf";
//    strcpy(str2, str1);
    printf("%s \n", str1);
    printf("%d \n",strcasecmp(str1, str2));
}

int count_spaces(const char *s)
{
    int count = 0;
    for (; *s != '\0'; s++) {
        if (*s == ' ') {
            count++;
        }
    }
    return count;
}

void formatPrint()
{
    int i;
    float x;
    
    i = 40212;
    x = 212.12;
    
    printf("|%d|%5d|-%5d|%10d|\n", i, i, i, i);
    printf("|%10.3f|%10.3e|%g|\n", x,x,x);
}

void forCycle()
{
    int  n, odd, square;
    unsigned long i = 1;
    n = 10;
    odd = 3;
    
    for (square = 1; i <= n; i++) {
        printf("%lu%10lu\n", i, i * i);
    }
}

void  sacnff()
{
    while (1) {
        int i;
        float j,k;
        scanf("%f%d%f", &j, &i, &k);
        printf("%f--%d---%f", j, i, k);
    }
}

void getcharfun()
{
    int len = 0;
    printf("enter a message:");
    while (getchar() != '\n') {
        len++;
    }
    printf("your message was %d character long. \n", len);
    
}

void compare(void)
{
    int i = -10;
    unsigned int j = 10;
    if (i < j) {
        printf("j更大. \n");
    }
    else{
        printf("i更大.\n");
    }
}



void test(char cha){
    int a[10] = {1,2,3,4,4,5};
    
    int b[10] = {0};
    memcpy(b, a, sizeof(a));
    printf("%d \n", b[3]);
}






