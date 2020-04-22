/*
 * 2006 (c) -={ MikeP }=-
 * 
 * A small and fast scalable http server
 */



#include	"qxh_common.h"
#include	"GetConf.h"





/*
 * Configuring our server goes here.
 */
int qx_httpd_configure( char *szFName ){
	
	char *szToken = NULL;
	
	/*if( ! GetConfOpen(szFName) ){
		printf("Error opening config\n");
		return 0;
	}*/
	
	return 1;
}






/*
 * Main server loop. We start pool of processors here, bind to
 * address, start listening and accepting connections.
 */

int main(){
	
	printf("QX-httpd server is starting...\n");
	
	/* read configuration options from file */
	
	return 0;
}
