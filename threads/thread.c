#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>
#include "thread.h"
#include "interrupt.h"

//define thread states
#define READY 0
#define RUNNING 1
#define EXIT 2

//declare global variables
Tid running_tid = 0;

//statically allocate THREAD_MAX_THREADS thread structures into an array all_threads
struct thread * all_threads[THREAD_MAX_THREADS];

//declare ready queue
struct ready_queue {
	Tid tid; //id of the thread in ready queue
	struct ready_queue *next; //pointer to the next thread in ready queue
};

//declare exit queue
struct exit_queue {
	Tid tid; //id of the thread in exit queue
	struct exit_queue *next; //pointer to the next thread in exit queue
};

/* This is the wait queue structure */
struct wait_queue {
	/* ... Fill this in Lab 3 ... */
};

//function to add threads to ready queue
void add_to_ready(Tid tid) {
	;
}

//function to add threads to exit queue
void add_to_exit(Tid tid) {
	;
}

//function to get threads from ready queue
void get_ready(Tid tid) {
	;
}

//function to get threads from exit queue
void get_exit(Tid tid) {
	;
}

/* This is the thread control block */
struct thread {
	/* ... Fill this in ... */
	Tid tid; //the id of the current thread
	int state; //the state of the current thread
	void *sp; //a pointer to the stack pointer (bottom of malloc'd stack for the thread)
	struct thread *next; //pointer to next thread in a queue
	ucontext_t t_context; //the current context of the thread
}

/* thread starts by calling thread_stub. The arguments to thread_stub are the
 * thread_main() function, and one argument to the thread_main() function. */
void
thread_stub(void (*thread_main)(void *), void *arg)
{
	Tid ret;

	thread_main(arg); // call thread_main() function with arg
	thread_exit();
}

void
thread_init(void)
{
	/* your optional code here */
	for(int i = 0; i < THREAD_MAX_THREADS; i++) {
		//initialize all thread structures as null (includes the tid)
		all_threads[i] = NULL;
	}

	//allocate a thread control block for the main kernel thread
	struct thread * main = (struct thread *)malloc(sizeof(struct thread));
	//save the context of the main kernel thread
	getcontext(&main->t_context);

	//set the context for the running thread which will be main
	main->tid = 0;
	main->state = RUNNING;
	main->sp = NULL;
	main->next = NULL;
	
	Tid running_tid = main->tid;
}

Tid
thread_id()
{
	//double check to see if the thread is actually in the running state
	if(all_threads[running_tid]->state == RUNNING) {
		return running_tid;
	}
	return THREAD_INVALID;
}

Tid
thread_create(void (*fn) (void *), void *parg)
{
	TBD();
	return THREAD_FAILED;
}

Tid
thread_yield(Tid want_tid)
{
	TBD();
	return THREAD_FAILED;
}

void
thread_exit()
{
	TBD();
}

Tid
thread_kill(Tid tid)
{
	TBD();
	return THREAD_FAILED;
}

/*******************************************************************
 * Important: The rest of the code should be implemented in Lab 3. *
 *******************************************************************/

/* make sure to fill the wait_queue structure defined above */
struct wait_queue *
wait_queue_create()
{
	struct wait_queue *wq;

	wq = malloc(sizeof(struct wait_queue));
	assert(wq);

	TBD();

	return wq;
}

void
wait_queue_destroy(struct wait_queue *wq)
{
	TBD();
	free(wq);
}

Tid
thread_sleep(struct wait_queue *queue)
{
	TBD();
	return THREAD_FAILED;
}

/* when the 'all' parameter is 1, wakeup all threads waiting in the queue.
 * returns whether a thread was woken up on not. */
int
thread_wakeup(struct wait_queue *queue, int all)
{
	TBD();
	return 0;
}

/* suspend current thread until Thread tid exits */
Tid
thread_wait(Tid tid)
{
	TBD();
	return 0;
}

struct lock {
	/* ... Fill this in ... */
};

struct lock *
lock_create()
{
	struct lock *lock;

	lock = malloc(sizeof(struct lock));
	assert(lock);

	TBD();

	return lock;
}

void
lock_destroy(struct lock *lock)
{
	assert(lock != NULL);

	TBD();

	free(lock);
}

void
lock_acquire(struct lock *lock)
{
	assert(lock != NULL);

	TBD();
}

void
lock_release(struct lock *lock)
{
	assert(lock != NULL);

	TBD();
}

struct cv {
	/* ... Fill this in ... */
};

struct cv *
cv_create()
{
	struct cv *cv;

	cv = malloc(sizeof(struct cv));
	assert(cv);

	TBD();

	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

	TBD();

	free(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}
