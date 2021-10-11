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

/* This is the thread control block */
struct thread {
	/* ... Fill this in ... */
	Tid tid; //the id of the current thread
	int state; //the state of the current thread
	void *sp; //a pointer to the stack pointer (bottom of malloc'd stack for the thread)
	struct thread *next; //pointer to next thread in a queue
	ucontext_t t_context; //the current context of the thread
};
typedef struct thread Thread;

//statically allocate THREAD_MAX_THREADS thread structures into an array all_threads and set to NULL
Thread all_threads[THREAD_MAX_THREADS] = {{0}};

//declare linked list for ready queue
struct ready_node {
	struct thread thread;
	struct ready_node *next;
};

//declare linked list for exit queue
struct exit_node {
	struct thread thread;
	struct exit_node *next;
};

//declare ready queue
struct ready_queue {
	int count;
	struct ready_node *front;
	struct ready_node *rear;
}*q_ready;

//declare exit queue
struct exit_queue {
	int count;
	struct exit_node *front;
	struct exit_node *rear;
}*q_exit;

//initializes queues
void initialize_ready(struct ready_queue *q) {
	q->count = 0;
	q->front = NULL;
	q->rear = NULL;
}

void initialize_exit(struct exit_queue *q) {
	q->count = 0;
	q->front = NULL;
	q->rear = NULL;
}

//checks if queues are empty
int is_ready_empty(struct ready_queue *q) {
	return (q->rear == NULL);
}

int is_exit_empty(struct exit_queue *q) {
	return (q->rear == NULL);
}

/* This is the wait queue structure */
struct wait_queue {
	/* ... Fill this in Lab 3 ... */
};

//function to add threads to ready queue
void add_to_ready(struct ready_queue *q, struct thread thread) {
	if(q->count < THREAD_MAX_THREADS) {
		//populate the queue (linked list)
		struct ready_node *temp;
		temp = malloc(sizeof(struct ready_node));
		temp->thread = thread;
		temp->next = NULL;
		//if the queue is not empty
		if(!is_ready_empty(q)) {
			//set the rear to the new node
			q->rear->next = temp;
			q->rear = temp;
		}
		else {
			q->front = temp;
			q->rear = temp;
		}
		q->count += 1; //keep track of how many items in ready queue
	}
	else {
		printf("queue full, max threads exceeded.");
	}
}

//function to add threads to exit queue
void add_to_exit(struct exit_queue *q, struct thread thread) {
	if(q->count < THREAD_MAX_THREADS) {
		//populate the queue (linked list)
		struct exit_node *temp;
		temp = malloc(sizeof(struct exit_node));
		temp->thread = thread;
		temp->next = NULL;
		//if the queue is not empty
		if(!is_exit_empty(q)) {
			//set the rear to the new node
			q->rear->next = temp;
			q->rear = temp;
		}
		else {
			q->front = temp;
			q->rear = temp;
		}
		q->count += 1; //keep track of how many items in exit queue
	}
	else {
		printf("queue full, max threads exceeded.");
	}
}


//function to get threads from ready queue
Thread get_ready(struct ready_queue *q) {
	struct ready_node *temp;
	Thread thread = q->front->thread;
	temp = q->front;
	q->front = q->front->next; //set the front of queue to next
	q->count -= 1;
	//eventually, all initializations of the queue are freed
	free(temp);
	return(thread);
}

//function to get threads from exit queue
Thread get_exit(struct exit_queue *q) {
	struct exit_node *temp;
	Thread thread = q->front->thread;
	temp = q->front;
	q->front = q->front->next; //set the front of queue to next
	q->count -= 1;
	//eventually, all initializations of the queue are freed
	free(temp);
	return(thread);
}

/* thread starts by calling thread_stub. The arguments to thread_stub are the
 * thread_main() function, and one argument to the thread_main() function. */
void
thread_stub(void (*thread_main)(void *), void *arg)
{
	//Tid ret; //currently unused causing compile errors

	thread_main(arg); // call thread_main() function with arg
	thread_exit();
}

void
thread_init(void)
{
	/* your optional code here */

	//allocate a thread control block for the main kernel thread
	Thread * init_thread = (Thread *)malloc(sizeof(Thread));
	//save the context of the main kernel thread
	getcontext(&init_thread->t_context);

	//set the context for the running thread which will be main
	init_thread->tid = 0;
	init_thread->state = RUNNING;
	init_thread->sp = NULL;
	init_thread->next = NULL;
	
	running_tid = init_thread->tid;

	//initialize queues (need to do this globally)
	q_ready = malloc(sizeof(struct ready_queue));
	initialize_ready(q_ready);
	
	q_exit = malloc(sizeof(struct exit_queue));
	initialize_exit(q_exit);

	/* the following code tests the queues functions only */
	
}

Tid
thread_id()
{
	//double check to see if the thread is actually in the running state
	if(all_threads[running_tid].state == RUNNING) {
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
