#include <stdio.h>
#include <errno.h>
#include <pthread.h>


FILE *	efd;



/* this is the thread */
void * thread(void* prm){
	int ret = 0;
	
	while(1){
		ret = fprintf(stderr,"this is thread\n");
		if(ret<0){
			printf("Thread print error: %s",strerror(errno));
		}else{
			printf("mamba feed with %d bytes\n",ret);
		}
		/*ret = fflush(stderr);
		if(ret<0) printf("fflush error\n");*/
		sleep(3);
	}
	
	return NULL;
}
















int main(){
	
	int ret = 0;
	
	/* reopen stderr */
	efd = freopen("mamba","a",stderr);
	if(efd == NULL){
		printf("Error: %s\n",strerror(errno));
		return 1;
	}
	setvbuf(stderr,NULL,_IOLBF,0);
	
	pthread_create(NULL,NULL,thread,NULL);
	
	while(1){
		ret = fprintf(stderr,"that works\n");
		if(ret<0){
			printf("main print error: %s",strerror(errno));
		}else{
			printf("mamba feed with %d bytes from main\n",ret);
		}
		sleep(7);
	}
	
	return 0;
}
