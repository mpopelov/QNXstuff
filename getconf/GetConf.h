/*
 * (c)
 */




#ifndef _GET_CONF_H_
#define _GET_CONF_H_

extern int	GetConf_ln;


int GetConf_open( const char * ConfFileName );
void GetConf_close();
char * GetConf_token();
char * GetConf_string();


#endif /* _GET_CONF_H_ */	
