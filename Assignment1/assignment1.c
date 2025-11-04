#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>

typedef unsigned char BYTE;

struct chunkinfo{
    int size; //size of complete chunk inluding the chunkinfo
    int inuse; //is it free? 0 or occupied? 1
    BYTE *next, *prev; //address of next and prev chunk
};

static struct chunkinfo *heap_start = NULL;

struct chunkinfo* get_last_chunk() //you can change it when you aim for performance
{
    if(!heap_start) //I have a global void *startofheap = NULL;
        return NULL;
    struct chunkinfo* ch = heap_start;
    for (; (struct chunkinfo*)ch->next; ch = (struct chunkinfo*)ch->next);
    ch->next = (BYTE*)0;
    return ch;
}

void analyze()
{
    printf("\n--------------------------------------------------------------\n");
    if(!heap_start)
    {
        printf("no heap, ");
        printf("program break on address: %p\n",sbrk(0));
        return;
    }
    struct chunkinfo* ch = heap_start;
    for (int no=0; ch; ch = (struct chunkinfo*)ch->next,no++)
    {
        printf("%d | current addr: %p |", no, (struct chunkinfo*)ch);
        printf("size: %d | ", ch->size);
        printf("info: %d | ", ch->inuse);
        printf("next: %p | ", (struct chunkinfo*)ch->next);
        printf("prev: %p", (struct chunkinfo*)ch->prev);
        printf(" \n");
    }
    printf("program break on address: %p\n",sbrk(0));
}

void merge(struct chunkinfo *a, struct chunkinfo *b){
    a->size += b->size;
    a->next = b->next;

    if(b->next){
        ((struct chunkinfo*)b->next)->prev = (BYTE*)a;
    }
}

struct chunkinfo* best_fit(int size){
    struct chunkinfo* ch = heap_start;
    struct chunkinfo* best_fit = NULL;

    while(ch){
        if(!ch->inuse && size <= ch->size){
            if(!best_fit || ch->size < best_fit->size){
                best_fit = ch;
            }
        }
        if(!ch->next){
            break;
        }
        ch = (struct chunkinfo*)ch->next;
    }

    if(!best_fit){
        best_fit = ch;
    }

    return best_fit;
}

void split_chunk(struct chunkinfo *chunk, int aligned_size) {
    int pagesize = 4096;
    if (chunk->size >= aligned_size + pagesize) {
        struct chunkinfo *new_chunk = (struct chunkinfo*)((BYTE*)(chunk) + aligned_size);
        new_chunk->size = chunk->size - aligned_size;
        new_chunk->inuse = 0;
        new_chunk->next = chunk->next;
        new_chunk->prev = (BYTE*)chunk;

        if(chunk->next){
            ((struct chunkinfo*)chunk->next)->prev = (BYTE*)new_chunk;
        }
        chunk->next = (BYTE*)new_chunk;
        chunk->size = aligned_size;
    }
}


BYTE *mymalloc(int size){
    int pagesize = 4096;
    int total_size = sizeof(struct chunkinfo) + size;
    int aligned_size = ((total_size + pagesize - 1) / pagesize) * pagesize;
    struct chunkinfo *best = NULL;

    if(heap_start){
        best = best_fit(total_size);
        if(!best->inuse && best->size >= total_size){
            split_chunk(best, aligned_size);
            best->inuse = 1;
            return (BYTE*)(best + 1);
        }
    }

    struct chunkinfo *chunk = sbrk(aligned_size);
    chunk->size = aligned_size;
    chunk->inuse = 1;
        
    if(!heap_start){
        heap_start = chunk;
        heap_start->prev = (BYTE*)0;
        return (BYTE*)(chunk + 1);
    }
    
    best->next = (BYTE*)chunk;
    chunk->prev = (BYTE*)best;

    return (BYTE*)(chunk + 1);
}

BYTE *myfree(BYTE *addr){
    struct chunkinfo *chunk = (struct chunkinfo*)(addr - sizeof(struct chunkinfo));
    
    if (!chunk->inuse) {
        return NULL;
    }

    chunk->inuse = 0;
    
    while(chunk->next && !((struct chunkinfo*)chunk->next)->inuse){
        merge(chunk, (struct chunkinfo*)chunk->next);
    }
    while(chunk->prev && !((struct chunkinfo*)chunk->prev)->inuse){
        merge((struct chunkinfo*)chunk->prev, chunk);
        chunk = (struct chunkinfo*)chunk->prev;
    }

    if (!chunk->next) {
        if (chunk->prev){
            ((struct chunkinfo*)chunk->prev)->next = NULL;
        }
        else{
            heap_start = NULL;
        }
        sbrk(-chunk->size);
    }

    return NULL;
    
}

int main(int argc, char *argv[]) {
    /*BYTE*a[100];
    analyze();
    for(int i=0;i<100;i++)
        a[i]= mymalloc(1000);
    for(int i=0;i<90;i++)
        myfree(a[i]);
    analyze();
    myfree(a[95]);
    a[95] = mymalloc(1000);
    analyze();//25% points, this new chunk should fill the smaller free one
    //(best fit)
    for(int i=90;i<100;i++)
        myfree(a[i]);
    analyze();// 25% should be an empty heap now with the start address
    //from the program start
    */

    // BYTE*b[100];
    // clock_t ca, cb;
    // ca = clock();
    // for(int i=0;i<100;i++)
    //     b[i]= mymalloc(1000);
    // for(int i=0;i<90;i++)
    //     myfree(b[i]);
    //         myfree(b[95]);
    // b[95] = mymalloc(1000);
    // for(int i=90;i<100;i++)
    //     myfree(b[i]);
    // cb = clock();
    // printf("\nduration: % f\n", (double)(cb - ca));
    // return 0;

    int possibe_sizes[] = { 1000, 6000, 20000, 30000 };
    srand(time(0));
    BYTE* a[100];
    clock_t ca, cb;
    ca = clock();
    for (int i = 0; i < 100; i++)
    {
        a[i] = mymalloc(possibe_sizes[rand() % 4]);
    }
    int count = 0;
    for (int i = 0; i < 99; i++)
    {
        if (rand() % 2 == 0)
        {
            myfree(a[i]);
            a[i] = 0;
            count++;
        }
        if (count >= 50)
            break;
    }
    for (int i = 0; i < 100; i++)
    {
        if (a[i] == 0)
        {
            a[i] = mymalloc(possibe_sizes[rand() % 4]);
        }
    }
    for (int i = 0; i < 100; i++)
    {
        myfree(a[i]);
    }
    cb = clock();
    analyze();
    printf("duration: %f\n", (double)(cb - ca) / CLOCKS_PER_SEC);
}