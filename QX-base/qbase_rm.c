/*
 * -=MikeP=- (c) 2004
 */


#include	"qbase_server.h"
#include	"qbase_iofunc.h"
#include	"GetConf.h"






/* globals, used to store thread pool and resource manager related data */
static resmgr_attr_t			resmgr_attr;
static resmgr_context_t *		ctp;
static resmgr_connect_funcs_t	connect_func;
static resmgr_io_funcs_t		io_func;
static iofunc_attr_t			attr;

static iofunc_funcs_t			mount_funcs = {
	_IOFUNC_NFUNCS,
	Qbase_OCB_calloc,
	Qbase_OCB_free
};
static iofunc_mount_t			mountpoint = { 0, 0, 0, 0, &mount_funcs };

static int						id;
static thread_pool_attr_t		pool_attr;
static thread_pool_t *			tpp;
static dispatch_t *				dpp;

extern Qbase_SrvInfo_t INFO;

int main()
{
	char * db_path;
	char * rm_path;
	char * token;
	
	/* 1) Make initial setup */
	INFO.StartTime = time(&(INFO.StartTime));
	
	db_path = (char*)malloc(255);
	strcpy(db_path, QBASE_RM_PATH);
	rm_path = (char*)malloc(255);
	strcpy(db_path, QBASE_DB_PATH);
	
	if( (dpp = dispatch_create()) == NULL ){
		printf("Can't create dispatch context.");
		return (EXIT_FAILURE);
	}
	
	memset( &pool_attr, 0, sizeof(pool_attr) );
	pool_attr.handle = dpp;
	pool_attr.context_alloc = resmgr_context_alloc;
	pool_attr.block_func = resmgr_block;
	pool_attr.handler_func = resmgr_handler;
	pool_attr.context_free = resmgr_context_free;
	
	pool_attr.lo_water = 2;
	pool_attr.hi_water = 5;
	pool_attr.increment = 1;
	pool_attr.maximum = 50;
	
	
	/* 2) read config file and tune database to one's desire */
	if( GetConf_open( "/etc/Qbase.cnf" ) == 0 ){
		printf("Can't open conf file.\n");
		return (EXIT_FAILURE);
	}
	
	while( (token = GetConf_string()) != NULL ){
		if( 0 == stricmp("POOL_LOWATER",token) ){
			if( NULL != (token = GetConf_token()) ){
				pool_attr.lo_water = atoi(token);
			}
			continue;
		}
		if( 0 == stricmp("POOL_HIWATER",token) ){
			if( NULL != (token = GetConf_token()) ){
				pool_attr.hi_water = atoi(token);
			}
			continue;
		}
		if( 0 == stricmp("POOL_INCREMENT",token) ){
			if( NULL != (token = GetConf_token()) ){
				pool_attr.increment = atoi(token);
			}
			continue;
		}
		if( 0 == stricmp("POOL_MAXIMUM",token) ){
			if( NULL != (token = GetConf_token()) ){
				pool_attr.maximum = atoi(token);
			}
			continue;
		}
		if( 0 == stricmp("RM_PATH",token) ){
			if( NULL != (token = GetConf_token()) ){
				strcpy(rm_path,token);
			}
			continue;
		}
		if( 0 == stricmp("DB_PATH",token) ){
			if( NULL != (token = GetConf_token()) ){
				strcpy(db_path,token);
			}
		}
	}
	GetConf_close();
	
	/* 3) now it is time to become a daemon */
	{
    pid_t pid = fork();
    if (pid < 0) {
	printf("Couldn't fork\n");
	return (-1);
    }
    if (pid != 0) return 0;
	}

    setsid();
	
	
	/* 3) change current working directory to one with DB's*/
	if( 0 != chdir(db_path) ){
		printf("Qbase: Unable to chdir() to working directory.");
		return (EXIT_FAILURE);
	}
	
	if( (tpp = thread_pool_create(&pool_attr, POOL_FLAG_EXIT_SELF)) == NULL){
		printf("Qbase: Can't create thread pool.");
		return (EXIT_FAILURE);
	}
	
	iofunc_func_init( _RESMGR_CONNECT_NFUNCS, &connect_func, _RESMGR_IO_NFUNCS, &io_func );
    io_func.read	= Qbase_read;		/* the _IO_READ handler    */
    io_func.write	= Qbase_write;	/* the _IO_WRITE handler   */
    io_func.devctl	= Qbase_devctl;	/* a handler to _IO_DEVCTL */

	iofunc_attr_init( &attr, S_IFNAM | 0777, 0, 0 );
	attr.mount = &mountpoint;
	
	/* tune resourse handling functions */
	memset( &resmgr_attr, 0, sizeof(resmgr_attr) );
	resmgr_attr.nparts_max = 1;
	resmgr_attr.msg_max_size = QBASE_MSG_LENGTH;
	
	/* register device */
	if( (id = resmgr_attach( dpp, &resmgr_attr, rm_path, _FTYPE_ANY, 0, &connect_func, &io_func, &attr )) == -1 ){
		printf("Qbase: Unable to attach resource name\n");
		return (EXIT_FAILURE);
	}
	
	/* close all inputs and outputs and free memory */
	printf("Qbase: Started on %s\n",ctime(&INFO.StartTime));
	free(db_path);
	free(rm_path);
    fclose(stdin);
    fclose(stdout);
    fclose(stderr);
	
	/* start the pool!!! and have no exit!!! - from here for sure ;0) */
	thread_pool_start( tpp );
}
