#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define __MALLOC_DEBUG  // 放在所有头文件之后
#include "memleak_detector.h"

int main() {

    char* s1 = (char*)malloc(10);
    char* s2 = (char*)malloc(20);

    free(s1);
    //free(s2); //it wiil leak

    return 0;
}