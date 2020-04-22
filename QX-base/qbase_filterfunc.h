/* here are definitions of filter related data and functions */
#ifndef _QBASE_FILTERFUNC_H_
#define _QBASE_FILTERFUNC_H_

#include	<stdint.h>




/* 
 * here are the types of comparisons that may be set in FLAGS 
 */
#define	Q_CMP_EQ		0x00000001	/* field is equal to value */
#define	Q_CMP_G			0x00000002	/* field is greater than value */
#define	Q_CMP_L			0x00000004	/* field is lower than value */
#define	Q_CMP_INCL		0x00000008	/* field (string) contains supplied substring */
#define	Q_CMP_CEQ		0x00000010	/* comparison for equality ignoring case */
#define	Q_CMP_CG		0x00000020	/* comparison for equality ignoring case */
#define	Q_CMP_CL		0x00000040	/* comparison for equality ignoring case */

#define	Q_CMP_STRICT	0x80000000	/* filter condition should be strict: 
                                     * the first occurance of non-matching record
                                     * forces server to respond with QBASE_E_LASTROW
                                     * to any subsequent FETCH operation */



/*
 * This defines one filter entry
 */
typedef struct Qbase_filter{
	uint32_t	Flags;		/* These flags define what comparison should be taken */
	uint32_t	FNum;		/* the number of field to apply filtering rule to */
	uint32_t	Field;		/* this is the offset to the field */
	uint32_t	FSize;		/* size of data, pointed to by Field */
	uint32_t	VSize;		/* size of data, pointed to by Value */
	int			(*cmp_func)(struct Qbase_filter * Condition, char* Row); /* pointer to the function, making comparison */
	struct Qbase_filter * Next;
	char		Value[1];		/* this is the value (in some form) explicitly following the filter structure */
} Qbase_filter_t;







/*
 * here are functions for comparison of different types
 * Prototype is:
 * 
 * int
 * Qbase_filter_TYPE(
 * 		Qbase_filter_t * 	Condition,	- filtering condition
 * 		char *				Row,		- the row, we want to apply filter to
 * );
 */
typedef int (*filter_func)(Qbase_filter_t *, char * );

int Qbase_filter_uint16( Qbase_filter_t * Condition, char * Row );
int Qbase_filter_int16( Qbase_filter_t * Condition, char * Row );
int Qbase_filter_uint( Qbase_filter_t * Condition, char * Row );
int Qbase_filter_int( Qbase_filter_t * Condition, char * Row );
int Qbase_filter_double( Qbase_filter_t * Condition, char * Row );
int Qbase_filter_timestamp( Qbase_filter_t * Condition, char * Row );
int Qbase_filter_binary( Qbase_filter_t * Condition, char * Row );
int Qbase_filter_string( Qbase_filter_t * Condition, char * Row );

extern filter_func FFUNCS[];


#endif /* _QBASE_FILTERFUNC_H_ */
