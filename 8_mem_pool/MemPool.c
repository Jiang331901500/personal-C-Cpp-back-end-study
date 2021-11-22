#include "MemPool.h"
#include <stdio.h>
#include <errno.h>

static inline void init_a_new_block(MP_POOL* pool, MP_BLOCK* newblock)
{
    newblock->next = NULL;
    newblock->ref_counter = 0;
    newblock->failed_time = 0;
    newblock->start_of_rest = (ADDR)newblock + sizeof(MP_BLOCK);
    newblock->end_of_block = newblock->start_of_rest + pool->block_size;
}

/* 计算一下当前 block的剩余空间 */
static inline size_t rest_block_space(MP_BLOCK* block)
{
    return block->end_of_block - block->start_of_rest;
}

static MP_BLOCK* malloc_a_block(MP_POOL* pool)
{
    MP_BLOCK* newblock;
    int ret = posix_memalign((void**)&newblock, MP_MEM_ALIGN, pool->block_size + sizeof(MP_BLOCK));
    if(ret)
    {
        log("[%d]posix_memalign error[%d].\n", __LINE__, errno);
        return NULL;
    }
    init_a_new_block(pool, newblock);
 
    MP_BLOCK* block = pool->current_block;  // 从 current_block 开始找链表尾部就行
    while(block->next) block = block->next;
    block->next = newblock;     // 插入到链表末尾
    
    return newblock;
}

static ADDR malloc_a_piece(MP_POOL* pool, size_t size)
{
    // 先尝试找到一个剩余空间足够的 block
    MP_BLOCK* block = pool->current_block;
    size_t real_piece_size = size + sizeof(MP_PIECE);
    while(block)
    {
        if(rest_block_space(block) >= real_piece_size)
        {   // 找到了
            MP_PIECE* piece = (MP_PIECE*)block->start_of_rest;
            block->start_of_rest += real_piece_size;
            block->ref_counter++;
            piece->block = block;
            return (ADDR)piece->data;
        }

        // 对于申请失败的 block，增加其失败次数
        block->failed_time++;
        if(block->failed_time >= MP_MAX_BLOCK_FAIL_TIME)
            pool->current_block = block->next;    // 如果当前这个 block的失败次数太多了，就放弃这个 block，下一次从其后面开始遍历
        
        block = block->next;
    }
    // 没找到，那就新建一个 block
    block = malloc_a_block(pool);
    if(block)
    {
        MP_PIECE* piece = (MP_PIECE*)block->start_of_rest;
        block->start_of_rest += real_piece_size;
        block->ref_counter++;
        piece->block = block;
        return (ADDR)piece->data;
    }
    else 
        return NULL;
}

static ADDR malloc_a_bucket(MP_POOL* pool, size_t size)
{
    // 在 block中先给 bucket 描述符找一块可用空间
    MP_BUCKET* bucket = pool->first_bucket; 
    MP_BUCKET* prev_bucket = bucket;
    while(bucket) 
    {    
        prev_bucket = bucket;
        if(bucket->start_of_bucket == NULL)
            break;  // bucket 描述符是可以复用的，因此找到一个已经被释放掉的 bucket就可以直接用它的描述符
        bucket = bucket->next;
    }
    if(bucket == NULL)
    {   // 如果所有 bucket 描述符当前都在使用，那就再从 block中申请一个新的 bucket 描述符
        bucket = (MP_BUCKET*)malloc_a_piece(pool, sizeof(MP_BUCKET));  
        if(bucket == NULL)
            return NULL;
        // 该 bucket所在的 block引用由 malloc_a_piece增加，此处不需要处理
        bucket->still_in_use = 0;
        if(prev_bucket) // 将新 bucket插到链表末尾
            prev_bucket->next = bucket;
        else            // prev_bucket 为 NULL说明这是链表的第一个节点
            pool->first_bucket = bucket;
    }
    else
    {   // 如果是复用之前的 bucket 描述符，则将其所在的 block 引用增加
        MP_PIECE *piece = (MP_PIECE *)((ADDR)bucket - sizeof(MP_PIECE));
        piece->block->ref_counter++;
    }

    int ret = posix_memalign((void**)&bucket->start_of_bucket, MP_MEM_ALIGN, size);
    if(ret)
    {
        MP_PIECE *piece = (MP_PIECE *)((ADDR)bucket - sizeof(MP_PIECE));
        piece->block->ref_counter--;   // 出错则不能增加 block引用
        log("[%d]posix_memalign error[%d].\n", __LINE__, errno);
        return NULL;
    }

    bucket->still_in_use = 1;
    return bucket->start_of_bucket;
}

static MP_BUCKET* find_bucket_with_addr(MP_POOL* pool, ADDR addr)
{
    MP_BUCKET* bucket = pool->first_bucket;
    while(bucket)
    {
        if(bucket->start_of_bucket == addr)
            return bucket;
        bucket = bucket->next;
    }
    return NULL;
}

static MP_BLOCK* find_block_with_addr(MP_POOL* pool, ADDR addr)
{
    MP_BLOCK* block = pool->first_block;
    while(block)
    {
        if( ((ADDR)block + sizeof(MP_BLOCK)) <= addr && addr < (ADDR)block->start_of_rest )
            return block;
        block = block->next;
    }

    return NULL;
}

static void clear_block(MP_POOL* pool, MP_BLOCK* block)
{
    if(block->ref_counter > 0)
        return;

    // 清空 block前先确保其中的 bucket内存都释放了
    MP_BUCKET* bucket = pool->first_bucket;
    MP_BUCKET* prev_bucket = bucket;
    while(bucket)
    {
        MP_PIECE *piece = (MP_PIECE *)((ADDR)bucket - sizeof(MP_PIECE));
        if(piece->block == block)   // 找出属于这个 block的 bucket确保其独立内存释放掉
        {
            if(bucket->start_of_bucket)
            {
                free(bucket->start_of_bucket);
                bucket->start_of_bucket = NULL;
            }
            // 删除 bucket 要注意相应的调整 bucket 链表
            if(bucket == pool->first_bucket)
            {
                pool->first_bucket = bucket->next;
                prev_bucket = pool->first_bucket;
            }
            else
            {
                prev_bucket->next = bucket->next;
            }
        }
        else
        {
            prev_bucket = bucket;
        }
        bucket = bucket->next;
    }

    // 恢复 block 参数
    block->ref_counter = 0;
    block->failed_time = 0;
    block->start_of_rest = (ADDR)block + sizeof(MP_BLOCK);
    pool->current_block = pool->first_block;    // 一定要将 current_block 也复位，否则可能导致后面一直不会用到这个 block
}

MP_POOL* mp_create_pool(size_t size, int auto_clear)
{
    if(size < MP_MIN_BLK_SZIE) return NULL;

    MP_POOL* pool;
    size_t block_size = size < MP_PAGE_SIZE ? size : MP_PAGE_SIZE;
    size_t real_size = sizeof(MP_POOL) + sizeof(MP_BLOCK) + block_size;
    int ret = posix_memalign((void**)&pool, MP_MEM_ALIGN, real_size);
    if(ret)
    {
        log("[%d]posix_memalign error[%d].\n", __LINE__, errno);
        return NULL;
    }

    pool->block_size = block_size;
    pool->auto_clear = auto_clear;
    pool->first_bucket = NULL;
    pool->current_block = pool->first_block;  // first_block是柔性数组，不需要赋值，实际已经指向正确的位置

    init_a_new_block(pool, pool->first_block);
    return pool;
}

void mp_destroy_pool(MP_POOL* pool)
{
    mp_reset_pool(pool);
    MP_BLOCK* block = pool->first_block->next;  // 从第二个 block 开始释放内存！ 第一个比较特殊
    MP_BLOCK* prev_block = block;
    while (block)
    {
        prev_block = block;
        block = block->next;
        free(prev_block);
    }
    free(pool); // 释放池的描述符空间以及第一个初始 block
}

void* mp_malloc(MP_POOL* pool, size_t size)
{
    if(size <= 0 || pool == NULL) return NULL;

    if(size <= pool->block_size)    // block足矣，不需要使用 bucket
    {
        return malloc_a_piece(pool, size);
    }
    else // 需要使用 bucket
    {
        return malloc_a_bucket(pool, size);
    }
}

void mp_free(MP_POOL* pool, void* addr)
{
    if(pool == NULL || addr == NULL)
        return ;

    MP_BLOCK* block = NULL;
    // 先看一下这个地址是不是一个 bucket
    MP_BUCKET* bucket = find_bucket_with_addr(pool, addr);
    if(bucket)  // 这个地址是 bucket
    {
        #ifdef MP_ALLWAYS_FREE_BUCKET   // 每次清理 bucket时都将其独立内存空间释放，这里可能可以做优化
        free(bucket->start_of_bucket);
        bucket->start_of_bucket = NULL;
        #endif
        bucket->still_in_use = 0;
        MP_PIECE *piece = (MP_PIECE *)((ADDR)bucket - sizeof(MP_PIECE));
        block = piece->block;
    }
    else    // 如果这个地址不是 bucket， 则什么都不需要处理, 找出它所在的 block就行了
    {
        MP_PIECE *piece = (MP_PIECE *)(addr - sizeof(MP_PIECE));
        block = piece->block;
    }

    if(block)
        block->ref_counter--;   // 减少 block 引用计数
    else
        return ; // 这个地址没有对应的内存块

    // 如果启用了自动清理内存池，则在适当的条件下执行清理操作
    if(pool->auto_clear)
        clear_block(pool, block);
}

/* 用户主动发起清理操作 */
void mp_reset_pool(MP_POOL* pool)
{
    if(pool == NULL)
        return;
    
    // 先确保释放掉所有 bucket 的空间
    MP_BUCKET* bucket = pool->first_bucket;
    while(bucket)
    {
        if(bucket->start_of_bucket)
        {
            free(bucket->start_of_bucket);
            bucket->start_of_bucket = NULL;
        }
        
        bucket = bucket->next;
    }
    pool->first_bucket = NULL;

    MP_BLOCK* block = pool->first_block;
    while(block)
    {
        block->ref_counter = 0;
        block->failed_time = 0;
        block->start_of_rest = (ADDR)block + sizeof(MP_BLOCK);
        block = block->next;
    }

    pool->current_block = pool->first_block;
}

/* 输出内存池统计信息 */
void mp_pool_statistic(MP_POOL* pool)
{
    if(pool == NULL) return;

    int bnum = 0;
    int currnum = 0;
    MP_BLOCK* block = pool->first_block;
    while(block)
    {
        bnum++;
        if(block == pool->current_block)
            currnum = bnum;
        block = block->next;
    }

    printf("# block size: %lu\n", pool->block_size);
    printf("# block(s) num: %d\n", bnum);
    printf("# block current: %d\n", currnum);

    block = pool->first_block;
    bnum = 0;
    while(block)
    {
        bnum++;
        printf("\n#### block %d", bnum);
        if(block == pool->current_block)
            printf(" *\n");
        else
            printf("\n");
        printf("space used: %ld\n", block->start_of_rest - ((ADDR)block + sizeof(MP_BLOCK)));
        printf("space free: %lu\n", rest_block_space(block));

        int bucket_in_this_block = 0;
        int bucket_in_this_block_in_use = 0;
        MP_BUCKET* bucket = pool->first_bucket;
        while(bucket)
        {
            MP_PIECE *piece = (MP_PIECE *)((ADDR)bucket - sizeof(MP_PIECE));
            if(piece->block == block)
            {
                bucket_in_this_block++;
                if(bucket->still_in_use)
                    bucket_in_this_block_in_use++;
            }
            bucket = bucket->next;
        }
        printf("buckets in the block: %d\n", bucket_in_this_block);
        printf("buckets in use: %d\n", bucket_in_this_block_in_use);
        printf("reference remain: %d\n", block->ref_counter);
        printf("failed time: %d\n", block->failed_time);

        block = block->next;
    }
}

