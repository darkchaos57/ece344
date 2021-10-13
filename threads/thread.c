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
				temp->next = finder->next;
				//double check finder->tid is what we are looking for
				if(finder->tid == tid) {
					free(finder); //free the node we found
					found = 1;
				}
			}
		}
		else {
			temp = temp->next;
		}
	} while(temp->next != NULL);
	//do something because tid is not in the queue
	return found;
}

//function to get tid from head of exit queue
Tid get_exit(struct exit_queue *q) {
	struct exit_node *temp;
	Tid tid = q->front->tid;
	temp = q->front;
	q->front = q->front->next; //set the front of queue to next
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
	init_thread->next = NULL;
	all_threads[init_thread->tid] = *init_thread; //saves this thread and its context
	
	running_tid = init_thread->tid;

	//initialize queues (need to do this globally)
	q_ready = malloc(sizeof(struct ready_queue));
	initialize_ready(q_ready);
	
	q_exit = malloc(sizeof(struct exit_queue));
	initialize_exit(q_exit);

	/*just to see if i actually made a thread.
	printf("Thread tid: %d, Thread state: %d", all_threads[init_thread->tid].tid, all_threads[init_thread->tid].state);*/

	/*try to add and remove a thread using ready queue
	 add_to_ready(q_ready, 0);
	 printf("This is what is in the front of the ready queue: %d, the rear of the ready queue: %d, and there are %d items in the ready queue.\n", q_ready->front->tid, q_ready->rear->tid, q_ready->count);
	 Tid tid = get_ready(q_ready);
	 printf("this is what we have at the start of the queue: %d\n", tid);
	 int is_empty = 0;
	 if(q_ready->front == NULL) is_empty = 1;
	 printf("This is the queue now (should be empty), count: %d, is list empty?: %d\n", q_ready->count, is_empty);
	 */


	//initialize a linked list of tid's from 1 to 1023
	//when a tid is used, take from the front of the linked list and point head to the
	//next tid, free the original head
	//when a tid is recycled, add the tid to the end of the linked list
	//this is faster than looping through tid's to find an available tid when create
	//this is faster than putting tid's into the first empty slot when exit

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
	//malloc stack of size struct thread
	//provide tid for thread create by taking the first element of an array containing tid's
	//that can be used
	//set the state
	//use getcontext of the current thread to save into t_context of new thread
	//set registers of new thread to point to the new stack, then set the frame pointer
	//to point to the new stack pointer (set registers, not sure how to do this)
	//add this new tid to the ready queue
	//setcontext the original caller to let the original caller continue running
	//return tid of new thread
	TBD();
	return THREAD_FAILED;
}

Tid
thread_yield(Tid want_tid)
{
	int found = 0;
	//make sure running_tid struct members are saved in all_threads[running_tid]
	//getcontext will take in an argument that references the t_context of the running_tid
	//place running_tid into the ready queue
	//fetch FIFO tid from ready queue that is ready to run
	//setcontext by referencing all_threads[ready_tid]
	//update all_threads[ready_tid] struct members
	//return tid of the new running thread (return(running_tid)) once updated
	
	//save the current running thread context
	getcontext(&all_threads[running_tid].t_context);

	//set members and add to ready queue
	all_threads[running_tid].state = READY;
	add_to_ready(q_ready, running_tid);

	//get want_tid context
	if(want_tid == THREAD_ANY) {
		if(q_ready->count == 0) return(THREAD_NONE); //no threads in ready queue
		//can only happen if last thread called thread_exit()
		running_tid = get_ready(q_ready);
		setcontext(&all_threads[running_tid].t_context);
		all_threads[running_tid].state = RUNNING;
	}
	//if same thread is run again
	else if(want_tid == THREAD_SELF) {
		found = get_ready_target(q_ready, running_tid); //will return 1 for sure
		all_threads[running_tid].state = RUNNING;
		setcontext(&all_threads[running_tid].t_context);
	}
	//if a specific thread is to be found to run
	else {
		found = get_ready_target(q_ready, want_tid);
		if(found) {
			running_tid = want_tid;
			all_threads[running_tid].state = RUNNING;
			setcontext(&all_threads[running_tid].t_context);
		}
		else {
			return(THREAD_INVALID);
		}
	}	
	return(running_tid);
	return THREAD_FAILED; //not sure what this does
}

void
thread_exit()
{
	//"yield" the thread by fetching a new tid from ready queue
	//getcontext the thread calling thread_exit() 
	//add the thread id to the exit queue
	//setcontext new thread from running queue
	//set running thread to new thread from ready queue
	TBD();
}

Tid
thread_kill(Tid tid)
{
	//check that the thread is in the exit queue
	//remove the thread tid from the ready queue (beginning, middle, or end)
	//find the thread id identified structure in all_threads and set all members to NULL
	//free the stack that the thread id was taking up
	//add this tid to an array containing all free tids
	//return the tid of the thread just killed
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
