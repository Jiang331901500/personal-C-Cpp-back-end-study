#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void* __malloc(size_t size, const char* file, int line) {
    void* p = malloc(size);
    if(p) { // 首先确保成功申请到了内存
        char buf[256] = {0};
        char filename[64] = {0};
        snprintf(filename, 63, "mem/%p.mem", p);
        int len = snprintf(buf, 255, "[%s, line %d] size=%lu\n", file, line, size);
        int fd = open(filename, O_RDWR | O_CREAT, S_IRUSR);
        if(fd < 0) {
            printf("open %s failed in __malloc.\n", filename);
        }
        else {
            write(fd, buf, len);
            close(fd);
        }
    }

    return p;
}

void __free(void* ptr, const char* file, int line) {
    char filename[64] = {0};
    snprintf(filename, 63, "mem/%p.mem", ptr);
    if(unlink(filename) < 0) {  // 使用 unlink 来删除文件
        printf("[%s, line %d] %p double free!\n", file, line, ptr);
    }

    free(ptr);
}


