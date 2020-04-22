/*
 * -=MikeP=- (c) 2004
 */

/*
 * Here follow types and common definitions
 */


/*
 * Here are definitions common to client and server
 */
#ifndef _QBASE_COMMON_H_
#define _QBASE_COMMON_H_

/* Path to our database manager */
#define QBASE_RM_PATH	"/dev/Qbase"
#define QBASE_DB_PATH	"/tmp"			/* path, where databases are stored */


/* definitions of some types */

#define	QB_result_t		uint32_t
#define	QB_connid_t		int




#define QBASE_TAB_NAMELEN	32
#define	QBASE_MSG_LENGTH	2048



/* 
 * common data types, used in tables
 */
#define QBASE_TYPE_BASE				0x00000000

#define QBASE_TYPE_UINT				0x00000000
#define QBASE_TYPE_INT				0x00000001
#define QBASE_TYPE_UINT16			0x00000002
#define QBASE_TYPE_INT16			0x00000003
#define QBASE_TYPE_DOUBLE			0x00000004
#define	QBASE_TYPE_TIMESTAMP		0x00000005
#define QBASE_TYPE_BINARY			0x00000006
#define	QBASE_TYPE_STRING			0x00000007

#define QBASE_TYPE_LAST_EFFECTIVE	0x00000007
#define QBASE_TYPE_VARLEN			0x00000006



/* 
 * recognized commands
 */
/* Forward definitions of used functions */
#define QBASE_CMD_BASE		    0

#define QBASE_CMD_TBLCREATE		__DIOTF(_DCMD_MISC,  QBASE_CMD_BASE + 0, int)	/* Create table with specific parameters   */
#define QBASE_CMD_TBLSELECT		__DIOTF(_DCMD_MISC,  QBASE_CMD_BASE + 1, int)	/* Select table with the specific name     */
#define QBASE_CMD_TBLDELETE		__DIOTF(_DCMD_MISC,  QBASE_CMD_BASE + 2, int)	/* Delete table with specified name        */
#define QBASE_CMD_TBLFLUSH		__DIOTF(_DCMD_MISC,  QBASE_CMD_BASE + 3, int)	/* Flush table data to disk                */
#define QBASE_CMD_TBLCAPSLEN	__DIOF(_DCMD_MISC,  QBASE_CMD_BASE + 4, int)	/* Get caption of currently selected table */
#define QBASE_CMD_TBLCAPS		__DIOF(_DCMD_MISC,  QBASE_CMD_BASE + 5, int)	/* Get caption of currently selected table */

#define QBASE_CMD_FLTRDEFINE	__DIOTF(_DCMD_MISC,  QBASE_CMD_BASE + 6, int)	/* Add the rule to filter data in table    */
#define QBASE_CMD_FLTRDELETE	__DIOTF(_DCMD_MISC,  QBASE_CMD_BASE + 7, int)	/* Delete all currently defined filters    */

#define QBASE_CMD_UPDATE		__DIOTF(_DCMD_MISC,  QBASE_CMD_BASE + 8, int)	/* updating data in the table              */
#define QBASE_CMD_UPDCUR		__DIOTF(_DCMD_MISC,  QBASE_CMD_BASE + 9, int)	/* update only current row of data         */
#define QBASE_CMD_DELETE		__DIOTF(_DCMD_MISC,  QBASE_CMD_BASE + 10, int)	/* deleting data from the table            */
#define QBASE_CMD_FETCHRST		__DIOTF(_DCMD_MISC,  QBASE_CMD_BASE + 11, int)	/* reset table to search from the beginning*/

#define QBASE_CMD_ERROR			__DIOF(_DCMD_MISC,  QBASE_CMD_BASE + 12, int)	/* read the result code of last operation  */
#define QBASE_CMD_INFO			__DIOF(_DCMD_MISC,  QBASE_CMD_BASE + 13, int)	/* read the info about server */
#define QBASE_CMD_SEEK			__DIOTF(_DCMD_MISC,  QBASE_CMD_BASE + 14, int)	/* try to locate the first appearance of supplied data */
#define	QBASE_CMD_FLTRPOP		__DIOF(_DCMD_MISC,	QBASE_CMD_BASE + 15, int)	/* pop the last filter rule from rules stack */



/*
 * recognized "UPDATE" commands
 */
#define	QBASE_U_NONE	0
#define QBASE_U_SET		1
#define QBASE_U_ADD		2
#define	QBASE_U_SUB		3
#define	QBASE_U_MUL		4
#define	QBASE_U_DIV		5





/*
 * descriptor of the field in table
 */
typedef struct Qbase_field_desc{
	
	uint32_t			Type;		/* type of data in the field */
	uint32_t			Length;		/* the length of field in bytes */
	uint32_t			Offset;		/* offset to field data from the beginning of data row */
	
} Qbase_field_desc_t;



/*
 * descriptor of table itself
 */
typedef struct Qbase_table_desc{
	
	uint32_t			Version;	/* the version of table format */
	uint32_t			Size;		/* the size of table caption */
	char				Name[QBASE_TAB_NAMELEN];	/* the name of the table */
	uint32_t			NameTerm;	/* should always be 0 to enable string functions on Name member */
	uint32_t			FieldNum;	/* number of fields in the table */
	uint32_t			RowLen;		/* the length of one row of table data (in bytes) */
	uint32_t			RefCounter;	/* number of clients, currently using table */
	uint32_t			Index;		/* current value of autoincremented index for the primary key field */
	uint32_t			PageRows;	/* when allocating page - how many records page may contain */
	uint32_t			LockPages;	/* How many pages server may lock in memory simultaneously */
	Qbase_field_desc_t	Fields[1];	/* fiels descriptors, explicitly following table descriptor */
} Qbase_table_desc_t;


typedef struct Qbase_SrvInfo{
	uint32_t	Version;
	uint32_t	MsgLen;
	uint32_t	TblNameLen;
	uint32_t	StartTime;
} Qbase_SrvInfo_t;

#endif /* _QBASE_COMMON_H_ */
