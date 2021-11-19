#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if 0
int make_next(char* pattern, int next[])
{
    int patLen = strlen(pattern);   // pattern的长度
    int tailIdx = 0;                // 模式串的最后一个字符的下标
    int subPatLen = 0;              // 前后缀的长度

    for(tailIdx = 0; tailIdx < patLen - 1; tailIdx++)       // 外层循环，构建每一个模式串，注意patLen-1
    {
        next[tailIdx] = 0;  // 先初始化为0
        for(subPatLen = tailIdx; subPatLen > 0; subPatLen--)    // 内层循环，逐渐缩小前后缀长度来匹配最大的公共元素k
        {
            if(memcmp(&pattern[0], &pattern[tailIdx - (subPatLen - 1)], subPatLen) == 0)
            {   // 比较当前的前后缀是否一致，若一致则匹配到了最大的公共元素k
                next[tailIdx] = subPatLen;
                break;
            }
        }
    }

    return 0;
}

# else

int make_next(char* pattern, int next[])
{
    int patLen = strlen(pattern);

    int prefixTail = 0; // k - 前缀的尾部
    int subfixTail = 0; // q - 后缀的尾部

    next[0] = 0;
    for(subfixTail = 1; subfixTail < patLen; subfixTail++)
    {
        while(prefixTail > 0 && pattern[subfixTail] != pattern[prefixTail])
        {
            prefixTail = next[prefixTail - 1]; // 出现失配时，回退到上一个子串，k=next[k-1]
        }

        if(pattern[subfixTail] == pattern[prefixTail])
        {
            prefixTail++;   // 若匹配，则继续扩大前缀长度，k++
        }

        next[subfixTail] = prefixTail;
    }
}

#endif

int kmp(char* text, char* pattern)
{
    int next[20] = {0};     // next的实际长度要根据pattern的长度来确定
    int t = 0;
    int p = 0;
    int patLen = strlen(pattern);

    make_next(pattern, next);

    while (text[t])
    {
        if(text[t] != pattern[p])
        {
            if(p == 0)
                t++;                // 没有能匹配的模式串，t后移
            else
                p = next[p - 1];    // 获取能匹配的模式串，t不动，进行下一次匹配
        }
        else
        {    
            p++;    // 相同则向后移动
            t++;
            if(p == patLen) // 匹配完整，可返回
                return t - patLen;
        }
    }
    
    return -1;
}

/* 测试程序 */
int main()
{
    char *text = "eabcabcabcabcabcdabc";
    char *pattern = "abcabcd";
    int pos = 0;

    char *patternTest = "abcdababc";//"ababababc";
    int patLen = strlen(patternTest); 
    int i;
    int next[20] = {0};

    printf("## %s make_next result: ", patternTest);
    make_next(patternTest, next);
    for(i = 0; i < patLen - 1; i++)
    {
        printf("[%c]=%d ", (char)patternTest[i], next[i]);
    }
    printf("\n");

    pos = kmp(text, pattern);
    printf("\n## KMP result: %d\n", pos);
    printf("## %s\n## ", text);
    for(i = 0; i < pos; i++)
    {
        printf(" ");
    }
    printf("%s\n", pattern);

    return 0;
}