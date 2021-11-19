#include "request.h"
#include "server_thread.h"
#include "common.h"

struct server {
	int nr_threads;
	int max_requests;
	int max_cache_size;
	int exiting;
	int *buffer; //circular buffer implementation
	pthread_t *worker_threads;
	/* add any other parameters you need */
};

/* static functions */
int in, out, requests = 0; //requests keeps track of number of requests so we don't overwrite our circular buffer
//static initializations, because our parameters are known
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;

/* initialize file data */
static struct file_data *
file_data_init(void)
{
	struct file_data *data;

	data = Malloc(sizeof(struct file_data));
	data->file_name = NULL;
	data->file_buf = NULL;
	data->file_size = 0;
	return data;
}

/* free all file data */
static void
file_data_free(struct file_data *data)
{
	free(data->file_name);
	free(data->file_buf);
	free(data);
}

static void
do_server_request(struct server *sv, int connfd)
{
	int ret;
	struct request *rq;
	struct file_data *data;

	data = file_data_init();

	/* fill data->file_name with name of the file being requested */
	rq = request_init(connfd, data);
	if (!rq) {
		file_data_free(data);
		return;
	}
	/* read file, 
	 * fills data->file_buf with the file contents,
	 * data->file_size with file size. */
	ret = request_readfile(rq);
	if (ret == 0) { /* couldn't read file */
		goto out;
	}
	/* send file to client */
	request_sendfile(rq);
out:
	request_destroy(rq);
	file_data_free(data);
}

/* entry point functions */

//stub function is entry point for threads, which waits until there are requests signalled by server_request as they come in
//when that happens, we save the data in the buffer and then increment the buffer out index
//then we decrement the number of requests waiting in the buffer, signal that the buffer is no longer full, and then do the server request
void *stub(struct server *sv) {
	//this loop ensures that all requests are handled until the thread is specifically called to exit
	while(1) {
		pthread_mutex_lock(&lock);
		//wait when empty
		while(requests == 0) {
			pthread_cond_wait(&empty, &lock);
			//if thread is called to exit, need to release lock and exit itself
			if(sv->exiting) {
				pthread_mutex_unlock(&lock);
				pthread_exit(sv); //i think this is impacting my performance as i am getting upper bound performance times
			}
		}
		int connfd = sv->buffer[out];
		out++;
		out = out % sv->max_requests; //circular buffer indexing
		requests--;
		pthread_cond_signal(&full); //signal that the buffer is no longer full
		pthread_mutex_unlock(&lock);
		do_server_request(sv, connfd);
	}
}


struct server *
server_init(int nr_threads, int max_requests, int max_cache_size)
{
	struct server *sv;

	sv = Malloc(sizeof(struct server));
	sv->nr_threads = nr_threads;
	sv->max_requests = max_requests;
	sv->max_cache_size = max_cache_size;
	sv->exiting = 0;
	
	if (nr_threads > 0 || max_requests > 0 || max_cache_size > 0) {
		if(max_requests > 0) {
			sv->buffer = (int *)malloc(sizeof(int) * (max_requests + 1)); //create a circular buffer of max_request (static) size when max_requests > 0
		}
		if(nr_threads > 0) {
			sv->worker_threads = (pthread_t *)malloc(sizeof(pthread_t) * nr_threads); //allocate memory for number of worker threads
			for(int i = 0; i < nr_threads; i++) {
				pthread_create(&(sv->worker_threads[i]), NULL, (void *)&stub, sv); //create n_threads and initialize them in stub
			}
		}
	}

	/* Lab 4: create queue of max_request size when max_requests > 0 */

	/* Lab 5: init server cache and limit its size to max_cache_size */

	/* Lab 4: create worker threads when nr_threads > 0 */

	return sv;
}

//continues saving data into buffers until it is full. Once it is full, signals to waiting worker threads that they can begin processing requests
//increments requests to keep track of number of requests in buffer so that the buffer does not overflow and lose data
void
server_request(struct server *sv, int connfd)
{
	if (sv->nr_threads == 0) { /* no worker threads */
		do_server_request(sv, connfd);
	} else {
		/*  Save the relevant info in a buffer and have one of the
		 *  worker threads do the work. */
		pthread_mutex_lock(&lock);
		//wait when full (using while instead of if is cleaner as per lecture)
		while(requests == sv->max_requests) {
			pthread_cond_wait(&full, &lock);
			//necessary check, because if thread is waiting for signal, but the server is planning to exit,
			//this thread will need to return and release its lock in order for join to complete
			if(sv->exiting) {
				pthread_mutex_unlock(&lock);
				pthread_exit(sv);
			} //if broadcast is called in thread_exit, this will let all threads release the lock and exit themselves
		}
		sv->buffer[in] = connfd; //save the request into the buffer
		in++;
		in = in % sv->max_requests; //circular buffer indexing
		requests++;
		pthread_cond_signal(&empty); //let the initialized threads know that there is a new request that came in
		pthread_mutex_unlock(&lock);
	}
}

void
server_exit(struct server *sv)
{
	/* when using one or more worker threads, use sv->exiting to indicate to
	 * these threads that the server is exiting. make sure to call
	 * pthread_join in this function so that the main server thread waits
	 * for all the worker threads to exit before exiting. */
	sv->exiting = 1;
	//broadcasts to all threads with full or empty condition variables that we are exiting, so they can wakeup
	//when they are context switched to
	pthread_cond_broadcast(&full);
	pthread_cond_broadcast(&empty);

	int ret;
	
	//makes sure every thread successfully exits before freeing data, and then letting main thread exit
	for(int i = 0; i < sv->nr_threads; i++) {
		//join will wait for thread to exit (return) before continuing, so loop will make sure all threads exit
		ret = pthread_join(sv->worker_threads[i], NULL);
		//checks that join indeed returns 0 without any errors
		assert(!ret);
	}

	/* make sure to free any allocated resources */
	free(sv->worker_threads);
	free(sv->buffer);
	free(sv);
}
