/*
 * 2005 (c) -={ MikeP }=-
 */



#include	"qxl_common.h"
#include	"GetConf.h"
#include	"qbase_client.h"


#include	<termios.h>





/*
 * Global variables go here
 */
uint32_t	cycle = 0;

/* settings from configuration file */
int		SuspendTime		= 1000;	/* time between polling in MILLI-seconds */
char	*device			= "/dev/qxbil";
int		logpkts			= 1000;
int		nTblRows		= 1000;

int		MaxTaxes		= 10;
int		MaxTaxDetails	= 100;
int		MaxTaxTimes		= 10;
int		MaxAccounts		= 1000;
int		MaxNICs			= 1000;

double	DataUnits		= 1000000;	/* tax assumes money for this many bytes */

/* nfm-qxc connector */
int		nfm_qxc_fd;


/* database connectors */

/* tax exemptions */
QB_connid_t			T_tax_exempt;
Qbase_table_desc_t	*TC_tax_exempt;
uint32_t			tax_exempt_rowlen;
uint8_t				*tax_exempt_row;

/* taxes */
QB_connid_t			T_taxes;
Qbase_table_desc_t	*TC_taxes;
uint32_t			taxes_rowlen;
uint8_t				*taxes_row;

/* tax details */
QB_connid_t			T_tax_details;
Qbase_table_desc_t	*TC_tax_details;
uint32_t			tax_details_rowlen;
uint8_t				*tax_details_row;

/* tax details that depend on time */
QB_connid_t			T_tax_times;
Qbase_table_desc_t	*TC_tax_times;
uint32_t			tax_times_rowlen;
uint8_t				*tax_times_row;

uint32_t			*TT_uid;
double				*TT_cost_to_cli;
double				*TT_cost_from_cli;

/* user accounts */
QB_connid_t			T_accounts;
Qbase_table_desc_t	*TC_accounts;
uint32_t			accounts_rowlen;
uint8_t				*accounts_row;

uint32_t			*ACC_neg_allowed;
double				*ACC_sum;

/* used to update user accounts */
QB_connid_t			T_updacc;
char				*updacc_row;
double				*updacc_sum;

/* Networc Interface Cards */
QB_connid_t			T_NICs;
Qbase_table_desc_t	*TC_NICs;
uint32_t			NICs_rowlen;
uint8_t				*NICs_row;

/* logging table */
QB_connid_t			T_log;
Qbase_table_desc_t	*TC_log;
uint32_t			log_row_len;
uint8_t				*log_row;

time_t				log_t_time = 0;
time_t				log_time = 0;
struct tm			log_tm;
struct tm			log_t_tm;


char				log_name[QBASE_TAB_NAMELEN];

/* Pointers into row of table data */
uint32_t			*tlog_time;
uint32_t			*tlog_nic;
uint8_t				*tlog_mac;
uint16_t			*tlog_dlc;
uint32_t			*tlog_ipclnt;
uint32_t			*tlog_iphost;
uint16_t			*tlog_proto;
uint16_t			*tlog_pclnt;
uint16_t			*tlog_phost;
uint32_t			*tlog_btocli;
uint32_t			*tlog_btohst;
double				*tlog_stocli;
double				*tlog_stohst;
uint32_t			*tlog_accpay;



/* Buffers for intermediate data storage from database.
 * Only items from database that explicitly marked active are
 * stored here.
 * Lists contents are renewed every SyspendTime milliseconds.
 */
qx_log_t		*shm_logs[2];
qx_log_t		*captured_log	= NULL;	/* here we place nfm-qxc log */
qx_log_t		*sessions_log	= NULL;	/* here we store sessions */
int				num_sessions	= 0;
int				log_id			= 0;

qxl_tax_t		*crnt_taxes		= NULL;	/* taxes lists */
qxl_tax_t		*shdw_taxes		= NULL;
int				num_taxes		= 0;

qxl_tax_det_t	*crnt_td		= NULL; /* tax details */
qxl_tax_det_t	*shdw_td		= NULL;
int				num_td			= 0;

qxl_tax_time_t	*crnt_tt		= NULL;	/* floating taxes */
int				num_tt			= 0;

qxl_accnt_t		*crnt_accounts	= NULL; /* active accounts */
qxl_accnt_t		*shdw_accounts	= NULL;
int				num_accounts	= 0;

qxl_NIC_t		*crnt_NICs		= NULL; /* active NICs */
qxl_NIC_t		*shdw_NICs		= NULL;
int				num_crnt_nics	= 0;
int				num_shdw_nics	= 0;



/* here comes stats structure */
typedef struct _cur_stats{
	uint32_t	a_texmpt;
	uint32_t	a_taxes;
	uint32_t	a_tdtls;
	uint32_t	a_tgrfx;
	uint32_t	a_accnt;
	uint32_t	a_nics;
	uint32_t	a_pkts;
	uint32_t	a_sess;
} cur_stats_t;

cur_stats_t	stats = {0,0,0,0,0,0,0};





/* All we need to start the EOD procedure */
pthread_mutex_t	EOD_mutex	= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t	EOD_cond	= PTHREAD_COND_INITIALIZER;
void * EOD_func(void * arg);


















/*
 * This function parses configuration file
 */
int qxl_parse_config(){
	
	char	*token;
	
	if( GetConf_open( QX_CONF_FILE ) == 0 ) return 0;
	
	while( (token = GetConf_string()) != NULL ){
		
		/* what interface are we going to handle */
		if( !stricmp("INTERFACE",token) ){
			/* set token value */
			if( NULL != (token = GetConf_token()) ){
				printf("Configured for interface: %s\n",token);
			}
			continue;
		}
		
		/* what device are we going to create */
		if( !stricmp("DEVICE",token) ){
			/* set token value */
			if( NULL != (token = GetConf_token()) ){
				device = strdup(token);
			}
			continue;
		}
		/* how many packets we should be able to hold in logging queue */
		if( !stricmp("LOGPKTS",token) ){
			/* set token value */
			if( NULL != (token = GetConf_token()) ){
				logpkts = atoi(token);
			}
			continue;
		}
		/* how many rows to allocate for one table page */
		if( !stricmp("LOGROWS",token) ){
			/* set token value */
			if( NULL != (token = GetConf_token()) ){
				nTblRows = atoi(token);
			}
			continue;
		}
		/* how many taxes are there expected */
		if( !stricmp("MAXTAXES",token) ){
			/* set token value */
			if( NULL != (token = GetConf_token()) ){
				MaxTaxes = atoi(token);
			}
			continue;
		}
		/* how many tax detailed rules are there expected */
		if( !stricmp("MAXTAXDETAILS",token) ){
			/* set token value */
			if( NULL != (token = GetConf_token()) ){
				MaxTaxDetails = atoi(token);
			}
			continue;
		}
		/* how many floating taxes we have */
		if( !stricmp("MAXTAXTIMES",token) ){
			/* set token value */
			if( NULL != (token = GetConf_token()) ){
				MaxTaxTimes = atoi(token);
			}
			continue;
		}
		/* how many accounts are there expected */
		if( !stricmp("MAXACCOUNTS",token) ){
			/* set token value */
			if( NULL != (token = GetConf_token()) ){
				MaxAccounts = atoi(token);
			}
			continue;
		}
		/* how many NICs will pass our filter */
		if( !stricmp("MAXNICS",token) ){
			/* set token value */
			if( NULL != (token = GetConf_token()) ){
				MaxNICs = atoi(token);
			}
			continue;
		}
		/* how long should we suspend */
		if( !stricmp("SUSPENDTIME",token) ){
			/* set token value */
			if( NULL != (token = GetConf_token()) ){
				SuspendTime = atoi(token);
			}
			continue;
		}
		/* DataUnits - how many bytes cost that much, as stated in database */
		if( !stricmp("DATAUNITS",token) ){
			/* set token value */
			if( NULL != (token = GetConf_token()) ){
				DataUnits = atoi(token);
			}
			continue;
		}
		
		/* we are here because we are not interested in this setting
		 * simply continue parsing file */
	}
	
	/* end parsing settings */
	GetConf_close();
	
	/* allocate memory for taxes */
	if( MaxTaxes > 0 ){
		crnt_taxes = (qxl_tax_t*)malloc( MaxTaxes*sizeof(qxl_tax_t) );
		shdw_taxes = (qxl_tax_t*)malloc( MaxTaxes*sizeof(qxl_tax_t) );
		num_taxes = 0;
	}
	/* allocate memory for tax_details */
	if( MaxTaxDetails > 0 ){
		crnt_td = (qxl_tax_det_t*)malloc( MaxTaxDetails*sizeof(qxl_tax_det_t) );
		shdw_td = (qxl_tax_det_t*)malloc( MaxTaxDetails*sizeof(qxl_tax_det_t) );
		num_td = 0;
	}
	/* allocate memory for tax_times */
	if( MaxTaxTimes > 0 ){
		crnt_tt = (qxl_tax_time_t*)malloc( MaxTaxTimes*sizeof(qxl_tax_time_t) );
		num_tt = 0;
	}
	/* allocate memory for accounts */
	if( MaxAccounts > 0 ){
		crnt_accounts = (qxl_accnt_t*)malloc( MaxAccounts*sizeof(qxl_accnt_t) );
		shdw_accounts = (qxl_accnt_t*)malloc( MaxAccounts*sizeof(qxl_accnt_t) );
		num_accounts = 0;
	}
	/* allocate memory for NICs */
	if( MaxNICs > 0 ){
		crnt_NICs = (qxl_NIC_t*)malloc( MaxNICs*sizeof(qxl_NIC_t) );
		shdw_NICs = (qxl_NIC_t*)malloc( MaxNICs*sizeof(qxl_NIC_t) );
		num_crnt_nics = 0;
		num_shdw_nics = 0;
	}
	
	/* allocate memory for log */
	if(logpkts > 0){
		sessions_log = (qx_log_t*)malloc( logpkts*sizeof(qx_log_t) );
		num_sessions = 0;
	}
	
	return 1;
}






































int main(){
	
	QB_result_t		QXB_rslt;	/* QX-base results */
	uint32_t		ret_val;
	int				res;		/* System results */
	
	int				counter;
	
	/* temporary data used for filtering and accounting */
	uint32_t		fld_act	= 0;	/* for filtering active records */
	double			sum_act	= 0.00;
	
	printf("QX-billing v.0.4.1a started\n");
	
	/* Read configuration data */
	if( !qxl_parse_config() ) exit(-1);
	
	/* open connections to QX-base */
	if( QBASE_E_OK != (QXB_rslt = Qbase_connect( QBASE_RM_PATH, &T_tax_exempt)) ){
		printf("tax_exempt: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_connect( QBASE_RM_PATH, &T_taxes)) ){
		printf("taxes: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_connect( QBASE_RM_PATH, &T_tax_details)) ){
		printf("tax_details: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_connect( QBASE_RM_PATH, &T_tax_times)) ){
		printf("tax_times: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_connect( QBASE_RM_PATH, &T_accounts)) ){
		printf("accounts: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_connect( QBASE_RM_PATH, &T_updacc)) ){
		printf("Update accounts: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_connect( QBASE_RM_PATH, &T_NICs)) ){
		printf("NICs: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_connect( QBASE_RM_PATH, &T_log)) ){
		printf("log table: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	printf("QX-base database engine is mounted\n");
	
	
	
	/* 
	 * select tables we want to work with.
	 * set predefined filters
	 */
	
	/* Table:	tax_exempt
	 * Filter:	require field ACTIVE to be > 0
	 */
	if( QBASE_E_OK != (QXB_rslt = Qbase_table_select( T_tax_exempt, "tax_exempt" ))){
		printf("tax_exempt: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_table_caps( T_tax_exempt, &TC_tax_exempt ))){
		printf("tax_exempt: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	/* fld_act equals 0 now. */
	if( QBASE_E_OK != (QXB_rslt = Qbase_filter_push(T_tax_exempt,Q_CMP_G,QXL_TAX_EXEMPT_ACTIVE,4,&fld_act)) ){
		printf("tax_exempt: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	tax_exempt_row = Qbase_row_newbuffer( TC_tax_exempt, &tax_exempt_rowlen );
	/* set fld_act to 1 to let active records */
	fld_act = 1;
	
	/* Table:	taxes
	 * Filter:	active taxes only
	 */
	if( QBASE_E_OK != (QXB_rslt = Qbase_table_select( T_taxes, "taxes" ))){
		printf("taxes: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_table_caps( T_taxes, &TC_taxes ))){
		printf("taxes: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_filter_push(T_taxes,Q_CMP_EQ,QXL_TAXES_ACTIVE,4,&fld_act)) ){
		printf("taxes: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	taxes_row = Qbase_row_newbuffer( TC_taxes, &taxes_rowlen );
	
	/* Table:	tax_details
	 * Filter:	active details only
	 */
	if( QBASE_E_OK != (QXB_rslt = Qbase_table_select( T_tax_details, "tax_details" ))){
		printf("tax_details: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_table_caps( T_tax_details, &TC_tax_details ))){
		printf("tax_details: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_filter_push(T_tax_details,Q_CMP_EQ,QXL_TAXDETAILS_ACTIVE,4,&fld_act)) ){
		printf("tax_details: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	tax_details_row = Qbase_row_newbuffer( TC_tax_details, &tax_details_rowlen );
	
	/* Table:	tax_times
	 * Filter:	active records only
	 */
	if( QBASE_E_OK != (QXB_rslt = Qbase_table_select( T_tax_times, "tax_times" ))){
		printf("tax_times: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_table_caps( T_tax_times, &TC_tax_times ))){
		printf("tax_times: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_filter_push(T_tax_times,Q_CMP_EQ,QXL_TAXTIMES_ACTIVE,4,&fld_act)) ){
		printf("tax_times: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	tax_times_row = Qbase_row_newbuffer( TC_tax_times, &tax_times_rowlen );
	
	/* Table:	accounts
	 * Filter:	active accounts only
	 * 			account balance > 0
	 */
	if( QBASE_E_OK != (QXB_rslt = Qbase_table_select( T_accounts, "accounts" ))){
		printf("accounts: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_table_caps( T_accounts, &TC_accounts ))){
		printf("accounts: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_filter_push(T_accounts,Q_CMP_EQ,QXL_ACCOUNTS_ACTIVE,4,&fld_act)) ){
		printf("accounts: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	/* This filter was used to filter accounts with negative balances. Now negative balance is allowed
	 * or disallowed by administrator */
	/*if( QBASE_E_OK != (QXB_rslt = Qbase_filter_push(T_accounts,Q_CMP_G,QXL_ACCOUNTS_SUM,sizeof(double),&sum_act)) ){
		printf("accounts: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}*/
	accounts_row = Qbase_row_newbuffer( TC_accounts, &accounts_rowlen );
	/* Now we need this thing for faster access */
	ACC_sum	= (double*)(accounts_row + TC_accounts->Fields[QXL_ACCOUNTS_SUM].Offset);
	ACC_neg_allowed	= (uint32_t*)(accounts_row + TC_accounts->Fields[QXL_ACCOUNTS_NEGATIVE].Offset);
	
	/* Table:	accounts (USED FOR UPDATING ACCOUNTS)
	 * Filter:	set to UID of account we update
	 */
	if( QBASE_E_OK != (QXB_rslt = Qbase_table_select( T_updacc, "accounts" ))){
		printf("Update accounts: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_GetUpdateHeader( TC_accounts, &updacc_row ))){
		printf("Update accounts: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	updacc_sum = (double*)(updacc_row + TC_accounts->Fields[QXL_ACCOUNTS_SUM].Offset + TC_accounts->FieldNum + sizeof(uint32_t));
	((uint8_t*)(updacc_row + sizeof(uint32_t)))[QXL_ACCOUNTS_SUM] = QBASE_U_SUB;
	
	
	/* Table:	NICs
	 * Filter:	active network interface cards only
	 */
	if( QBASE_E_OK != (QXB_rslt = Qbase_table_select( T_NICs, "NICs" ))){
		printf("NICs: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_table_caps( T_NICs, &TC_NICs ))){
		printf("NICs: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	if( QBASE_E_OK != (QXB_rslt = Qbase_filter_push(T_NICs,Q_CMP_EQ,QXL_NICS_ACTIVE,4,&fld_act)) ){
		printf("NICs: %s\n", QBERROR[QXB_rslt]);
		exit(-1);
	}
	NICs_row = Qbase_row_newbuffer( TC_NICs, &NICs_rowlen );
	printf("Billing tables are mounted\n");
	
	
	
	/* Table:	Logging table. Name is ISO formatted date: YYYY-MM-DD
	 * Filter:	no filter at all.
	 * 
	 * We prepare to log as many rows per page as we can read through one
	 * logging period. We must ensure that if the logpkts changes during
	 * restart of logger within one day - there should be no problem with
	 * logging.
	 */
	TC_log = Qbase_new_tbldesc( QXL_LOG_FCOUNT, nTblRows, QXL_LOG_PCOUNT, "log" );
	Qbase_set_field_metrics( TC_log, QXL_LOG_TIME,   QBASE_TYPE_UINT,   0 );
	Qbase_set_field_metrics( TC_log, QXL_LOG_NIC,    QBASE_TYPE_UINT,   0 );
	Qbase_set_field_metrics( TC_log, QXL_LOG_MAC,    QBASE_TYPE_BINARY, 6 );
	Qbase_set_field_metrics( TC_log, QXL_LOG_DLC,    QBASE_TYPE_UINT16, 0 );
	Qbase_set_field_metrics( TC_log, QXL_LOG_IPCLNT, QBASE_TYPE_UINT,   0 );
	Qbase_set_field_metrics( TC_log, QXL_LOG_IPHOST, QBASE_TYPE_UINT,   0 );
	Qbase_set_field_metrics( TC_log, QXL_LOG_PROTO,  QBASE_TYPE_UINT16, 0 );
	Qbase_set_field_metrics( TC_log, QXL_LOG_PCLNT,  QBASE_TYPE_UINT16, 0 );
	Qbase_set_field_metrics( TC_log, QXL_LOG_PHOST,  QBASE_TYPE_UINT16, 0 );
	Qbase_set_field_metrics( TC_log, QXL_LOG_BTOCLI, QBASE_TYPE_UINT,   0 );
	Qbase_set_field_metrics( TC_log, QXL_LOG_BTOHST, QBASE_TYPE_UINT,   0 );
	Qbase_set_field_metrics( TC_log, QXL_LOG_STOCLI, QBASE_TYPE_DOUBLE, 0 );
	Qbase_set_field_metrics( TC_log, QXL_LOG_STOHST, QBASE_TYPE_DOUBLE, 0 );
	Qbase_set_field_metrics( TC_log, QXL_LOG_ACCPAY, QBASE_TYPE_UINT,	0 );
	
	log_row = Qbase_row_newbuffer( TC_log, &log_row_len );
	
	tlog_time	= (uint32_t*)(log_row + TC_log->Fields[QXL_LOG_TIME].Offset);
	tlog_nic	= (uint32_t*)(log_row + TC_log->Fields[QXL_LOG_NIC].Offset);
	tlog_mac	= (uint8_t*)(log_row + TC_log->Fields[QXL_LOG_MAC].Offset);
	tlog_dlc	= (uint16_t*)(log_row + TC_log->Fields[QXL_LOG_DLC].Offset);
	tlog_ipclnt	= (uint32_t*)(log_row + TC_log->Fields[QXL_LOG_IPCLNT].Offset);
	tlog_iphost	= (uint32_t*)(log_row + TC_log->Fields[QXL_LOG_IPHOST].Offset);
	tlog_proto	= (uint16_t*)(log_row + TC_log->Fields[QXL_LOG_PROTO].Offset);
	tlog_pclnt	= (uint16_t*)(log_row + TC_log->Fields[QXL_LOG_PCLNT].Offset);
	tlog_phost	= (uint16_t*)(log_row + TC_log->Fields[QXL_LOG_PHOST].Offset);
	tlog_btocli	= (uint32_t*)(log_row + TC_log->Fields[QXL_LOG_BTOCLI].Offset);
	tlog_btohst	= (uint32_t*)(log_row + TC_log->Fields[QXL_LOG_BTOHST].Offset);
	tlog_stocli = (double*)(log_row + TC_log->Fields[QXL_LOG_STOCLI].Offset);
	tlog_stohst = (double*)(log_row + TC_log->Fields[QXL_LOG_STOHST].Offset);
	tlog_accpay = (uint32_t*)(log_row + TC_log->Fields[QXL_LOG_ACCPAY].Offset);
	memset( log_name, 0, QBASE_TAB_NAMELEN );
	
	
	/* Connect to device */
	if( -1 == (nfm_qxc_fd = open(device,O_RDONLY)) ){
		printf("Can't access nfm-qxc\n");
		exit(-1);
	}
	
	/* open logging shared memory regions */
	{
		int lfd = 0;
		
		if(-1 == (lfd = shm_open("/dev/qxc_log0", O_RDONLY, 0)) ){
			printf("Error opening qxc_log0\n");
			exit(-1);
		}
		shm_logs[0] = mmap(0, logpkts*sizeof(qx_log_t), PROT_READ, MAP_SHARED,lfd,0);
		if(shm_logs[0] == MAP_FAILED){
			printf("Unable to map() qxc_log0: %s\n", strerror(errno));
			exit(-1);
		}
		
		if(-1 == (lfd = shm_open("/dev/qxc_log1", O_RDONLY, 0)) ){
			printf("Error opening qxc_log1\n");
			exit(-1);
		}
		shm_logs[1] = mmap(0, logpkts*sizeof(qx_log_t), PROT_READ, MAP_SHARED,lfd,0);
		if(shm_logs[1] == MAP_FAILED){
			printf("Unable to map() qxc_log1: %s\n", strerror(errno));
			exit(-1);
		}
	}
	
	
	
	
	/* now finally start the EOD thread */
	pthread_create( NULL, NULL, &EOD_func, NULL );
	/* before entering loop first time - set up comparative time */
	log_t_time = 0;
	localtime_r( &log_t_time, &log_t_tm );
	
	
	/* 
	 *                   * ********** *
	 *                   * DO THE JOB *
	 *                   * ********** *
	 */
	while(1){
		
		int		current_hour = 0;
		
		/*
		 * 1) read and set active tax exemptions
		 */
		Qbase_fetchrst( T_tax_exempt );
		counter = 0;
		while( QBASE_E_OK == (QXB_rslt = Qbase_fetch(T_tax_exempt,tax_exempt_row,tax_exempt_rowlen)) ){
			
			qx_rule_exempt_t cur_rule;
			
			cur_rule.active		= QXB_FV_UINT(   QXL_TAX_EXEMPT_ACTIVE, TC_tax_exempt, tax_exempt_row);
			cur_rule.dlc_proto	= QXB_FV_UINT16( QXL_TAX_EXEMPT_DLC, TC_tax_exempt, tax_exempt_row);
			cur_rule.ip_mask	= QXB_FV_UINT(   QXL_TAX_EXEMPT_IP_MASK, TC_tax_exempt, tax_exempt_row);
			cur_rule.ip_addr	= QXB_FV_UINT(   QXL_TAX_EXEMPT_IP_ADDR, TC_tax_exempt, tax_exempt_row);
			cur_rule.ip_proto	= QXB_FV_UINT16( QXL_TAX_EXEMPT_PROTO, TC_tax_exempt, tax_exempt_row);
			cur_rule.ip_port_lo	= QXB_FV_UINT16( QXL_TAX_EXEMPT_PORT_LO, TC_tax_exempt, tax_exempt_row);
			cur_rule.ip_port_hi = QXB_FV_UINT16( QXL_TAX_EXEMPT_PORT_HI, TC_tax_exempt, tax_exempt_row);
			
			res = devctl(nfm_qxc_fd, QXC_ADD_TE, &cur_rule, sizeof(cur_rule), &ret_val);
			counter++;
			
			/* DEBUG */
			/*printf("*d* TE_N: %u\n",QXB_FV_UINT( QXL_TAX_EXEMPT_N, TC_tax_exempt, tax_exempt_row) );*/
		}
		if( QXB_rslt != QBASE_E_LASTROW ){
			printf("Error reading tax_exempt\n");
		}
		stats.a_texmpt = counter;
		
		
		
		/* 2) read active taxes into plain buffer */
		Qbase_fetchrst( T_taxes );
		counter = 0;
		while( QBASE_E_OK == (QXB_rslt = Qbase_fetch(T_taxes,taxes_row,taxes_rowlen)) ){
			
			shdw_taxes[counter].UID			= QXB_FV_UINT(QXL_TAXES_N, TC_taxes, taxes_row);
			shdw_taxes[counter].def_tax_to_cli	= QXB_FV_DBL( QXL_TAXES_DEF_SUM_IN, TC_taxes, taxes_row);
			shdw_taxes[counter].def_tax_from_cli	= QXB_FV_DBL( QXL_TAXES_DEF_SUM_OUT, TC_taxes, taxes_row);
			shdw_taxes[counter].td			= NULL;
			
			/* DEBUG */
			/*printf("*d* TAX: %u\n",QXB_FV_UINT( QXL_TAXES_N, TC_taxes, taxes_row) );*/
			
			counter++;
			if( counter == MaxTaxes ){
				QXB_rslt = QBASE_E_LASTROW;
				break;
			}
		}
		if( QXB_rslt != QBASE_E_LASTROW ){
			printf("Error reading taxes\n");
		}
		num_taxes = counter;
		stats.a_taxes = counter;
		
		
		
		/* 3) read active tax details that depend on current time */
		Qbase_fetchrst( T_tax_times );
		counter = 0;
		/* now determine what hour of day is it now */
		{
			time_t		cur_loc_time;
			struct tm	cur_loc_tm;
			
			cur_loc_time = time(NULL);
			localtime_r( &cur_loc_time, &cur_loc_tm );
			current_hour = cur_loc_tm.tm_hour;
		}
		while( QBASE_E_OK == (QXB_rslt = Qbase_fetch(T_tax_times,tax_times_row,tax_times_rowlen)) ){
			
			/* now read all active floating taxes based on current_hour */
			crnt_tt[counter].UID = QXB_FV_UINT( QXL_TAXTIMES_N, TC_tax_times, tax_times_row );
			crnt_tt[counter].cur_tax_to_cli = QXB_FV_DBL( QXL_TAXTIMES_COSTIN + current_hour, TC_tax_times, tax_times_row);
			crnt_tt[counter].cur_tax_from_cli = QXB_FV_DBL(	QXL_TAXTIMES_COSTOUT + current_hour, TC_tax_times, tax_times_row);
			
			counter++;
			if( counter == MaxTaxTimes ){
				QXB_rslt = QBASE_E_LASTROW;
				break;
			}
		}
		if( QXB_rslt != QBASE_E_LASTROW ){
			printf("Error reading tax_times\n");
		}
		num_tt = counter;
		stats.a_tgrfx = counter;
		
		
		
		/* 4) read active tax details and link them to active taxes */
		Qbase_fetchrst( T_tax_details );
		counter = 0;
		while( QBASE_E_OK == (QXB_rslt = Qbase_fetch(T_tax_details,tax_details_row,tax_details_rowlen)) ){
			
			uint32_t	tax;
			uint32_t	grfx;
			int			i;
			
			/* read next tax details into buffer */
			tax								= QXB_FV_UINT(	QXL_TAXDETAILS_TAX, TC_tax_details, tax_details_row);
			grfx							= QXB_FV_UINT(	QXL_TAXDETAILS_TIMES, TC_tax_details, tax_details_row);
			shdw_td[counter].ip_mask		= QXB_FV_UINT(	QXL_TAXDETAILS_IP_MASK, TC_tax_details, tax_details_row);
			shdw_td[counter].ip_addr		= QXB_FV_UINT(	QXL_TAXDETAILS_IP_ADDR, TC_tax_details, tax_details_row);
			shdw_td[counter].ip_proto		= QXB_FV_UINT16(QXL_TAXDETAILS_PROTO, TC_tax_details, tax_details_row);
			shdw_td[counter].ip_port		= QXB_FV_UINT16(QXL_TAXDETAILS_PORT, TC_tax_details, tax_details_row);
			shdw_td[counter].tax_to_cli		= QXB_FV_DBL(	QXL_TAXDETAILS_TAXTOCLI, TC_tax_details, tax_details_row);
			shdw_td[counter].tax_from_cli	= QXB_FV_DBL(	QXL_TAXDETAILS_TAXFMCLI, TC_tax_details, tax_details_row);
			shdw_td[counter].next			= NULL;
			
			/* if current tax_details is assigned costs
			 * based on daytime - find appropriate tax_time and
			 * fix tax_to_cli/tax_from_cli values */
			for( i=0; i<num_tt; i++ ){
				if( crnt_tt[i].UID == grfx ){
					/* there is active tax grfx - fix values for tax details */
					shdw_td[counter].tax_to_cli = crnt_tt[i].cur_tax_to_cli;
					shdw_td[counter].tax_from_cli = crnt_tt[i].cur_tax_from_cli;
					break;
				}
			}
			
			/* try to link tax details with known active taxes */
			for( i=0; i<num_taxes; i++ ){
				if( shdw_taxes[i].UID == tax ){
					shdw_td[counter].next = shdw_taxes[i].td;
					shdw_taxes[i].td = &shdw_td[counter];
					tax = 0;
					
					/* DEBUG  */
					/*printf("*d* TD_N: %u\n",QXB_FV_UINT( QXL_TAXDETAILS_N, TC_tax_details, tax_details_row) );*/
					
					break;
				}
			}
			if( tax == 0 ) counter++; /* in case we found the tax in a list */
			if( counter == MaxTaxDetails ){
				QXB_rslt = QBASE_E_LASTROW;
				break;
			}
		}
		if( QXB_rslt != QBASE_E_LASTROW ){
			printf("Error reading tax_details\n");
		}
		num_td = counter;
		stats.a_tdtls = counter;
		
		
		
		
		
		/* 4) read active positive accounts into plain buffer and link them to taxes */
		Qbase_fetchrst( T_accounts );
		counter = 0;
		while( QBASE_E_OK == (QXB_rslt = Qbase_fetch(T_accounts,accounts_row,accounts_rowlen)) ){
			
			uint32_t	tax;
			int			i;
			
			/* See if this account is allowed to be active */
			if( *ACC_sum < 0.00 ){
				/* Balance is negative. If negative balance prohibited for account -
				 * proceed to next account. */
				if( *ACC_neg_allowed != 1 ) continue;
			}
			
			/* read next account into buffer */
			tax							= QXB_FV_UINT(	QXL_ACCOUNTS_TAX, TC_accounts, accounts_row);
			shdw_accounts[counter].UID	= QXB_FV_UINT(	QXL_ACCOUNTS_N, TC_accounts, accounts_row);
			shdw_accounts[counter].tax	= NULL;
			
			/* try to find appropriate tax */
			for( i=0; i<num_taxes; i++ ){
				if( shdw_taxes[i].UID == tax ){
					shdw_accounts[counter].tax = &shdw_taxes[i];
					
					/* DEBUG */
					/*printf("*d* ACC: %u\n",QXB_FV_UINT( QXL_ACCOUNTS_N, TC_accounts, accounts_row) );*/
					
					tax = 0;
					break;
				}
			}
			if( tax == 0 ) counter++; /* in case we found the tax in a list */
			if( counter == MaxAccounts ){
				QXB_rslt = QBASE_E_LASTROW;
				break;
			}
		}
		if( QXB_rslt != QBASE_E_LASTROW ){
			printf("Error reading accounts\n");
		}
		num_accounts = counter;
		stats.a_accnt = counter;
		
		
		
		
		
		/* 5) read active NICs, link them to accounts and push to nfm-qxc */
		Qbase_fetchrst( T_NICs );
		counter = 0;
		while( QBASE_E_OK == (QXB_rslt = Qbase_fetch(T_NICs,NICs_row,NICs_rowlen)) ){
			
			uint32_t	accnt;
			int			i;
			
			/* read next account into buffer */
			accnt						= QXB_FV_UINT( QXL_NICS_ACCOUNT, TC_NICs, NICs_row);
			shdw_NICs[counter].rule.UID	= shdw_NICs[counter].UID = QXB_FV_UINT( QXL_NICS_N, TC_NICs, NICs_row);
			shdw_NICs[counter].rule.ip_addr	= QXB_FV_UINT( QXL_NICS_IP, TC_NICs, NICs_row);
			memcpy( &shdw_NICs[counter].rule.mac, QXB_FV_BIN(QXL_NICS_MAC,TC_NICs,NICs_row), 6 );
			shdw_NICs[counter].account	= NULL;
			
			/* try to find appropriate account */
			for( i=0; i<num_accounts; i++ ){
				if( shdw_accounts[i].UID == accnt ){
					shdw_NICs[counter].account = &shdw_accounts[i];
					
					/* DEBUG */
					/*printf("*d* NIC: %u\n",QXB_FV_UINT( QXL_NICS_N, TC_NICs, NICs_row) );*/
					
					/* now push the rule into nfm-qxc */
					res = devctl(nfm_qxc_fd, QXC_ADD_NIC, &(shdw_NICs[counter].rule), sizeof(qx_rule_nics_t), &ret_val);
					
					accnt = 0;
					break;
				}
			}
			if( accnt == 0 ) counter++; /* in case we found the tax in a list */
			if( counter == MaxNICs ){
				QXB_rslt = QBASE_E_LASTROW;
				break;
			}
		}
		if( QXB_rslt != QBASE_E_LASTROW ){
			printf("Error reading accounts\n");
		}
		num_shdw_nics = counter;
		stats.a_nics = counter;

		
		
		
		
		
		/*
		 * At this moment all crnt_* lists contain tarification information
		 * that is currently actual for rules that nfm-qxc.so follows.
		 * All shdw_* lists contain information that will be actual for nfm-qxc
		 * after QXC_SWAP_GET_LOG.
		 * 
		 * ***************
		 * 6) kick nfm-qxc to swap buffers and reset counters
		 * - read log from nfm-qxc
		 * - swap current and shadow pointers
		 * ***************
		 */
		/*printf("resetting nfm-qxc (%u)\n", cycle);*/
		res = devctl(nfm_qxc_fd, QXC_SWAP_GET_LOG, &log_id, sizeof(log_id), &ret_val);
		{
			int				tmp_cntr	= 0;
			qxl_tax_t		*tmp_taxes	= crnt_taxes;
			qxl_tax_det_t	*tmp_td		= crnt_td;
			qxl_accnt_t		*tmp_accnt	= crnt_accounts;
			qxl_NIC_t		*tmp_NICs	= crnt_NICs;
			
			/* swap taxes lists */
			crnt_taxes		= shdw_taxes;
			shdw_taxes		= tmp_taxes;
			
			/* swap tax details lists */
			crnt_td			= shdw_td;
			shdw_td			= tmp_td;
			
			/* swap accounts lists */
			crnt_accounts	= shdw_accounts;
			shdw_accounts	= tmp_accnt;
			
			/* swap NICs lists */
			tmp_cntr		= num_crnt_nics;
			crnt_NICs		= shdw_NICs;
			shdw_NICs		= tmp_NICs;
			num_crnt_nics	= num_shdw_nics;
			num_shdw_nics	= tmp_cntr;
			
			/* get the right pointer */
			captured_log = shm_logs[log_id];
		}
		cycle++;
		
		/*
		 * Now, as we actually swapped rules within nfm-qxc, everything that
		 * was previously crnt_* now became shdw_* and is listed in the log.
		 * All that was stored in shdw_* now became active.
		 * As we swapped buffers - we are going to work with the
		 * shdw_* lists.
		 */
		/*printf("Parsing %u packets from /dev/qxc_log%i...\n", ret_val, log_id);*/
		stats.a_pkts = ret_val;
		
		
		/*
		 * parse retrieved log: form sessions out of packets log
		 */
		num_sessions = 0;
		for( counter=0; counter<ret_val; counter++ ){
			
			int i = 0;
			
			/* take each packet and try to find the same ones */
			for( i=0; i<num_sessions; i++ ){
				/* compare packets. The parts that are most likely different come first.
				 * The most common parts compared last */
				if( 0 != memcmp(captured_log[counter].mac,sessions_log[i].mac,6) ) continue;
				if( captured_log[counter].IP_host != sessions_log[i].IP_host ) continue;
/*				if( captured_log[counter].Port_clnt != sessions_log[i].Port_clnt ) continue;*/
				if( captured_log[counter].IP_clnt != sessions_log[i].IP_clnt ) continue;
				if( captured_log[counter].Port_host != sessions_log[i].Port_host ) continue;
				if( captured_log[counter].IP_proto != sessions_log[i].IP_proto ) continue;
				if( captured_log[counter].DLC_proto != sessions_log[i].DLC_proto ) continue;
				
				/* this seems to be session we are looking for */
				sessions_log[i].bytes_to_cli += captured_log[counter].bytes_to_cli;
				sessions_log[i].bytes_to_host += captured_log[counter].bytes_to_host;
				break;
			}
			
			if( i == num_sessions ){
				/* we found nothing alike. Copy entire logged packet. */
				memcpy( &sessions_log[i], &captured_log[counter], sizeof(qx_log_t) );
				num_sessions++;
			}
		}
		
		/*printf("<%u>> Logged %u packets -> formed %i sessions\n",cycle,ret_val,num_sessions);*/
		stats.a_sess = num_sessions;
		
		
		
		
		
		/* Create or select table */
		log_time = time(NULL);
		localtime_r( &log_time, &log_tm ); /* get the local time */
		
		/* we change tables only in case date has changed */
		/* we look if the day of the year has changed - we do the swap */
		/* NOTE: we start program with 1st january 1970 in the log_t_time
		 * so we must ensure YEARS differ for all cases */
		if( (log_tm.tm_yday!=log_t_tm.tm_yday) || (log_tm.tm_year!=log_t_tm.tm_year) ){
			/* we are just started or the new day happened */
			/* 1) form the name of a new table */
			strftime(log_name,QBASE_TAB_NAMELEN,"%F",&log_tm); /* table name is ISO date representation: YYYY-MM-DD */
			
			/* 2) try to select a table. If table exists - ALL OK. IF not - create it */
			QXB_rslt = Qbase_table_select( T_log, log_name );
			if( QXB_rslt != QBASE_E_OK )
				if( QXB_rslt != QBASE_E_NOTBLEXIST ){
					printf("QX-base: %s\n", QBERROR[QXB_rslt]);
					exit(-1);
				}else{
					/* set new name and create table */
					memcpy( TC_log->Name, log_name, QBASE_TAB_NAMELEN );
					TC_log->PageRows = nTblRows;
					if( QBASE_E_OK != (QXB_rslt = Qbase_table_create(T_log,TC_log)) ){
						printf("QX-base: %s\n", QBERROR[QXB_rslt]);
						exit(-1);
					}
				}
			/* Now we have a valid logging table mounted and may write safely.*/
			memcpy(&log_t_tm,&log_tm,sizeof(struct tm));
			if(log_t_time != 0){ /* -- this prevents from EOD-ing at startup */
				/* signalling EOD event processor */
				/*printf("Signalling EOD\n");*/
				pthread_mutex_lock( &EOD_mutex );
				pthread_cond_signal( &EOD_cond );
				pthread_mutex_unlock( &EOD_mutex );
			}
			log_t_time = log_time;
			cycle = 0;
		}
		
		/* Now set all values that are default for current logging period */
		*tlog_time = log_time;
		
		
		
		
		
		
		
		
		
		
		/* store sessions into database and update accounts - !!! MONEY !!! */
		{
			int			i		= 0;
			int			nic_i	= 0;
			qxl_NIC_t	*c_NIC	= NULL;
			qxl_tax_t	*c_TAX	= NULL;
			qxl_tax_det_t *c_TD	= NULL;
			
			for( i=0; i<num_sessions; i++ ){
				/* DEBUG - session details */
				/*printf("S[%i]: dlc=%hx | cli_ip=%x | hst_ip=%x | ip_p=%hx | cli_p=%hu | hst_p=%hu\n", \
				i, sessions_log[i].DLC_proto, sessions_log[i].IP_clnt, sessions_log[i].IP_host, \
				sessions_log[i].IP_proto, sessions_log[i].Port_clnt, sessions_log[i].Port_host );*/
				
				/* set fields */
				memcpy(tlog_mac, sessions_log[i].mac, 6);
				*tlog_dlc		= sessions_log[i].DLC_proto;
				*tlog_ipclnt	= sessions_log[i].IP_clnt;
				*tlog_iphost	= sessions_log[i].IP_host;
				*tlog_proto		= sessions_log[i].IP_proto;
/*				*tlog_pclnt		= sessions_log[i].Port_clnt;*/
				*tlog_phost		= sessions_log[i].Port_host;
				*tlog_btocli	= sessions_log[i].bytes_to_cli;
				*tlog_btohst	= sessions_log[i].bytes_to_host;
				
				/* Find appropriate NIC */
				for(nic_i=0; nic_i<num_shdw_nics; nic_i++){
					if( 0 == memcmp(tlog_mac,shdw_NICs[nic_i].rule.mac,6) ){
						/* Clients behind a router will have the same MAC.
						 * Need to change IP */
						if(*tlog_ipclnt == shdw_NICs[nic_i].rule.ip_addr){
							/* we found a NIC */
							c_NIC = &(shdw_NICs[nic_i]);
							break;
						}
					}
				}
				if( nic_i == num_shdw_nics ){
					printf("ERROR: session has no associated NIC!\n");
					continue;
				}
				
				/* we have NIC -> Account -> Tax. Find appropriate tax details */
				/*printf("Session of NIC: %u\n",c_NIC->UID);*/
				*tlog_nic = c_NIC->UID;
				*tlog_accpay = c_NIC->account->UID;
				c_TAX = c_NIC->account->tax;
				c_TD = c_TAX->td;
				
				while( c_TD != NULL ){
					/* we found appropriate tax detail */
					if( c_TD->ip_addr == (*tlog_iphost & c_TD->ip_mask) ){
						if( (c_TD->ip_proto == 0) || (c_TD->ip_proto == *tlog_proto) ){
							if( (c_TD->ip_port == 0) || (c_TD->ip_port == *tlog_phost)){
								/* there is specific rule for this session */
								break;
							}
						}
					}
					c_TD = c_TD->next;
				}
				
				/* how much this session costed client */
				if( c_TD != NULL ){
					/* there was detailed tax rule */
					*tlog_stocli = (c_TD->tax_to_cli   * (*tlog_btocli))/DataUnits;
					*tlog_stohst = (c_TD->tax_from_cli * (*tlog_btohst))/DataUnits;
				}else{
					/* no details - count money based on common tax values */
					*tlog_stocli = (c_TAX->def_tax_to_cli * (*tlog_btocli))/DataUnits;
					*tlog_stohst = (c_TAX->def_tax_from_cli * (*tlog_btohst))/DataUnits;
				}
				
				/* insert record */
				if( QBASE_E_OK != (QXB_rslt = Qbase_insert( T_log, log_row, log_row_len )) ){
					printf("LOG> %s\n", QBERROR[QXB_rslt]);
					exit(-1);
				}
				
				/* update account */
				*updacc_sum = (*tlog_stocli) + (*tlog_stohst);
				/*printf("They owe us %f bucks\n", *updacc_sum);*/
				if( QBASE_E_OK != (QXB_rslt = Qbase_seek(T_updacc,c_NIC->account->UID)) ){
					printf("ERROR account(%u): %s\n", c_NIC->account->UID, QBERROR[QXB_rslt]);
				}
				if( QBASE_E_OK != (QXB_rslt = Qbase_updcur(T_updacc,updacc_row)) ){
					printf("ERROR updating account(%u): %s\n", c_NIC->account->UID, QBERROR[QXB_rslt]);
				}
			}
		}
		
		/* after changing database - flush it */
		Qbase_flush(T_updacc);
		Qbase_flush(T_log);
		
		
		/* print stats */
		printf("\f|Current|Active|Active|Active|Active| Active |Active| Packets | Active |\n");
		printf(  "| round |t/exmp|t/grfx|taxes |t/dtls|accounts| NICs |  read   |sessions|\n");
		printf(  "+-------+------+------+------+------+--------+------+---------+--------+\n");
		printf(  "| %5u | %4u | %4u | %4u | %4u | %6u | %4u | %7u | %6u |\n",cycle,stats.a_texmpt,stats.a_tgrfx,
		       stats.a_taxes, stats.a_tdtls, stats.a_accnt, stats.a_nics, stats.a_pkts, stats.a_sess );
		printf(  "+----------------------------------------------------------------------+\n");
		
		{
			int str_left = 20;
			int i = 0;
			struct in_addr c_in;
			struct in_addr s_in;
			char * szProto;
			
			for(; ((i<num_sessions)&&(str_left>0)) ; i++,str_left-- ){
				
				switch (sessions_log[i].IP_proto){
					case 1:
						szProto = " ICMP ";
						break;
					case 6:
						szProto = "  TCP ";
						break;
					case 17:
						szProto = "  UDP ";
						break;
					default:
						szProto = "not IP";
				}
				
				printf("%2d: %15s -%s-> ",i,	inet_ntoa(sessions_log[i].IP_clnt),szProto);
				printf("%15s : %hu",inet_ntoa(sessions_log[i].IP_host),(sessions_log[i].Port_host) );
				if(str_left!=1) printf("\n"); /* do not terminate the last line */
			}
			
		}
		
		/* suspend for configured ammount of time */
		delay(SuspendTime);
		
		/* go on for the next loop */
	}
	
	exit(0);
}


/*EOF*/

