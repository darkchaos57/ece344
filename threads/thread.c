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
	int not_empty;
	int setcontext_called;
	ucontext_t t_context; //the current context of the thread
};
typedef struct thread Thread;

//statically allocate THREAD_MAX_THREADS+1 thread structures into an array all_threads and set to NULL
Thread all_threads[THREAD_MAX_THREADS+1] = {{0}};

//declare linked list for ready queue (stores just the Tid of the thread in queue)
struct ready_node {
	Tid tid;
	struct ready_node *next;
};

//declare linked list for exit queue (stores just the Tid of the thread in queue)
struct exit_node {
	Tid tid;
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
//this is unused for now
int is_exit_empty(struct exit_queue *q) {
	return (q->rear == NULL);
}

/* This is the wait queue structure */
struct wait_queue {
	/* ... Fill this in Lab 3 ... */
};

//function to add threads to ready queue
void add_to_ready(struct ready_queue *q, Tid tid) {
	if(q->count < THREAD_MAX_THREADS) {
		//populate the queue (linked list)
		struct ready_node *temp;
		temp = malloc(sizeof(struct ready_node));
		temp->tid = tid; //stores the thread id of the element added to queue
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
void add_to_exit(struct exit_queue *q, Tid tid) {
	if(q->count < THREAD_MAX_THREADS) {
		//populate the queue (linked list)
		struct exit_node *temp;
		temp = malloc(sizeof(struct exit_node));
		temp->tid = tid;
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


//function to get tid from head of ready queue
Tid get_ready(struct ready_queue *q) {
	struct ready_node *temp;
	Tid tid = q->front->tid;
	temp = q->front;
	q->front = q->front->next; //set the front of queue to next
	//if q->front becomes NULL, make sure to set rear to NULL as well
	if(q->front == NULL) {
		q->rear = NULL;
	}
	q->count -= 1;
	//eventually, all initializations of the queue are freed
	free(temp);
	return(tid);
}

//function to get target thread from ready queue. return 1 if found, otherwise return 0
int get_ready_target(struct ready_queue *q, Tid tid) {
	int found = 0;
	struct ready_node *temp;
	struct ready_node *finder;
	temp = q->front;
	do {
		//found the tid
		if(temp->tid == tid) {
			//if this is the head
			if(temp == q->front) {
				//just pop the head of the queue
				get_ready(q);
				return 1;
			}
			else {
				//remove from queue
				finder = temp;
				temp = q->front;
				//loop temp until finder is found
				while(temp->next != finder) {
					temp = temp->next;
				}
				//point pointer before the target to pointer after the target
				if(finder == q->rear) {
					q->rear = temp;
				}
				temp->next = temp->next->next;
				//double check finder->tid is what we are looking for
				if(finder->tid == tid) {
					free(finder); //free the node we found
					found = 1;
					q->count -= 1;
					return found; //return since we found the tid
				}
			}
		}
		else {
			temp = temp->next;
		}
	} while(temp != NULL);
	//do something because tid is not in the queue
	return found;
}

//function to get tid from head of exit queue
Tid get_exit(struct exit_queue *q) {
	struct exit_node *temp;
	Tid tid = q->front->tid;
	temp = q->front;
	q->front = q->front->next;
	//if q->front becomes NULL, make sure to set rear to NULL as well
	if(q->front == NULL) {
		q->rear = NULL;
	}
	q->count -= 1;
	//eventually, all initializations of the queue are freed
	free(temp);
	return(tid);
}

/* thread starts by calling thread_stub. The arguments to thread_stub are the
 * thread_main() function, and one argument to the thread_main() function. */
void
thread_stub(void (*thread_main)(void *), void *arg)
{
	thread_main(arg); // call thread_main() function with arg
	thread_exit();
}

void
thread_init(void)
{
	//allocate a thread control block for the main kernel thread
	Thread * init_thread = (Thread *)malloc(sizeof(Thread));
	
	//set the context for the running thread which will be main
	init_thread->tid = 0;
	init_thread->state = RUNNING;
	init_thread->not_empty = 1;
	init_thread->setcontext_called = 0;
	all_threads[init_thread->tid] = *init_thread; //saves this thread and its context

	getcontext(&all_threads[init_thread->tid].t_context);
	long long int offset = all_threads[init_thread->tid].t_context.uc_mcontext.gregs[REG_RSP] % 16;
	all_threads[init_thread->tid].t_context.uc_mcontext.gregs[REG_RSP] -= offset;

	running_tid = init_thread->tid;

	//initialize queues (need to do this globally)
	q_ready = malloc(sizeof(struct ready_queue));
	initialize_ready(q_ready);
	
	q_exit = malloc(sizeof(struct exit_queue));
	initialize_exit(q_exit);
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
	//return tid of new thread
	Tid to_free;
	while(q_exit->count != 0) {
		to_free = get_exit(q_exit);
		free(all_threads[to_free].sp);
		all_threads[to_free] = all_threads[THREAD_MAX_THREADS];
	}

	int i = 0;
	//loop until we find a tid that is NULL (available)
	while(all_threads[i].not_empty == 1 && i < 1024) {
		i++; //increments until we find an available tid
	}
	if(i == 1024) {
		return THREAD_NOMORE; //if we couldn't find anything available after 1024 threads, then we return THREAD_NOMORE;
	}
	//set up the thread
	all_threads[i].tid = i;
	all_threads[i].state = READY;
	all_threads[i].sp = (void *)malloc(THREAD_MIN_STACK); //allocate space for new stack
	all_threads[i].not_empty = 1;
	all_threads[i].setcontext_called = 0;
	if(all_threads[i].sp == NULL) {
		return THREAD_NOMEMORY; //if malloc returns NULL, it means no space left to allocate stack
	}
	add_to_ready(q_ready, i); //add the thread to the ready queue
	//when the thread is ready to be run
	getcontext(&all_threads[i].t_context);
	long long int offset = (long long int) all_threads[i].sp % 16;
	all_threads[i].t_context.uc_mcontext.gregs[REG_RSP] = (long long int) (all_threads[i].sp + THREAD_MIN_STACK - offset - 8); //stack pointer to "bottom" of stack
	all_threads[i].t_context.uc_mcontext.gregs[REG_RBP] = (long long int) all_threads[i].sp; //base pointer to "top" of stack
	all_threads[i].t_context.uc_mcontext.gregs[REG_RDI] = (long long int) fn;
	all_threads[i].t_context.uc_mcontext.gregs[REG_RSI] = (long long int) parg;
	all_threads[i].t_context.uc_mcontext.gregs[REG_RIP] = (long long int) thread_stub;
	return(i);
	return THREAD_FAILED;
}

Tid
thread_yield(Tid want_tid)
{
	int found = 0;

	all_threads[running_tid].setcontext_called = 0;

	//save the current running thread context
	getcontext(&all_threads[running_tid].t_context);
	
	if(all_threads[running_tid].setcontext_called == 0) {
		//set members and add to ready queue
		all_threads[running_tid].state = READY;
		add_to_ready(q_ready, running_tid);
	
		//get want_tid context
		if(want_tid == THREAD_ANY) {
			if(q_ready->count == 1) {
				//make sure only thread is running thread (only possibility anyway)
				if(q_ready->front->tid == running_tid) {
					get_ready(q_ready); //clean up the queue
					return(THREAD_NONE); //no threads in ready queue
				}
			}
			//can only happen if last thread called thread_exit()
			all_threads[running_tid].setcontext_called = 1;
			running_tid = get_ready(q_ready);
			all_threads[running_tid].state = RUNNING;
			setcontext(&all_threads[running_tid].t_context);
		}
		//if same thread is run again
		else if(want_tid == THREAD_SELF) {
			found = get_ready_target(q_ready, running_tid); //will return 1 for sure
			all_threads[running_tid].state = RUNNING;
			all_threads[running_tid].setcontext_called = 1;
			setcontext(&all_threads[running_tid].t_context);
		}
		//if a specific thread is to be found to run
		else {
			found = get_ready_target(q_ready, want_tid);
			if(found) {
				all_threads[running_tid].setcontext_called = 1;
				running_tid = want_tid;
				all_threads[running_tid].state = RUNNING;
				setcontext(&all_threads[running_tid].t_context);
			}
			else {
				//must restore original thread
				found = get_ready_target(q_ready, running_tid);
				all_threads[running_tid].state = RUNNING;
				return(THREAD_INVALID);
			}
		}
	}
	if(all_threads[running_tid].setcontext_called == 1 && want_tid >= 0) {
		all_threads[running_tid].setcontext_called = 0;
		return(want_tid);
	}
	return(running_tid);
	return THREAD_FAILED; //not sure what this does
}

void
thread_exit()
{
	//set running thread to new thread from ready queue
	add_to_exit(q_exit, running_tid); //add the current running thread to the exit queue
	all_threads[running_tid].state = EXIT;
	if(q_ready->count > 0) {
		running_tid = get_ready(q_ready);
		all_threads[running_tid].state = RUNNING;
		setcontext(&all_threads[running_tid].t_context);
	}
	else {
		exit(0);
	}
}

Tid
thread_kill(Tid tid)
{
	if(tid < 0 || tid > THREAD_MAX_THREADS - 1) {
		return THREAD_INVALID;
	}
	int kill = 0;

	//return the tid of the thread just killed
	if(tid == running_tid || all_threads[tid].not_empty == 0) {
		return THREAD_INVALID; //returns invalid if the thread is the currently running one or does not exist
	}
	else {
		//make sure we don't access an empty list
		if(q_ready->count > 0) {
			kill = get_ready_target(q_ready, tid);	//fetch the target out of the ready queue to be killed
		}
		if(kill) {
			//ensures that it isn't something in the exit queue
			free(all_threads[tid].sp); //free the stack
			all_threads[tid] = all_threads[THREAD_MAX_THREADS]; //a zero-initialized structure to zero out killed thread structures
			return(tid);
		}
		else {
			return THREAD_INVALID; //in the exit queue, so it wil be killed later
		}
	}
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
