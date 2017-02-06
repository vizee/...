#include "thrpool.h"
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

struct thrpool_task {
	thrpool_taskfn taskfn;
	void *param;
	struct thrpool_task *next;
};

struct thrpool_internal {
	int min_workers;
	pthread_cond_t worker_c;
	pthread_mutex_t worker_m;
	pthread_cond_t task_c;
	pthread_cond_t task_0_c;
	pthread_mutex_t task_m;
	volatile int destroy;
	volatile int running_workers;
	volatile int wait_task_0;
	struct thrpool_task *task_front;
	struct thrpool_task *task_rear;
};

static void _worker_cleanup(void * param)
{
	struct thrpool_internal *ti;

	ti = (struct thrpool_internal *)param;
	pthread_mutex_lock(&ti->worker_m);
	ti->running_workers--;
	pthread_mutex_unlock(&ti->worker_m);
	pthread_cond_signal(&ti->worker_c);
}

static void _task_enq(struct thrpool_internal *ti, struct thrpool_task *t)
{
	pthread_mutex_lock(&ti->task_m);
	if (ti->task_front == NULL) {
		ti->task_front = t;
	} else {
		ti->task_rear->next = t;
	}
	ti->task_rear = t;
	pthread_mutex_unlock(&ti->task_m);
	pthread_cond_signal(&ti->task_c);
}

static void *_worker(void *param)
{
	struct thrpool_internal *ti;
	struct thrpool_task *t;
	
	ti = (struct thrpool_internal *)param;
	pthread_cleanup_push(_worker_cleanup, ti);
	for (;;) {
		t = NULL;
		pthread_mutex_lock(&ti->task_m);
		while (ti->task_front == NULL) {
			if (ti->wait_task_0) {
				ti->wait_task_0 = 0;
				pthread_cond_broadcast(&ti->task_0_c);
			}
			if (ti->destroy) {
				break;
			}
			pthread_cond_wait(&ti->task_c, &ti->task_m);
		}
		if (ti->task_front != NULL) {
			t = ti->task_front;
			ti->task_front = t->next;
		}
		pthread_mutex_unlock(&ti->task_m);
		if (t == NULL) {
			break;
		}
		if (t->taskfn(t->param)) {
			t->next = NULL;
			_task_enq(ti, t);
		} else {
			free(t);
		}
	}
	pthread_cleanup_pop(1);
	return NULL;
}

thrpool_t thrpool_create(int workers)
{
	struct thrpool_internal *ti;
	int r;
	int i;
	pthread_t t;
	
	if (workers < 0) {
		return NULL;
	}
	ti = (struct thrpool_internal *)malloc(sizeof(struct thrpool_internal));
	memset(ti, 0, sizeof(struct thrpool_internal));
	ti->min_workers = workers;
	pthread_cond_init(&ti->worker_c, NULL);
	pthread_mutex_init(&ti->worker_m, NULL);
	pthread_cond_init(&ti->task_c, NULL);
	pthread_cond_init(&ti->task_0_c, NULL);
	pthread_mutex_init(&ti->task_m, NULL);
	r = 0;
	for (i = 0; i < workers; i++) {
		if (pthread_create(&t, NULL, _worker, ti) != 0) {
			r = 1;
			break;
		}
		pthread_detach(t);
		ti->running_workers++;
	}
	if (r == 1) {
		thrpool_destroy((thrpool_t)ti, 0);
		ti = NULL;
	}
	return (thrpool_t)ti;
}

void thrpool_destroy(thrpool_t pool, int wait)
{
	struct thrpool_internal *ti;
	struct thrpool_task *t;
	struct thrpool_task *next;

	if (pool == NULL) {
		return;
	}
	ti = (struct thrpool_internal *)pool;
	if (ti->destroy) {
		return;
	}
	if (!wait) {
		pthread_mutex_lock(&ti->task_m);
		t = ti->task_front;
		ti->task_front = NULL;
		pthread_mutex_unlock(&ti->task_m);
		while (t != NULL) {
			next = t->next;
			free(t);
			t = next;
		}
	}
	ti->destroy = 1;
	pthread_mutex_lock(&ti->worker_m);
	while (ti->running_workers > 0) {
		pthread_cond_signal(&ti->task_c);
		pthread_cond_wait(&ti->worker_c, &ti->worker_m);
	}
	pthread_mutex_unlock(&ti->worker_m);
	pthread_cond_destroy(&ti->worker_c);
	pthread_mutex_destroy(&ti->worker_m);
	pthread_cond_destroy(&ti->task_c);
	pthread_cond_destroy(&ti->task_0_c);
	pthread_mutex_destroy(&ti->task_m);
	free(ti);
}

int thrpool_add_task(thrpool_t pool, thrpool_taskfn taskfn, void *param)
{
	struct thrpool_internal *ti;
	struct thrpool_task *t;
	
	if (pool == NULL || taskfn == NULL) {
		return 1;
	}
	ti = (struct thrpool_internal *)pool;
	if (ti->destroy) {
		return 1;
	}
	t = (struct thrpool_task *)malloc(sizeof(struct thrpool_task));
	t->taskfn = taskfn;
	t->param = param;
	t->next = NULL;
	_task_enq(ti, t);
	return 0;
}

void thrpool_wait(thrpool_t pool)
{
	struct thrpool_internal *ti;
	
	if (pool == NULL) {
		return;
	}
	ti = (struct thrpool_internal *)pool;
	if (ti->destroy) {
		return;
	}
	pthread_mutex_lock(&ti->task_m);
	if (ti->task_front != NULL) {
		ti->wait_task_0 = 1;
		pthread_cond_wait(&ti->task_0_c, &ti->task_m);
	}
	pthread_mutex_unlock(&ti->task_m);
}
