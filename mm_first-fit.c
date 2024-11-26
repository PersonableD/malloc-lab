/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "seulFighting",
    /* First member's full name */
    "seula Kim",
    /* First member's email address */
    "urghb12375@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

static char *heap_listp;//항상 프롤로그 블록을 가리키는 포인터
// 정렬 기준은 8바이트
/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
//8단위로 할당하겠다라는 말 8바이트 까지는 8에 꽉채우는데 9바이트 이상 할당해야할때는 16바이트 할당이 필요함
/* rounds up to the nearest multiple of ALIGNMENT */
// #define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
// #define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
///////////////////////////////////////////////////////
#define WSIZE 4 //워드크기
#define DSIZE 8 //더블워드크기
#define CHUNKSIZE (1<<12) // 4096바이트

#define MAX(x,y) ((x)>(y)? (x):(y))

#define PACK(size,malloc) ((size) | (malloc)) // 비트연산자 "or" 수행

#define GET(p) (*(unsigned int *)(p)) //주소 p에 있는 unsigned int 크기의 워드를 읽어서 반환
#define PUT(p,val) (*(unsigned int*)(p)=(val))//주소 p에 unsigned int 크기의 값 val을 저장

#define GET_SIZE(p) (GET(p) & ~0x7)//주소 p 에 있는 헤더 또는 푸터의 size를 리턴
#define GET_ALLOC(p) (GET(p) & 0x1)//주소 p 에 있는 헤더 또는 푸터의 할당 비트를 리턴 1 or 0

#define HDRP(bp) ((char *)(bp) - WSIZE) // 헤더의 시작주소 리턴
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp))-DSIZE)// bp주소+ 헤더 블록크기 -8 해서 푸터의 시작주소 리턴

#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE(((char*)(bp)-WSIZE)))//다음 블록의 시작 주소
#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE(((char*)(bp)-DSIZE)))//이전블록의 시작 주소

//collapse(붕괴하다) 와 coalesce(병합하다) 는 다른뜻!
static void *coalesce(void *bp){
    //현재블록은 free인 상태
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));//이전에 할당된 블록 가용 여부 1이면 사용중,0이면 free 상태
    size_t next_alloc = GET_ALLOC(FTRP(NEXT_BLKP(bp)));//현재 블록 다음 할당 블록 가용 여부
    size_t size = GET_SIZE(HDRP(bp));//현재 블록 사이즈
    //이전 블록 다음 블록 둘 다 사용중인 경우
    if(prev_alloc && next_alloc){
        //병합불가하니 그냥 포인터만 반환
        return bp;
    }
    //이전 블록은 사용중이고 다음 블록 이 free인 상태
    else if(prev_alloc && !next_alloc){
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));//다음 블록의 사이즈를 현재 블록 사이즈에 더해준다
        PUT(HDRP(bp), PACK(size, 0));//헤더에 적힌 크기 정보를 업데이트
        PUT(FTRP(bp), PACK(size, 0));//푸터에 적힌 크기 정보를 업데이트
    }
     else if(!prev_alloc && next_alloc){
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));//이전 블록의 사이즈를 현재 블록 사이즈에 더해준다
        PUT(FTRP(bp), PACK(size, 0));//푸터에 적힌 크기 정보를 업데이트
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0)); //이전 헤더에 적힌 크기 정보를 업데이트
        bp = PREV_BLKP(bp);//앞을 늘려줬기 때문에 현재포인터를 이전블록 포인터로 바꿔준다.
    }
    else{
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));//이전 다음 블록 사이즈 모두 합쳐주고
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));//이전 헤드 사이즈 업뎃
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));//다음 푸터 사이즈 업뎃
        bp = PREV_BLKP(bp);//이전블록 포인터가 현재 포인터가 됨
    }
    return bp;
}

static void *extend_heap(size_t words){
    char *bp;
    size_t size;
    //asize 가 짝수일때 block 한칸 추가, 홀수일 때는 추가x
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    //할당 실패시 NULL 반환
    if((long)(bp = mem_sbrk(size))==-1)
        return NULL;
    PUT(HDRP(bp), PACK(size, 0));// 기존 epilogue header 위치에 새 블록의 header 생성
    PUT(FTRP(bp), PACK(size, 0));// 새 블록의 footer 생성
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));// 새로운 epilogue header 생성
    //전 블록이 free 상태라면 합치기 진행
    return coalesce(bp);
}


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{   //묵시적 가용 리스트를 만들어 준다
    if((heap_listp = mem_sbrk(4*WSIZE)) == (void*)-1){
        return -1;
    }
    PUT(heap_listp, 0);//unused
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));//prologueHeader
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));//prologueFooter
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));//epilog header
    heap_listp += (2 * WSIZE);//prologue 가운데를 가르키는 포인터가 됨.

    if(extend_heap(CHUNKSIZE/WSIZE)==NULL){
        return -1;
    }

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose asize is a multiple of the alignment.
 */

static void *find_fit(size_t asize){
    void *bp;

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
        if(!GET_ALLOC(HDRP(bp)) && asize <= GET_SIZE(HDRP(bp))){
            return bp;
        }
    }
    return NULL;
    //에필로그 헤더의 사이즈는 0
    // char *bp = heap_listp + (3*WSIZE);
    // while(GET(HDRP(bp))!=0){
    //     size_t isused = GET_ALLOC(HDRP(bp));
    //     size_t free_size = GET_SIZE(HDRP(bp));
    //     if(!isused && asize<=free_size){
    //         return bp;
    //     }
    //     bp = bp + GET_SIZE(HDRP(bp));
    // }
    // return NULL;
}

static void place(void *bp, size_t asize){
    size_t free_size = GET_SIZE(HDRP(bp));
    if((free_size-asize) >= (2*DSIZE)){
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(free_size-asize, 0));
        PUT(FTRP(bp), PACK(free_size-asize, 0));
    }
    else{
        PUT(HDRP(bp), PACK(free_size, 1));
        PUT(FTRP(bp), PACK(free_size, 1));
    }
}

void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;
    if(size ==0){
        return NULL;
    }
    //요청 블록크기 조정 헤더 푸터 자리 만듬
    if(size<=DSIZE){
        asize = 2 * DSIZE;
    }
    else{//8의 배수로 맞춰줘야해서 연산이 약간 복잡하다 18바이트라면 18->20으로 맞춰주고 헤더블록 + 푸터블록 + 패딩블록 총 32비트가 필요하다
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    }
    //기존 힙에 블록이 들어갈자리가 있으면 넣는다.
    if((bp=find_fit(asize))!=NULL){
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if((bp=extend_heap(extendsize/WSIZE))==NULL){
        return NULL;
    }
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block dWoes nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    //이건 초기 header 기준!
    // copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    copySize = GET_SIZE(HDRP(ptr));
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}