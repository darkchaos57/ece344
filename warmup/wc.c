#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "wc.h"
#include <string.h>
#include <ctype.h>

typedef struct entry {
	char *word;
	int value;
}entry;

struct wc{
	entry **entries;
	long size;
};

unsigned long hash_function(char *str, long table_size) {
	//cse.yorku.ca/~oz/hash.html
	unsigned long hash = 5381;
	int c = 0;
	
	while ((c = *str++)) {
		hash = ((hash << 5) + hash) + c;
	}
	return hash % table_size;
}

void add_hash(struct wc *wc, char *word, int key, int word_len) {
	if(wc->entries[key]->word == NULL) {
		wc->entries[key]->word = (char *)malloc((word_len + 1)*sizeof(char));
		strncpy(wc->entries[key]->word, word, word_len); //saves the word to the key location of the hash map
		wc->entries[key]->value = 1; //keeps track of the first time a word appears in the hash map
	}
	else if(strncmp(wc->entries[key]->word, word, word_len) == 0) {
		wc->entries[key]->value += 1; //keep track of n-th appearance of word
	}
	else {
		//if the next key location isn't out of bounds
		if(key + 1 < wc->size) {
			add_hash(wc, word, key + 1, word_len); //try again at the next key location
		}
		//loop to index 0
		else {
			add_hash(wc, word, 0, word_len);
		}
	}
}

void create_words(struct wc* wc, char *word_array) {
	char *temp_word = NULL;
	int i = 0;
	int j = 0;
	int word_len = 0;
	int word_start = 0;
	unsigned long hash_key;

	while (word_array[i] != '\0') {
		//if not a character
		if(isspace(word_array[i])) {
			//check if the length of word is greater than 0
			if (word_len > 0) {
				temp_word = (char *)malloc((word_len + 1)*sizeof(char)); //saves space for word and null character
				assert(temp_word);
				
				while (j < word_len) {
					temp_word[j] = word_array[word_start + j]; //traverse the word and create string
					j++;
				}
				temp_word[j] = '\0'; //append the null character at the end of the word
				hash_key = hash_function(temp_word, wc->size); //find the hash key
				add_hash(wc, temp_word, hash_key, word_len); //add this to the hash table
				//printf("%s, %ld\n", temp_word, hash_key); //makes sure hash function hashes correctly
			}
			word_start = i+1;
			j = 0;
			word_len = 0;

		}
		else {
			word_len++;
		}
		i++;
	}
	/*view hash table
	for(long k = 0; k < wc->size; k++) {
		printf("%ld, %s, %d\n", k, wc->entries[k]->word, wc->entries[k]->value);
	}*/
}

struct wc *
wc_init(char *word_array, long size)
{
	struct wc *wc;

	wc = (struct wc *)malloc(sizeof(struct wc));
	assert(wc);

	wc->entries = (entry **)malloc(size * sizeof(entry *));
	assert(wc->entries);

	for(int i = 0; i < size; i++) {
		wc->entries[i] = (entry *)malloc(size * sizeof(entry));
	}

	wc->size = size;

	create_words(wc, word_array);
	
	wc_output(wc);

	return wc;
}

void
wc_output(struct wc *wc)
{
	for(long i = 0; i < wc->size; i++) {
		if(wc->entries[i]->word != NULL) {
			printf("%s:%d\n", wc->entries[i]->word, wc->entries[i]->value);
		}
	}
}

void
wc_destroy(struct wc *wc)
{
	;
	free(wc);
}

int main() {
	char* test = "aaa vvv sss aaa ddd xxx ";
	wc_init(test,10);
	return 0;
}//test function
