/*
 * These funcs are dealing with data filtering
 */
#include	<time.h>
#include	"qbase_filterfunc.h"


/* we'll use this structure to make the chise of propper filtering function faster */
filter_func FFUNCS[] = {
	Qbase_filter_uint,
	Qbase_filter_int,
	Qbase_filter_uint16,
	Qbase_filter_int16,
	Qbase_filter_double,
	Qbase_filter_timestamp,
	Qbase_filter_binary,
	Qbase_filter_string
};





int Qbase_filter_uint16( Qbase_filter_t * Condition, char * Row )
{
	uint16_t	Value = *(uint16_t*)(Condition->Value);
	uint16_t	Field = *((uint16_t*)((uint32_t)Condition->Field + (uint32_t)Row));
	
	if( Condition->Flags & Q_CMP_EQ )
		/* compare if the value  is equal to field */
		if( Field == Value ) return 1;
	
	if( Condition->Flags & Q_CMP_G )
		/* compare if the field is greater than value */
		if( Field > Value ) return 1;
	
	if( Condition->Flags & Q_CMP_L )
		/* compare if the field is lower than value */
		if( Field < Value ) return 1;
	
	return 0;
}



int Qbase_filter_int16( Qbase_filter_t * Condition, char * Row )
{
	int16_t	Value = *(int16_t*)(Condition->Value);
	int16_t	Field = *((int16_t*)((uint32_t)Condition->Field + (uint32_t)Row));
	
	if( Condition->Flags & Q_CMP_EQ )
		/* compare if the value  is equal to field */
		if( Field == Value ) return 1;
	
	if( Condition->Flags & Q_CMP_G )
		/* compare if the field is greater than value */
		if( Field > Value ) return 1;
	
	if( Condition->Flags & Q_CMP_L )
		/* compare if the field is lower than value */
		if( Field < Value ) return 1;
	
	return 0;
}



int Qbase_filter_uint( Qbase_filter_t * Condition, char * Row )
{
	uint32_t	Value = *(uint32_t*)(Condition->Value);
	uint32_t	Field = *((uint32_t*)((uint32_t)Condition->Field + (uint32_t)Row));
	
	if( Condition->Flags & Q_CMP_EQ )
		/* compare if the value  is equal to field */
		if( Field == Value ) return 1;
	
	if( Condition->Flags & Q_CMP_G )
		/* compare if the field is greater than value */
		if( Field > Value ) return 1;
	
	if( Condition->Flags & Q_CMP_L )
		/* compare if the field is lower than value */
		if( Field < Value ) return 1;
	
	return 0;
}



int Qbase_filter_int( Qbase_filter_t * Condition, char * Row )
{
	int		Value = *(int*)(Condition->Value);
	int		Field = *(int*)((uint32_t)Condition->Field + (uint32_t)Row);
	
	if( Condition->Flags & Q_CMP_EQ )
		/* compare if the value  is equal to field */
		if( Field == Value ) return 1;
	
	if( Condition->Flags & Q_CMP_G )
		/* compare if the field is greater than value */
		if( Field > Value ) return 1;
	
	if( Condition->Flags & Q_CMP_L )
		/* compare if the field is lower than value */
		if( Field < Value ) return 1;
	
	return 0;
}



int Qbase_filter_double( Qbase_filter_t * Condition, char * Row )
{
	double	Value = *(double *)(Condition->Value);
	double	Field = *(double *)((uint32_t)Condition->Field + (uint32_t)Row);
	
	if( Condition->Flags & Q_CMP_EQ )
		/* compare if the value  is equal to field */
		if( Field == Value ) return 1;
	
	if( Condition->Flags & Q_CMP_G )
		/* compare if the field is greater than value */
		if( Field > Value ) return 1;
	
	if( Condition->Flags & Q_CMP_L )
		/* compare if the field is lower than value */
		if( Field < Value ) return 1;
	
	return 0;
}



int Qbase_filter_timestamp( Qbase_filter_t * Condition, char * Row )
{
	struct timespec * Value = (struct timespec *)(Condition->Value);
	struct timespec * Field = (struct timespec *)((uint32_t)Condition->Field + (uint32_t)Row);
	
	if( Condition->Flags & Q_CMP_EQ )
		/* compare if the value  is equal to field */
		if( Field->tv_sec == Value->tv_sec ) return 1;
	
	if( Condition->Flags & Q_CMP_G )
		/* compare if the field is greater than value */
		if( Field->tv_sec > Value->tv_sec ) return 1;
	
	if( Condition->Flags & Q_CMP_L )
		/* compare if the field is lower than value */
		if( Field->tv_sec < Value->tv_sec ) return 1;
	
	return 0;
}



int Qbase_filter_binary( Qbase_filter_t * Condition, char * Row )
{
	uint32_t	RFSize = Condition->FSize;
	uint32_t	RVSize = Condition->VSize;
	char *		Field = (char *)((uint32_t)Condition->Field + (uint32_t)Row);
	
	/* we assume that stored binary data should be compared to data of the same length */
	if( RFSize != RVSize ) return 0;
	
	if( Condition->Flags & Q_CMP_EQ )
		/* the only type of comparison of binary data is
		 * comparison of its equality */
		if( 1 == memcmp(Field,Condition->Value,RFSize) ) return 1;
	
	return 0;
}



int Qbase_filter_string( Qbase_filter_t * Condition, char * Row )
{
	uint32_t	RFSize = Condition->FSize;
	uint32_t	RVSize = Condition->VSize;
	char *	Value = (char *)(Condition->Value);
	char *	Field = (char *)((uint32_t)Condition->Field + (uint32_t)Row);
	
	if( Condition->Flags & Q_CMP_EQ ){
		/* compare if the value  is equal to field */
		if( RFSize==RVSize ){
			if( 0 == memcmp(Field,Value,RFSize) ) return 1;
		}
	}
	if( Condition->Flags & Q_CMP_CEQ ){
		/* compare if the value  is equal to field */
		if( RFSize==RVSize ){
			if( 0 == memicmp(Field,Value,RFSize) ) return 1;
		}
	}
	
	RFSize = min( RFSize, RVSize );
	
	if( Condition->Flags & Q_CMP_G )
		/* compare if the field is greater than value */
		/* Field > Value */
		if( 0 < memcmp(Field,Value,RFSize) ) return 1;
	if( Condition->Flags & Q_CMP_CG )
		/* compare if the field is greater than value */
		/* Field > Value */
		if( 0 < memicmp(Field,Value,RFSize) ) return 1;
	
	if( Condition->Flags & Q_CMP_L )
		/* compare if the field is greater than value */
		/* Field < Value */
		if( 0 > memcmp(Field,Value,RFSize) ) return 1;
	if( Condition->Flags & Q_CMP_CL )
		/* compare if the field is greater than value */
		/* Field < Value */
		if( 0 < memicmp(Field,Value,RFSize) ) return 1;
	
	return 0;
}
