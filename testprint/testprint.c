/*
 * -={ MikeP }=-
 */






#include	<stdio.h>
#include	<stdlib.h>
#include	<stdint.h>
#include	"draw_lines.h"

char * tab_trailer =	"ภฤฤฤฤฤฤฤมฤฤฤฤฤฤมฤฤฤฤฤฤมฤฤฤฤฤฤมฤฤฤฤฤฤฤฤมฤฤฤฤฤฤมฤฤฤฤฤฤฤฤฤมฤฤฤฤฤฤฤฤู\n";
char * tab_caption =	"ีอออออออัออออออัออออออัออออออัออออออออัออออออัอออออออออัออออออออธ\n"
						"ณCurrentณActiveณActiveณActiveณ Active ณActiveณ Packets ณ Active ณ\n"
						"ณ round ณt/exmpณtaxes ณt/dtlsณaccountsณ NICs ณ  read   ณsessionsณ\n"
						"ฦอออออออุออออออุออออออุออออออุออออออออุออออออุอออออออออุออออออออต\n";

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
