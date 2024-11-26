#include <stdio.h>
#include <errno.h>
//다른 소스 파일에서 정의된 함수들
extern int mm_init(void);
extern void *mm_malloc(size_t size);
extern void mm_free(void *ptr);

//정적 포인터 변수 선언
static char *mem_heap; // 힙공간이 말록에의해 할당 된 후 힙공간의 시작 주소를 가리킴
static char *mem_brk;//할당이 시작된 후 힙의 꼭대기 주소를 가리킴
static char *mem_max_addr;//처음 초기화된 힙의 끝 주소를 가리킴

#define MAX_HEAP (20*(1<<20))

void mem_init(void){
    mem_heap =(char*)Malloc(MAX_HEAP);// 힙공간이 말록에의해 할당 된 후 힙공간의 시작 주소를 가리킴
    mem_brk = (char *)mem_heap;//할당이 시작된 후 힙의 꼭대기 주소를 가리킴
    mem_max_addr = (char *)(mem_heap + MAX_HEAP);//처음 초기화된 힙의 끝 주소를 가리킴
}
//추가적인 힙 메모리를 요청하는 함수
void *mem_sbrk(int incr)
{   //문자형 포인터 old_brk 안에 힙의 끝주소를 넣는다.
    char *old_brk = mem_brk;
    //인자 값이 0 보나 작거나  할당될 값이 힙 공간을 초과한다면
    if((incr)<0 || ((mem_brk +incr)>mem_max_addr)){
        //에러 출력
        errno = ENOMEM;
        fprintf(stderr, "ERROR: mem_sbrk failed. Ran out of memory...\n");
        //-1 반환
        return (void *)-1;
    }
    //힙의 꼭대기 주소 에 incr 만큼 더한 주소를 mem_brk 에 넣음.
    mem_brk += incr;
    //이전 주소 반환
    return (void *)old_brk;
}
