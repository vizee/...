#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include "../thrpool.h"

static int task_proc(void *param)
{
	printf("thread %p param %p\n", (void *)pthread_self(), param);
	sleep(1);
	return 0;
}

int main()
{
	thrpool_t pool;
	int i;
	
	pool = thrpool_create(2);
	for (i = 0; i < 10; i++) {
		thrpool_add_task(pool, task_proc, (void *)(uintptr_t)i);
	}
	printf("begin\n");
	thrpool_wait(pool);
	thrpool_destroy(pool, 0);
	printf("end\n");
	return 0;
}

