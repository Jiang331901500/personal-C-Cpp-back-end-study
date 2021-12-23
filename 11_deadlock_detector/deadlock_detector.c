#define _GNU_SOURCE
#include <dlfcn.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "deadlock_detector.h"

struct resource_lock;
struct vertex {
    struct vertex* next;
    pthread_t tid;        // 每个线程是一个节点 vertex，因此用线程 id 作为节点的 id
    struct resource_lock* waitting;  // waitting 不为 NULL时，指向当前正在等待释放的锁资源，通过 resource_lock 可以获取到 holder 从而构建图
    int visited;    // 遍历时用于标记是否访问过
};

struct resource_lock {
    struct resource_lock* next;
    struct vertex* holder;  // 持有当前锁的 vertex
    pthread_mutex_t* lock; // 该锁资源的 mutx 地址
    int used;       // 有多少线程当前在持有该资源或等待该资源
};

struct task_graph {
    struct vertex vertex_list;  // 辅助节点，实际第一个节点是vertex_list.next
    struct resource_lock lock_list;// 辅助节点，实际第一个节点是lock_list.next
    pthread_mutex_t mutex;
};

typedef int (*pthread_mutex_lock_ptr)(pthread_mutex_t* mutex);
pthread_mutex_lock_ptr __pthread_mutex_lock;

typedef int (*pthread_mutex_unlock_ptr)(pthread_mutex_t* mutex);
pthread_mutex_unlock_ptr __pthread_mutex_unlock;

// static resources
static struct task_graph graph;



static struct vertex* add_vertex(pthread_t tid) {
    struct vertex* v = (struct vertex*)malloc(sizeof(struct vertex));
    if(v != NULL) {
        memset(v, 0, sizeof(struct vertex));
        v->tid = tid;
    }
    return v;
}

static struct resource_lock* add_lock(pthread_mutex_t* mtx) {
    struct resource_lock* r = (struct resource_lock*)malloc(sizeof(struct resource_lock));
    if(r != NULL) {
        memset(r, 0, sizeof(struct resource_lock));
        r->lock = mtx;
    }
    return r;
}

static inline void add_relation_before_lock(struct vertex* vtx, struct resource_lock* lock) {
    vtx->waitting = lock;   // 不管是否能获取到锁，总是先记录 waitting ，避免多线程调度顺序不明确引起的错误
    lock->used++;
}

static inline void become_holder_after_lock(struct vertex* vtx, struct resource_lock* lock) {
    lock->holder = vtx;     // 成为其持有者
    vtx->waitting = NULL;
}

static inline void dereference_after_unlock(struct resource_lock* lock) {
    lock->used--;
    lock->holder = NULL;
}

static inline struct vertex* get_wait_vertex(struct vertex* vtx) {
    return vtx->waitting ? vtx->waitting->holder : NULL;
}

static struct vertex* search_vertex(struct task_graph* g, pthread_t tid) {
    struct vertex* v = g->vertex_list.next;
    while(v) {
        if(tid == v->tid) {
            return v;
        }
        v = v->next;
    }

    return NULL;
}

static struct resource_lock* search_lock(struct task_graph* g, pthread_mutex_t* mtx) {
    struct resource_lock* l = g->lock_list.next;
    while(l) {
        if(mtx == l->lock) {
            return l;
        }
        l = l->next;
    }

    return NULL;
}

static void add_vertex_to_graph(struct task_graph* g, struct vertex* vtx) {
    struct vertex* node = g->vertex_list.next;
    struct vertex* prev = &g->vertex_list;
    while(node) {
        if(node == vtx) {
            return; // 避免重复添加
        }
        node = node->next;
        prev = prev->next;
    }
    prev->next = vtx;
}

static void add_lock_to_graph(struct task_graph* g, struct resource_lock* lock) {
    struct resource_lock* node = g->lock_list.next;
    struct resource_lock* prev = &g->lock_list;
    while(node) {
        if(node == lock) {
            return; // 避免重复添加
        }
        node = node->next;
        prev = prev->next;
    }
    prev->next = lock;
}

static void before_lock(struct task_graph* g, pthread_t tid, pthread_mutex_t* mtx) {

    struct vertex* vtx;
    struct resource_lock* lock;

    __pthread_mutex_lock(&g->mutex);

    if((vtx = search_vertex(g, tid)) == NULL) { // 是否已存在该线程的 vertex
        vtx = add_vertex(tid);  // 没有则新建一个 vertex 并挂到链表中
        if(vtx) {
            add_vertex_to_graph(g, vtx);
        }
        else {
            printf("add_vertex error...\n");
            assert(0);
        }
    }

    if((lock = search_lock(g, mtx)) == NULL) { // 是否已存在该 mutex 的 resource_lock
        lock = add_lock(mtx);  // 没有则新建一个 resource_lock 并挂到链表中
        if(lock) {
            add_lock_to_graph(g, lock);
        }
        else {
            printf("add_lock error...\n");
            assert(0);
        }
    }

    add_relation_before_lock(vtx, lock);    // 建立调用关系（即图的边）

    __pthread_mutex_unlock(&g->mutex);
}

static void after_lock(struct task_graph* g, pthread_t tid, pthread_mutex_t* mtx) {

    struct vertex* vtx;
    struct resource_lock* lock;

    __pthread_mutex_lock(&g->mutex);

    vtx = search_vertex(g, tid);
    lock = search_lock(g, mtx);

    if(vtx == NULL || lock == NULL) {
        printf("ERROR: vtx == NULL || lock == NULL in after_lock...\n");
        assert(0);
    }

    become_holder_after_lock(vtx, lock);    // 正式拥有当前的锁资源

    __pthread_mutex_unlock(&g->mutex);
}

static void after_unlock(struct task_graph* g, pthread_mutex_t* mtx) {

    __pthread_mutex_lock(&g->mutex);

    struct resource_lock* lock = search_lock(g, mtx);
    if(lock == NULL){
        printf("ERROR: lock NULL in after_unlock...\n");
        assert(0);
    }

    dereference_after_unlock(lock);
    if(lock->used == 0) {   // lock 没有使用者就将其释放
        
        struct resource_lock* node = g->lock_list.next;
        struct resource_lock* prev = &g->lock_list;
        while(node != NULL) {
            if(node == lock) {
                prev->next = node->next;
                free(lock);
                break;
            }

            node = node->next;
            prev = prev->next;
        }
    }

    __pthread_mutex_unlock(&g->mutex);
}

/* lock 和 unlock 的 hook */
int pthread_mutex_lock(pthread_mutex_t* mutex) {

    pthread_t tid = pthread_self();
    before_lock(&graph, tid, mutex);
    int ret = __pthread_mutex_lock(mutex);
    after_lock(&graph, tid, mutex);
    return ret;
}

int pthread_mutex_unlock(pthread_mutex_t* mutex) {
    
    int ret = __pthread_mutex_unlock(mutex);
    after_unlock(&graph, mutex);

    return ret;
}

static void all_vertex_unvisited(struct vertex* vtx) {
    while(vtx) {
        vtx->visited = 0;
        vtx = vtx->next;
    }
}

static struct vertex* search_cross(struct vertex* vtx) {

    do
    {
        if(vtx->visited == 1){  // 重新回到了已访问过的节点，存在环
            return vtx;
        }
        vtx->visited = 1;
        vtx = get_wait_vertex(vtx);
    } while(vtx);

    return NULL;
    
}

static void print_cycle(struct vertex* cross) {
    struct vertex* vtx = cross;
    char buf[32];

    do {
        pthread_getname_np(vtx->tid, buf, 32);
        printf("%s(%lu) -------> ", buf, vtx->tid);
        vtx = get_wait_vertex(vtx);
    } while(vtx && vtx != cross);
    pthread_getname_np(vtx->tid, buf, 32);
    printf("%s(%lu)\n", buf, vtx->tid);
}

// 循环检查有向图中是否有环
static void* detector_routine(void* arg) {

    struct vertex* vtx;
    while(1) {

        __pthread_mutex_lock(&graph.mutex);

        vtx = graph.vertex_list.next;
        while(vtx) {

            all_vertex_unvisited(graph.vertex_list.next);
            struct vertex* cross = search_cross(vtx);
            if(cross) { // 存在环
                print_cycle(cross);
                break;
            }

            while(vtx && vtx->visited) {    // 一个小优化：当前已经被标记为 visited 的节点不需要重新查找
                vtx = vtx->next;
            }
        }
        
        __pthread_mutex_unlock(&graph.mutex);
        sleep(5);
    }

    return NULL;
}

// 初始化检测组件，务必最先调用
void init_detector() {

    // 找到动态库中函数的入口
    __pthread_mutex_lock = dlsym(RTLD_NEXT, "pthread_mutex_lock");
    __pthread_mutex_unlock = dlsym(RTLD_NEXT, "pthread_mutex_unlock");

    memset(&graph, 0, sizeof(struct task_graph));
    pthread_mutex_init(&graph.mutex, NULL);

    pthread_t tid;
    pthread_create(&tid, NULL, detector_routine, NULL);
    pthread_detach(tid);

}

