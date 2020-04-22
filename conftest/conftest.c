
#include	<stdio.h>
#include	"GetConf.h"

#define		Config "test.cnf"

int main()
{
	char * token;
	
	printf("Testing the job of the conf file reader\n");
	printf("Test file is: %s\n", Config);
	
	/* open the conf file */
	if( GetConf_open( Config ) == 0 ){
		printf("Can't open conf file.\n");
		return 0;
	}
	
	while( (token = GetConf_string()) != NULL ){
		printf("The config parameter was: %s\n", token);
		printf("It's values are:\n");
		while( (token = GetConf_token()) != NULL )
			printf("Value: %s\n", token);
		
		printf("\n");
	}
	
	GetConf_close();
	
	return 0;
}
