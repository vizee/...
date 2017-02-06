#ifndef _THRPOOL_H_
#define _THRPOOL_H_

typedef void *thrpool_t;
typedef int (*thrpool_taskfn)(void *param);

thrpool_t thrpool_create(int workers);
void thrpool_destroy(thrpool_t pool, int wait);
int thrpool_add_task(thrpool_t pool, thrpool_taskfn taskfn, void *param);
void thrpool_wait(thrpool_t pool);

#endif /* !_THRPOOL_H_ */
