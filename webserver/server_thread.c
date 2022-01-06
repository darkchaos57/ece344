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
	struct hash_table *hash_table;

	/* add any other parameters you need */
};

/* static functions */
int in, out, requests = 0; //requests keeps track of number of requests so we don't overwrite our circular buffer
//static initializations, because our parameters are known
pthread_mutex_t lock, hash_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;

//cache
typedef struct cache_hash {
	struct file_data *file_data;
}cache_hash;

//by default we assume the hash table to have max_cache_size number of indices, as there can't be more files than the number of bytes of the cache
struct hash_table {
	cache_hash **entry;
	unsigned long available_size;
};

struct LRU_node {
	unsigned long hash_key;
	struct LRU_node *next;
};

struct LRU_queue {
	struct LRU_node *front;
	struct LRU_node *rear;
}*q_LRU;

void initialize_LRU(struct LRU_queue *q) {
	q->front = NULL;
	q->rear = NULL;
}

int is_LRU_empty(struct LRU_queue *q) {
	return(q->rear == NULL);
}

void add_to_LRU(struct LRU_queue *q, unsigned long hash_key) {
	struct LRU_node *temp = (struct LRU_node *)Malloc(sizeof(struct LRU_node));
	temp->hash_key = hash_key;
	temp->next = NULL;
	if(!is_LRU_empty(q)) {
		q->rear->next = temp;
		q->rear = temp;
	}
	else {
		q->front = temp;
		q->rear = temp;
	}
}

int get_LRU(struct LRU_queue *q, struct hash_table *hash_table) {
	struct LRU_node *temp = NULL;
	int file_size_rm = 0;
	if(q->front == NULL) {
		q->rear = NULL;
	}
	else {
		temp = q->front;
		q->front = q->front->next;
	}
	if(temp != NULL) {
		file_size_rm = hash_table->entry[temp->hash_key]->file_data->file_size; //fetch the file size of evicted file
		free(hash_table->entry[temp->hash_key]->file_data);
		hash_table->entry[temp->hash_key]->file_data = NULL;
		free(hash_table->entry[temp->hash_key]);
		hash_table->entry[temp->hash_key] = NULL;
		free(temp);
	}
	return(file_size_rm);
}

//hash function from cse.yorku.ca/~oz/hash.html
unsigned long hash_function(char *str, int table_size) {
	unsigned long hash = 5381;
	int c = 0;
	
	while ((c = *str++)) {
		hash = ((hash << 5) + hash) + c;
	}
	return hash % table_size;
}

//inserts file name and file data to hashed location in hash table, which makes up our cache
void cache_insert(struct hash_table *hash_table, struct file_data *data, int max_cache_size, unsigned long key) {
	//there can't be more than max_cache_size number of files in the cache, unless every file is 1 byte, so this is reasonable
	//unsigned long key = hash_fddunction(data->file_name, max_cache_size); //hash the file name
	if(hash_table->entry[key] == NULL) {
		hash_table->entry[key] = Malloc(sizeof(cache_hash));
	}
	if(hash_table->entry[key]->file_data == NULL) {
		//store the data into the hash table entries
		memset(hash_table->entry[key], 0, sizeof(cache_hash));
		hash_table->entry[key]->file_data = (struct file_data *)Malloc(sizeof(struct file_data) * data->file_size);
		memset(hash_table->entry[key], 0, sizeof(cache_hash));
		hash_table->entry[key]->file_data = (struct file_data *)Malloc(sizeof(struct file_data) * data->file_size);
		hash_table->entry[key]->file_data->file_name = malloc(strlen(data->file_name)+1);
		hash_table->entry[key]->file_data->file_buf = malloc(data->file_size + 1);
		memset(hash_table->entry[key]->file_data->file_buf, 0, data->file_size + 1);
		memset(hash_table->entry[key]->file_data->file_name, 0, strlen(data->file_name)+1);
		memcpy(hash_table->entry[key]->file_data->file_buf, data->file_buf, data->file_size + 1);
		hash_table->entry[key]->file_data->file_size = data->file_size;
		strncpy(hash_table->entry[key]->file_data->file_name, data->file_name, strlen(data->file_name)+1);
		add_to_LRU(q_LRU, key);
	}
	else {
		while(hash_table->entry[key] != NULL) {
			key = (key + 1) % max_cache_size;
		}
		hash_table->entry[key] = Malloc(sizeof(cache_hash));
		memset(hash_table->entry[key], 0, sizeof(cache_hash));
		hash_table->entry[key]->file_data = (struct file_data *)Malloc(sizeof(struct file_data) * data->file_size);
		hash_table->entry[key]->file_data->file_name = malloc(strlen(data->file_name)+1);
		hash_table->entry[key]->file_data->file_buf = malloc(data->file_size + 1);
		memset(hash_table->entry[key]->file_data->file_buf, 0, data->file_size + 1);
		memset(hash_table->entry[key]->file_data->file_name, 0, strlen(data->file_name)+1);
		memcpy(hash_table->entry[key]->file_data->file_buf, data->file_buf, data->file_size + 1);
		hash_table->entry[key]->file_data->file_size = data->file_size;
		strncpy(hash_table->entry[key]->file_data->file_name, data->file_name, strlen(data->file_name)+1);
		add_to_LRU(q_LRU, key);
	}
}

//looksup the cache based on file name (by hashing it)
int cache_lookup(struct hash_table *hash_table, struct file_data *data, int max_cache_size, int key) {
	if(key == -1) {
		//first entry into function
		key = hash_function(data->file_name, max_cache_size);
	}
	if(hash_table->entry[key] == NULL) {
		//not found
		return -1;
	}
	else if(strncmp(hash_table->entry[key]->file_data->file_name, data->file_name, strlen(data->file_name)) == 0) {
		//do something with the found cache
		return key;
	}
	else {
		//walk the hash table as if collisions had happened until cache is found
		if(key + 1 < max_cache_size) {
			key = cache_lookup(hash_table, data, max_cache_size, key + 1);
		}
		else {
			key = cache_lookup(hash_table, data, max_cache_size, 0);
		}
	}
	return key;
}

//evict the selected file name from the hash table
void cache_evict(int file_size, struct hash_table *hash_table, int max_cache_size) {
	//find key at head of LRU queue, pop off the queue, delete and free elements of that key in hash table
	//remember size of file that was removed, check to see if its larger than file_size
	//if not, that means we need to continue evicting (use cache_evict in a loop)
	/*int size_evicted = get_LRU(q_LRU, hash_table); //size of file evicted
	hash_table->available_size += size_evicted;
	if(hash_table->available_size < file_size) {
		cache_evict(file_size, hash_table);
	}*/
	for(int i = 0; i < max_cache_size; i++) {
		if(hash_table->entry[i] != NULL) {
			if(hash_table->entry[i]->file_data != NULL) {
				hash_table->entry[i]->file_data = NULL;
			}
			hash_table->entry[i] = NULL;
		}
	}
	hash_table->available_size = max_cache_size;
}

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
	if(sv->max_cache_size > 0) {
		pthread_mutex_lock(&hash_lock);
		int key = cache_lookup(sv->hash_table, data, sv->max_cache_size, -1);
		if(key == -1) {
			//if this file was not cached before, send it, hash it and then insert
			ret = request_readfile(rq);
			if(ret == 0) {
				goto out;
			}
			request_sendfile(rq);
			if(data->file_size < sv->hash_table->available_size) {
				//only cache if the file is small enough to fit in the cache
				key = hash_function(data->file_name, sv->max_cache_size);
				cache_insert(sv->hash_table, data, sv->max_cache_size, key);
				sv->hash_table->available_size -= data->file_size;
			}
			else if(data->file_size > sv->hash_table->available_size && data->file_size < sv->max_cache_size) {
				//if cache is large enough to accommodate file but doesn't have enough space, evict until enough space
				cache_evict(data->file_size, sv->hash_table, sv->max_cache_size);
				//once enough space, insert
				key = hash_function(data->file_name, sv->max_cache_size);
				cache_insert(sv->hash_table, data, sv->max_cache_size, key);
				sv->hash_table->available_size -= data->file_size;
			}
		}
		else {
			//if found in cache, set rq to data and send
			request_set_data(rq, sv->hash_table->entry[key]->file_data);
			request_sendfile(rq);
		}
	}
	else {
		pthread_mutex_lock(&hash_lock);
		ret = request_readfile(rq);
		if(ret == 0) {
			goto out;
		}
		request_sendfile(rq);
	}
	/* read file, 
	 * fills data->file_buf with the file contents,
	 * data->file_size with file size. */
	//ret = request_readfile(rq);
	//if (ret == 0) { couldn't read file
	//	goto out;
	//}
	/* send file to client */
	//request_sendfile(rq);
out:
	request_destroy(rq);
	file_data_free(data);
	pthread_mutex_unlock(&hash_lock);
}

/* entry point functions */

//stub function is entry point for threads, which waits until there are requests signalled by server_request as they come in
//when that happens, we save the data in the buffer and then increment the buffer out ndex
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
	
	q_LRU = Malloc(sizeof(struct LRU_queue));
	initialize_LRU(q_LRU);
	
	if (nr_threads > 0 || max_requests > 0 || max_cache_size > 0) {
		if(max_cache_size > 0) {
			sv->hash_table = Malloc(sizeof(struct hash_table));
			sv->hash_table->entry = Malloc(sizeof(cache_hash) * max_cache_size); //average size of file
			//populate hash table
			for (int i = 0; i < max_cache_size; i++) {
				sv->hash_table->entry[i] = NULL;
			}
			sv->hash_table->available_size = max_cache_size*sizeof(cache_hash)/10117;
		}
		if(max_requests > 0) {
			sv->buffer = (int *)Malloc(sizeof(int) * (max_requests + 1)); //create a circular buffer of max_request (static) size when max_requests > 0
		}
		if(nr_threads > 0) {
			sv->worker_threads = (pthread_t *)Malloc(sizeof(pthread_t) * nr_threads); //allocate memory for number of worker threads
			for(int i = 0; i < nr_threads; i++) {
				pthread_create(&(sv->worker_threads[i]), NULL, (void *)&stub, (void *)sv); //create n_threads and initialize them in stub
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
	if(sv->nr_threads > 0) {
		free(sv->worker_threads);
	}
	if(sv->max_requests > 0) {
		free(sv->buffer);
	}
	free(sv);
}
