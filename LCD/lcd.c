#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <errno.h>

int terminal;



int LCD_open( char * path ){
	
	struct termios pLCD;
	
	/* open serial port for writing */
	terminal = open( path, O_RDWR );
	
	/* set it's baud rate and disable
	 * HW flow control */
	tcgetattr( terminal, &pLCD );
	cfmakeraw( &pLCD );
	cfsetospeed( &pLCD, B19200 );
	pLCD.c_lflag |= IEXTEN;
	if( pLCD.c_cflag & OHFLOW ) pLCD.c_cflag ^= OHFLOW; /* disable HW output control */
	if( pLCD.c_cflag & IHFLOW ) pLCD.c_cflag ^= IHFLOW; /* disable HW input control  */
	return tcsetattr( terminal, TCSANOW, &pLCD );
}

int LCD_close(){
	return close( terminal );
}
	
int LCD_clear(){
	char CLRSCR = 27;
	return write( terminal, &CLRSCR, 1 );
}


int LCD_printf( char * p_str ){
	int res;
	int p_str_len = strlen( p_str );
	write( terminal, p_str, p_str_len );
	res = tcdrain( terminal );
	if( res == EINTR )
		printf("TCDRAIN: leaved on signal\n");
	if( res == ENOSYS )
		printf("TCDRAIN: function not supported\n");
}



/* writing to LCD through serial port */
int main(){
	
	LCD_open("/dev/ser2");
	LCD_clear();
	getchar();
	LCD_printf("Some strange text\nAnd one more line");
	/*LCD_printf("One more line to print!!!");*/
	LCD_close();
	return 0;
}
