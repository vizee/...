#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <malloc.h>
#include <pthread.h>
#include "../thrpool.h"

static int task_proc(void *param)
{
	int *p = (int *)param;

	if (*p > 4) {
		return 0;
	}
	(*p)++;
	printf("%p -> %d\n", p, *p);
	sleep(1);
	return 1;
}

int main()
{
	thrpool_t pool;
	int n;
	int m;
	
	pool = thrpool_create(1);
	n = 0;
	m = 0;
	thrpool_add_task(pool, task_proc, &n); 
	thrpool_add_task(pool, task_proc, &m);
	sleep(1);
	thrpool_destroy(pool, 1);
	printf("end\n");
	return 0;
}
