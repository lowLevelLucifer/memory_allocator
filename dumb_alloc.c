//custom_memory_allocator_using_C 

#include <unistd.h>
#include <stdio.h>

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
void *my_malloc(size_t size){
	block_meta *current = head; 
	while(current){
		if(current->free && current->size >= size){
			if(current->size >= sizeof(block_meta) && current->size - sizeof(block_meta) >= size){
				split_block(current , size);
			}
			current->free = 0; 
			return (void*)(current + 1); 
		}
		current = current->next;
	}
	block_meta *block = sbrk(sizeof(block_meta) + size); 
	if(block == (void*)-1) return NULL; 
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
	return (void*)(block+1);
}
void my_free(void *ptr){
	if(!ptr) return; 
	block_meta *block = (block_meta *)ptr -1;
	block->free = 1;
}
int main(){
	char *x = my_malloc(100);
	my_free(x); 

	block_meta *block = (block_meta *)x -1;

	block->size = (size_t)-1; 
	block->free = 1;

	printf("Block size set to: %zu\n", block->size);

	size_t request = (size_t)-1 - 10; 

	printf("Requesting size: %zu\n", request);
	void  *result = my_malloc(request); 
	printf("Result pointer: %p\n", result); 
	printf("Block size after: %zu , free= %d\n", block->size, block->free);

	return 0;
}
