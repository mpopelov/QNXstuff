/*
 * 2005 (c) -={ MikeP }=-
 */


/*
 * Everything that concerns database structure and
 * table fiels comes here.
 */





#ifndef	_QXL_DB_H_
#define	_QXL_DB_H_



/* "tax_exempt" table */
#define	QXL_TAX_EXEMPT_N		0
#define	QXL_TAX_EXEMPT_DLC		1
#define	QXL_TAX_EXEMPT_IP_MASK	2
#define	QXL_TAX_EXEMPT_IP_ADDR	3
#define	QXL_TAX_EXEMPT_PROTO	4
#define	QXL_TAX_EXEMPT_PORT_LO	5
#define	QXL_TAX_EXEMPT_PORT_HI	6
#define	QXL_TAX_EXEMPT_ACTIVE	7


/* "taxes" table */
#define	QXL_TAXES_N				0
#define	QXL_TAXES_ACTIVE		1
#define	QXL_TAXES_DEF_SUM_IN	2
#define	QXL_TAXES_DEF_SUM_OUT	3
#define	QXL_TAXES_NAME			4
#define	QXL_TAXES_MONTAX		5


/* "tax_details" table */
#define	QXL_TAXDETAILS_N		0
#define	QXL_TAXDETAILS_TAX		1
#define	QXL_TAXDETAILS_ACTIVE	2
#define	QXL_TAXDETAILS_IP_MASK	3
#define	QXL_TAXDETAILS_IP_ADDR	4
#define	QXL_TAXDETAILS_PROTO	5
#define	QXL_TAXDETAILS_PORT		6
#define	QXL_TAXDETAILS_TAXTOCLI	7
#define	QXL_TAXDETAILS_TAXFMCLI	8
#define	QXL_TAXDETAILS_TIMES	9

/* 
 * "tax_imes" table
 * 
 * describes taxes based on day time.
 * each of 24 hours has its own entry
 * in the table.
 * 
 * Taxes for incoming traffic start at 3rd field
 * Taxes for outgoing traffic start at 27th field
 */
#define	QXL_TAXTIMES_N			0
#define	QXL_TAXTIMES_ACTIVE		1
#define	QXL_TAXTIMES_NAME		2
#define	QXL_TAXTIMES_COSTIN		3
#define	QXL_TAXTIMES_COSTOUT	27


/* "accounts" table */
#define	QXL_ACCOUNTS_N			0
#define	QXL_ACCOUNTS_USER		1
#define	QXL_ACCOUNTS_NUMBER		2
#define	QXL_ACCOUNTS_TAX		3
#define	QXL_ACCOUNTS_SUM		4
#define	QXL_ACCOUNTS_DOPEN		5
#define	QXL_ACCOUNTS_DACT		6
#define	QXL_ACCOUNTS_DCLOSE		7
#define	QXL_ACCOUNTS_ACTIVE		8
#define	QXL_ACCOUNTS_NEGATIVE	9


/* "NICs" table */
#define	QXL_NICS_N				0
#define	QXL_NICS_USER			1
#define	QXL_NICS_ACCOUNT		2
#define	QXL_NICS_ACTIVE			3
#define	QXL_NICS_IP				4
#define	QXL_NICS_MAC			5


/* "payments" table */
#define	QXL_PAYMENTS_N			0
#define	QXL_PAYMENTS_ACC		1
#define	QXL_PAYMENTS_ADMIN		2
#define	QXL_PAYMENTS_DPAY		3
#define	QXL_PAYMENTS_SUM		4
#define	QXL_PAYMENTS_DESC		5

/* "daily_totals" table */
#define	QXL_DT_N				0
#define	QXL_DT_DATE				1
#define	QXL_DT_USER				2
#define	QXL_DT_ACCNT			3
#define	QXL_DT_NIC				4
#define	QXL_DT_SUM				5
#define	QXL_DT_BYTES_IN			6
#define	QXL_DT_BYTES_OUT		7


/* log table */
#define	QXL_LOG_N				0
#define	QXL_LOG_TIME			1
#define	QXL_LOG_NIC				2
#define	QXL_LOG_MAC				3
#define	QXL_LOG_DLC				4
#define	QXL_LOG_IPCLNT			5
#define	QXL_LOG_IPHOST			6
#define	QXL_LOG_PROTO			7
#define	QXL_LOG_PCLNT			8
#define	QXL_LOG_PHOST			9
#define	QXL_LOG_BTOCLI			10
#define	QXL_LOG_BTOHST			11
#define	QXL_LOG_STOCLI			12
#define	QXL_LOG_STOHST			13
#define	QXL_LOG_ACCPAY			14

#define	QXL_LOG_FCOUNT			14			/* number of user fields */
#define	QXL_LOG_RPP				1000		/* number of rows per page */
#define QXL_LOG_PCOUNT			5			/* pages to lock at one time */


#endif	/* _QXL_DB_H_ */
