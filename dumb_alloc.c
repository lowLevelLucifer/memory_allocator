#include <unistd.h>
#include <stddef.h>
#include <sys/mman.h>
#include <pthread.h>

void *malloc(size_t size); 
void free(void *ptr);
void *calloc(size_t nmemb, size_t size); 
void *realloc(void *ptr, size_t size);

typedef struct block_meta{
	size_t size; 
	struct block_meta *next; 
	int free;
} block_meta;

block_meta *head = NULL; 
void split_block(block_meta *block,size_t size) {
	block_meta *new_block = (block_meta *)((char *)(block + 1) + size); 
	new_block->size = block->size - size - sizeof(block_meta); 
	new_block->free = 1; 
	new_block->next = block->next; 

	block->size = size; 
	block->next = new_block;
}

static pthread_mutex_t alloc_lock = PTHREAD_MUTEX_INITIALIZER;

void *malloc(size_t size){
	if(size == 0) return NULL; 
	pthread_mutex_lock(&alloc_lock);
	write(2, "M", 1);
	block_meta *current = head; 
	while(current){
		if(current->free && current->size >= size){
			if(current->size >= sizeof(block_meta) && current->size - sizeof(block_meta) >= size){
				split_block(current , size);
			}
			current->free = 0; 
			pthread_mutex_unlock(&alloc_lock);
			return (void*)(current + 1); 
		}
		current = current->next;
	}
	block_meta *block = mmap(NULL, sizeof(block_meta) + size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1,0);
	if(block == MAP_FAILED){
		pthread_mutex_unlock(&alloc_lock);
		return NULL;
	}
	block->size = size; 
	block->free = 0; 
	block->next = NULL; 
	
	if(head == NULL){
		head = block; 
	}else{
		block_meta *current = head; 
		while(current->next != NULL){
			current = current->next;
		}
		current->next = block;
	}
	pthread_mutex_unlock(&alloc_lock);
	return (void*)(block+1);
}

void *calloc(size_t nmemb, size_t size){
	void *ptr = malloc(nmemb * size);
	if(ptr){
		char *p =ptr;
		size_t total =nmemb * size;
		while(total--) *p++ =0;
	}
	return ptr;
}

void *realloc(void *ptr, size_t size){
	if(!ptr) return malloc(size);
	if(size == 0){
		free(ptr);
		return NULL;
	}
	block_meta *block = (block_meta *)ptr - 1;
	if(block->size >= size) return ptr;

	void *new_ptr = malloc(size);
	if(!new_ptr) return NULL;

	char *src = ptr;
	char *dst = new_ptr;
	size_t copy_size = block->size;
	while(copy_size--) *dst++ = *src++;

	free(ptr);
	return new_ptr;
}

void free(void *ptr){
	if(!ptr) return; 
	pthread_mutex_lock(&alloc_lock);
	block_meta *block = (block_meta *)ptr -1;
	block->free = 1;
	if(block->next && block->next->free){
		block->size = block->size + sizeof(block_meta) + block->next->size; 
		block->next = block->next->next;
	}
	pthread_mutex_unlock(&alloc_lock);
}

#ifdef TESTING
#include <stdio.h>
int main(){
	char *a = malloc(8);
	char *b = malloc(32);
	block_meta *cur = head; 
	int i =0; 

	printf("------before coalescing------\n");
	while(cur){
		printf("Block %d: addr = %p , size = %zu , free = %d\n",i , cur , cur->size, cur->free);
		cur = cur->next; i++;
	}

	free(b);
	free(a);

	printf("------after coalescing------\n");
	cur = head; 
	i = 0; 
	while(cur){
		printf("Block %d: addr = %p , size = %zu , free = %d\n",i , cur , cur->size, cur->free);
		cur = cur->next; i++;
	}

	return 0;
}
#endif
