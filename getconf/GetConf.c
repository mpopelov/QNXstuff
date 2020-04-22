/*
 * Copyright (c) 2004, -=MikeP=-
 * 
 * Think on license, under which to release the source code
 */


#include	<stdio.h>


#define		ConfStringLen	4096


int	GetConf_ln = 0;


static FILE *	ConfFile;
static char *	ConfString;
static char		ConfComment = '#';
static int		ConfToken = 0;


/*
 * Init the session for reading configuration
 */
int GetConf_open( const char * ConfFileName )
{
	ConfFile = fopen( ConfFileName, "r" );
	if(ConfFile == NULL) return 0;
	
	ConfString = (char *) malloc( ConfStringLen );
	ConfToken = 0;
	GetConf_ln = 0;
	return 1;
}


/*
 * Close all data for session
 */
void GetConf_close()
{
	free( ConfString );
	fclose( ConfFile );
	ConfToken = 0;
}



/*
 * Get the next token from string
 * 
 * Returns:
 * 
 *	- pointer to the token, if possible
 *	- NULL if the EOL reached or ConfComment character met
 *         as the first character of token
 */
char * GetConf_token()
{
	int i;

	/* strip all preceeding spaces and tabs */
	for(ConfToken; ConfToken<ConfStringLen-1; ConfToken++){
		if((ConfString[ConfToken] != ' ') && (ConfString[ConfToken] != 9)) break;
	}
	
	/* see if the string is not empty */
	i = ConfToken;
	while( (ConfString[i] != 0) && (ConfString[i] != '\n') && (ConfString[i] != ' ') && (ConfString[i] != 9)) i++;
	if(i == ConfToken ) return NULL;
	
	/* see if the rest of line is not a comment */
	if(ConfString[ConfToken] == ConfComment) return NULL;
	
	/* now find the end of the token */
	i = ConfToken;
	for(ConfToken; ConfToken<ConfStringLen-1; ConfToken++){
		if((ConfString[ConfToken] == ' ') || (ConfString[ConfToken] == 9) || (ConfString[ConfToken] == 0) || (ConfString[ConfToken] == '\n')) break;
	}
	
	ConfString[ConfToken] = 0;
	ConfToken++;
	return &ConfString[i];
}



/*
 * Get one string from file
 * 
 * Returns:
 * 
 * 	pointer to first token - if there is some conf data
 * 	NULL - if read out of ConfFile file
 * 
 *	if there is line - that starts with the ConfComment character
 *	this line is skipped and the next one is tried.
 */
char * GetConf_string()
{
	char *	result;

	while( 1 ){
		GetConf_ln++;
		
		result = fgets( ConfString, ConfStringLen, ConfFile );
		if(result == NULL){
			return (NULL);
		}
		
		ConfToken = 0;
		result = GetConf_token();
		if( result != NULL ) return result;
	}
}
