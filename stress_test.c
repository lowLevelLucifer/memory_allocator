#include <stdlib.h>
#include <stdio.h>

int main(){
	void *ptrs[10000];
	int num = 0;
	for(int i = 0 ; i < 10000; i++){
		num = rand() % 1017 + 8;
		malloc(num);
		*ptrs[i] = num;
	}
	for(int i = 0 ; i < 10000; i++){
		free(*ptrs[i]);
	}
	return 0;
}
