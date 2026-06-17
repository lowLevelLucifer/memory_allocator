#include <unistd.h>
#include <stdio.h>

typedef struct block_meta{
	size_t size; 
	struct block_meta *next; 
	int free;
} block_meta;

block_meta *head = NULL; 

void *my_malloc(size_t size){
	block_meta *current = head; 
	while(current){
		if(current->free && current->size >= size){
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
	char *a = my_malloc(8); 
       	a[0] = 'A'; 
	a[1] = '\0'; 
	printf("a = %p, content =%s\n", a,a); 

	char *b = my_malloc(16); 
	b[0] = 'B'; 
	b[1] = '\0'; 
	printf("b = %p, content =%s\n",b,b); 
	
	char *c = my_malloc(32); 
	c[0] = 'C'; 
	c[1] = '\0';
	printf("c = %p, content =%s\n",c,c); 

	block_meta *current = head; 
	int i =0; 
	while(current){
		printf("Block %d: addr=%p size=%zu free=%d\n", i,current,current->size, current->free); 
		current = current->next;
		i++;
	}
	my_free(b);
	printf("b = %p , content =%s\n",b,b);
	my_free(a); 
	my_free(c);
	char* d = my_malloc(40);
	printf("d = %p\n",d);
	return 0;
}
