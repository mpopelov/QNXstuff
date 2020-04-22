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
#define GR_SBSB 51 /* ³  */
#define GR_SBSS 52 /*  ´ */
#define GR_SBSD 53 /* µ  */
#define GR_DBDS 54 /*  ¶ */
#define GR_BBDS 55 /* ·  */
#define GR_BBSD 56 /*  ¸ */
#define GR_DBDD 57 /* ¹  */
#define GR_DBDB 58 /*  º */
#define GR_BBDD 59 /* »  */
#define GR_DBBD 60 /*  ¼ */
#define GR_DBBS 61 /* ½  */
#define GR_SBBD 62 /*  ¾ */
#define GR_BBSS 63 /* ¿  */
#define GR_SSBB 64 /*  À */
#define GR_SSBS 65 /* Á  */
#define GR_BSSS 66 /*  Â */
#define GR_SSSB 67 /* Ã  */
#define GR_BSBS 68 /*  Ä */
#define GR_SSSS 69 /* Å  */
#define GR_SDSB 70 /*  Æ */
#define GR_DSDB 71 /* Ç  */
#define GR_DDBB 72 /*  È */
#define GR_BDDB 73 /* É  */
#define GR_DDBD 74 /*  Ê */
#define GR_BDDD 75 /* Ë  */
#define GR_DDDB 76 /*  Ì */
#define GR_BDBD 77 /* Í  */
#define GR_DDDD 78 /*  Î */
#define GR_SDBD 79 /* Ï  */
#define GR_DSBS 80 /*  Ð */
#define GR_BDSD 81 /* Ñ  */
#define GR_BSDS 82 /*  Ò */
#define GR_DSBB 83 /* Ó  */
#define GR_SDBB 84 /*  Ô */
#define GR_BDSB 85 /* Õ  */
#define GR_BSDB 86 /*  Ö */
#define GR_DSDS 87 /* ×  */
#define GR_SDSD 88 /*  Ø */
#define GR_SBBS 89 /* Ù  */
#define GR_BSSB 90 /*  Ú */







#endif /* _DRAW_LINES_H_ */
