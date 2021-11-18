#include "request.h"
#include "server_thread.h"
#include "common.h"

struct server {
	int nr_threads;
	int max_requests;
	int max_cache_size;
	int exiting;
	int *buffer;
	pthread_t *worker_threads;
	/* add any other parameters you need */
};

/* static functions */
int in, out, requests = 0;
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
void *stub(struct server *sv) {
	while(1) {
		pthread_mutex_lock(&lock);
		//wait when empty
		while(requests == 0) {
			pthread_cond_wait(&empty, &lock);
			if(sv->exiting) {
				pthread_mutex_unlock(&lock);
				pthread_exit(sv);
			}
		}
		int connfd = sv->buffer[out];
		out++;
		out = out % sv->max_requests;
		requests--;
		pthread_cond_signal(&full);
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
			sv->buffer = (int *)malloc(sizeof(int) * (max_requests + 1)); //create a queue of max_request size when max_requests > 0
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

void
server_request(struct server *sv, int connfd)
{
	if (sv->nr_threads == 0) { /* no worker threads */
		do_server_request(sv, connfd);
	} else {
		/*  Save the relevant info in a buffer and have one of the
		 *  worker threads do the work. */
		pthread_mutex_lock(&lock);
		//wait when full
		while(requests == sv->max_requests) {
			pthread_cond_wait(&full, &lock);
			if(sv->exiting) {
				pthread_mutex_unlock(&lock);
				pthread_exit(sv);
			}
		}
		sv->buffer[in] = connfd; //save the request into the buffer
		in++;
		in = in % sv->max_requests; //in case we need to loop back to the 0th position
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
	pthread_cond_broadcast(&full);
	pthread_cond_broadcast(&empty);

	for(int i = 0; i < sv->nr_threads; i++) {
		pthread_join(sv->worker_threads[i], NULL);
		free(&sv->worker_threads[i]); //should be okay since pthread_join returns after thread has exitted
	}

	/* make sure to free any allocated resources */
	if(sv->nr_threads > 0) {
		free(sv->worker_threads);
	}
	if(sv->max_requests > 0) {
		free(sv->buffer);
	}
	free(sv);
}
