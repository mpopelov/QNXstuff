/*
 * -={ MikeP }=-
 */






#include	<stdio.h>
#include	<stdlib.h>
#include	<stdint.h>
#include	"draw_lines.h"

char * tab_trailer =	"읕컴컴컴좔컴컴컨컴컴컴좔컴컴컨컴컴컴컴좔컴컴컨컴컴컴컴컨컴컴컴컴�\n";
char * tab_caption =	"郞袴袴袴佶袴袴錮袴袴袴佶袴袴錮袴袴袴袴佶袴袴錮袴袴袴袴錮袴袴袴袴�\n"
						"쿎urrent쿌ctive쿌ctive쿌ctive� Active 쿌ctive� Packets � Active �\n"
						"� round 퀃/exmp퀃axes 퀃/dtls쿪ccounts� NICs �  read   퀂essions�\n"
						"팠袴袴袴妄袴袴曲袴袴袴妄袴袴曲袴袴袴袴妄袴袴曲袴袴袴袴曲袴袴袴袴�\n";

int main(){
	uint8_t c = 0;
	
	/* By default GR maps to G2 and
	 * G2 is selected as ISO-Latin1. Change G2 to ASCII */
/*	printf("\E)U~"); /* set G2 to PC font and then map GR to G2 permamnently */
/*	printf("%c%c%c%c",0x1e,0x);*/
	printf("\E[11m");
	fflush(stdout);
	
	/* Try to print header */
	printf(tab_caption);
	
	for( c='0'; c<128; c++ ){
		/* Print first line */
		printf(tab_trailer);
		fflush(stdout);
		sleep(1);
	}
	
	return 1;
}
