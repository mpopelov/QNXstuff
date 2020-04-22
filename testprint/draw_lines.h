/*
 * 2006 (c) -={ MikeP }=-
 */

#ifndef _DRAW_LINES_H_
#define _DRAW_LINES_H_


/* This turns line drawing mode on and off */
#define GR_ON	"\E[12m"
#define GR_OFF	"\E[10m"





/*
 * This defines the drawing line primitives.
 * The form of the macros is the following:
 * GR_trbl
 * Imagine the following figure:
 * 
 * 									|
 * 								  - + -
 * 									|
 * 
 * t, r, b, l specify the presence of the corresponding line
 * in the cross above. the line may be [B]lank, [S]ingle, [D]ouble.
 */
#define GR_SBSB 51 /* �  */
#define GR_SBSS 52 /*  � */
#define GR_SBSD 53 /* �  */
#define GR_DBDS 54 /*  � */
#define GR_BBDS 55 /* �  */
#define GR_BBSD 56 /*  � */
#define GR_DBDD 57 /* �  */
#define GR_DBDB 58 /*  � */
#define GR_BBDD 59 /* �  */
#define GR_DBBD 60 /*  � */
#define GR_DBBS 61 /* �  */
#define GR_SBBD 62 /*  � */
#define GR_BBSS 63 /* �  */
#define GR_SSBB 64 /*  � */
#define GR_SSBS 65 /* �  */
#define GR_BSSS 66 /*  � */
#define GR_SSSB 67 /* �  */
#define GR_BSBS 68 /*  � */
#define GR_SSSS 69 /* �  */
#define GR_SDSB 70 /*  � */
#define GR_DSDB 71 /* �  */
#define GR_DDBB 72 /*  � */
#define GR_BDDB 73 /* �  */
#define GR_DDBD 74 /*  � */
#define GR_BDDD 75 /* �  */
#define GR_DDDB 76 /*  � */
#define GR_BDBD 77 /* �  */
#define GR_DDDD 78 /*  � */
#define GR_SDBD 79 /* �  */
#define GR_DSBS 80 /*  � */
#define GR_BDSD 81 /* �  */
#define GR_BSDS 82 /*  � */
#define GR_DSBB 83 /* �  */
#define GR_SDBB 84 /*  � */
#define GR_BDSB 85 /* �  */
#define GR_BSDB 86 /*  � */
#define GR_DSDS 87 /* �  */
#define GR_SDSD 88 /*  � */
#define GR_SBBS 89 /* �  */
#define GR_BSSB 90 /*  � */







#endif /* _DRAW_LINES_H_ */
