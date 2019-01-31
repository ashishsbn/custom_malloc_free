#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
	int *ptr = NULL;
	//ptr = malloc(10);
	ptr = mymalloc(1);
	if (ptr != NULL) {
		printf("My malloc: %p\n", ptr);
	}
	/*void *p = sbrk (100);
if (p == (void *) -1) {
        perror("sbrk");
        return -1;
}
perror("sbrk");
*/
	myfree(ptr);
	return 0;
}
