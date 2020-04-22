/*
 * -=MikeP=- (c) 2005
 */


#include	"qavird.h"
#include	"GetConf.h"






/* globals, used to store thread pool and resource manager related data */
static resmgr_attr_t			resmgr_attr;
static resmgr_context_t *		ctp;
static resmgr_connect_funcs_t	connect_func;
static resmgr_io_funcs_t		io_func;
static iofunc_attr_t			attr;


IOFUNC_OCB_T * Qavird_OCB_calloc( resmgr_context_t * qctp, IOFUNC_ATTR_T *qattr);
void Qavird_OCB_free( IOFUNC_OCB_T * ocb );
static iofunc_funcs_t			mount_funcs = {
	_IOFUNC_NFUNCS,
	Qavird_OCB_calloc,
	Qavird_OCB_free
};
static iofunc_mount_t			mountpoint = { 0, 0, 0, 0, &mount_funcs };

static int						id;
static thread_pool_attr_t		pool_attr;
static thread_pool_t *			tpp;
static dispatch_t *				dpp;








static struct Qavird_info		state_info;
int								ret;
struct cl_node *				cl_root_node = NULL;
pthread_rwlock_t				cl_root_node_rwlock = PTHREAD_RWLOCK_INITIALIZER;

char *							AVbases = "/Qavird/AVbases";
#define QAVIRD_DEV "/dev/Qav"
#define	QAVIRD_MSG_LENGTH 1024



















/* 
 * OCB handling functions
 */

IOFUNC_OCB_T * Qavird_OCB_calloc( resmgr_context_t * qctp, IOFUNC_ATTR_T *qattr){
	struct Qavird_ocb * ocb;
	
	if( !(ocb = calloc(1, sizeof(struct Qavird_ocb))) ){
		return 0;
	}
	/* tune all of OUR OWN members */
	ocb->cl_errno = 0;
	ocb->stream = NULL;
	ocb->options = CL_SCAN_STDOPT;
	
	/* maximal number of files in archive */
	ocb->limits.maxfiles = 1000;
	/* maximal archived file size */
	ocb->limits.maxfilesize = 10 * 1048576; /* 10 MB */
	/* maximal recursion level */
	ocb->limits.maxreclevel = 5;
	/* maximal compression ratio */
	ocb->limits.maxratio = 200;
	/* disable memory limit for bzip2 scanner */
	ocb->limits.archivememlim = 0;
	
	return (ocb);
}

void Qavird_OCB_free( IOFUNC_OCB_T * ocb ){
	
	/* clean up all the resources - close database if needed */
	if( ocb == NULL ) return;
	/* release memory */
	free( ocb );
}








/*
 * devctl handling function
 */

int Qavird_devctl(resmgr_context_t *qctp, io_devctl_t *msg, RESMGR_OCB_T *ocb)
{
	int retval = 0;
	int nbytes = 0;
	char * data = _DEVCTL_DATA(msg->i);
	
	switch(msg->i.dcmd){
		
		/*############# DISPLAY INFO ON THE CURRENT ENGINE ###########*/
		case QAVIRD_CMD_INFO:
		{
			/* 1) check the size of data supplied in case of error - return error */
			if( msg->i.nbytes < sizeof(struct Qavird_info) ){
				retval = QAV_DATA;
				nbytes = 0;
				break;
			}
			
			nbytes = sizeof(struct Qavird_info);
			pthread_rwlock_rdlock( &cl_root_node_rwlock );
			memcpy( data, &state_info, nbytes);
			pthread_rwlock_unlock( &cl_root_node_rwlock );
			retval = QAV_CLEAN;
			break;
		}
		
		/*############# DISPLAY INFO ON THE CURRENT ENGINE ###########*/
		case QAVIRD_CMD_AVBASERELOAD:
		{
			struct Qavird_info	new_state_info;
			struct cl_node *	new_cl_root_node = NULL;
			struct cl_node *	old_node = cl_root_node;
			
			/* 1) load new database into separate memory */
			nbytes = 0;
			new_state_info.signum = 0;
			ocb->cl_errno = cl_loaddbdir(AVbases,&new_cl_root_node,&new_state_info.signum);
			if(ocb->cl_errno){
				printf("Error loading bases: %s\n",cl_strerror(ocb->cl_errno));
				retval = QAV_INTERNAL;
				break;
			}
			
			printf("REloaded %d signatures\n",new_state_info.signum);
			
			ocb->cl_errno = cl_build(new_cl_root_node);
			if(ocb->cl_errno){
				printf("Error loading bases: %s\n",cl_strerror(ocb->cl_errno));
				cl_free(new_cl_root_node);
				retval = QAV_INTERNAL;
				break;
			}
			
			printf("Engine rebuilt succesfully\n");
			
			
			/* parse headers of databases */
			{
				char	cvdname[256];
				struct cl_cvd * cvd_header;
				
				/* main.cvd */
				snprintf(cvdname,256,"%s/main.cvd",AVbases);
				cvd_header = cl_cvdhead(cvdname);
				if(!cvd_header){
					printf("error opening %s\n",cvdname);
					cl_free(new_cl_root_node);
					retval = QAV_INTERNAL;
					break;
				}
				new_state_info.m_ver = cvd_header->version;
				new_state_info.m_sig = cvd_header->sigs;
				new_state_info.m_fl = cvd_header->fl;
				new_state_info.m_btime = cvd_header->stime;
				cl_cvdfree(cvd_header);
				/* daily.cvd */
				snprintf(cvdname,256,"%s/daily.cvd",AVbases);
				cvd_header = cl_cvdhead(cvdname);
				if(!cvd_header){
					printf("error opening %s\n",cvdname);
					cl_free(new_cl_root_node);
					retval = QAV_INTERNAL;
					break;
				}
				new_state_info.d_ver = cvd_header->version;
				new_state_info.d_sig = cvd_header->sigs;
				new_state_info.d_fl = cvd_header->fl;
				new_state_info.d_btime = cvd_header->stime;
				cl_cvdfree(cvd_header);
			}
			
			time(&new_state_info.base_time);
			
			/* 2) switch engine to a new database */
			pthread_rwlock_wrlock( &cl_root_node_rwlock );
			{
				cl_root_node = new_cl_root_node;
				new_state_info.virfound = state_info.virfound;
				new_state_info.fdone = state_info.fdone;
				new_state_info.startup = state_info.startup;
				memcpy(&state_info,&new_state_info,sizeof(struct Qavird_info));
			}
			pthread_rwlock_unlock( &cl_root_node_rwlock );
			
			printf("Successfully switched to a reloaded database\n");
			
			/* 3) free old database */
			cl_free(old_node);
			
			retval = QAV_CLEAN;
			break;
		}
		
		
		
		
		/*############ SCAN THE FILE BASED ON THE NAME SUPPLIED #########*/
		case QAVIRD_CMD_SCANFILE:
		{
			const char * virname = NULL;
			
			/* 1) check the size of data supplied in case of error - return error */
			if( msg->i.nbytes < 3 ){
				retval = QAV_DATA;
				nbytes = 0;
				break;
			}
			
			/* scan the file */
			
			pthread_rwlock_rdlock( &cl_root_node_rwlock );
			
			ocb->cl_errno = cl_scanfile(data,&virname,NULL,cl_root_node,&ocb->limits,ocb->options);
			switch(ocb->cl_errno){
				case CL_VIRUS:
					printf("Detected %s virus.\n", virname);
					atomic_add(&state_info.virfound,1);
					retval = QAV_VIRUS;
					nbytes = snprintf(data,msg->i.nbytes,"%s",virname);
					if(nbytes < msg->i.nbytes){
						nbytes++;
					}else{
						nbytes = msg->i.nbytes;
					}
					break;
					
				case CL_CLEAN:
					printf("File clean\n");
					retval = QAV_CLEAN;
					nbytes = 0;
					break;
					
				default:
					printf("Internal error\n");
					retval = QAV_INTERNAL;
					nbytes = snprintf(data,msg->i.nbytes,"%s",cl_strerror(ocb->cl_errno));
					if(nbytes < msg->i.nbytes){
						nbytes++;
					}else{
						nbytes = msg->i.nbytes;
					}
					break;
			}
			atomic_add(&state_info.fdone,1);
			pthread_rwlock_unlock( &cl_root_node_rwlock );
			
			break;
		}
		
		/*######## WE SHOULD NEVER GET IN HERE ########*/
		default:
			/* in normal case should never get in here */
			ocb->cl_errno = 0;
			return ENOSYS;
	}
	
	/* return answer to client */
	memset(&msg->o, 0, sizeof(msg->o) );
	msg->o.ret_val = retval;
	msg->o.nbytes = nbytes;
	return(_RESMGR_PTR(qctp, &msg->o, sizeof(msg->o)+nbytes ));
}



























































/*
 * Main function
 */
int main()
{
	char * token;
	
	/* 1) Make initial setup */
	if( (dpp = dispatch_create()) == NULL ){
		perror("dispatch context");
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
	
	memset( &state_info, 0, sizeof(state_info));
	time(&state_info.startup);
	
	
	/* 2) read config file and tune database to one's desire
	if( GetConf_open( "/etc/qavird.cnf" ) == 0 ){
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
	}
	GetConf_close(); /**/
	
	/* 3) now it is time to become a daemon
	{
		pid_t pid = fork();
		if (pid < 0) {
			perror("fork");
			return (-1);
		}
		if (pid != 0) return 0;
	}/**/	
	
	/* 3) change current working directory to one with DB's*/
	if( (tpp = thread_pool_create(&pool_attr, POOL_FLAG_EXIT_SELF)) == NULL){
		perror("pool");
		return (EXIT_FAILURE);
	}
	
	iofunc_func_init( _RESMGR_CONNECT_NFUNCS, &connect_func, _RESMGR_IO_NFUNCS, &io_func );
    io_func.devctl	= Qavird_devctl;	/* a handler to _IO_DEVCTL */

	iofunc_attr_init( &attr, S_IFNAM | 0777, 0, 0 );
	attr.mount = &mountpoint;
	
	/* tune resourse handling functions */
	memset( &resmgr_attr, 0, sizeof(resmgr_attr) );
	resmgr_attr.nparts_max = 1;
	resmgr_attr.msg_max_size = QAVIRD_MSG_LENGTH;
	
	/* register device */
	if( (id = resmgr_attach( dpp, &resmgr_attr, QAVIRD_DEV, _FTYPE_ANY, 0, &connect_func, &io_func, &attr )) == -1 ){
		perror("resource name");
		return (EXIT_FAILURE);
	}
	
	/* close all inputs and outputs and free memory */
    /*fclose(stdin);
    fclose(stdout);
    fclose(stderr);*/
    
    
    
    /*
     * Here we prepare libclamav
     */
    state_info.signum = 0;
    if( (ret = cl_loaddbdir(AVbases,&cl_root_node,&state_info.signum)) ){
    	printf("Error loading bases: %s\n",cl_strerror(ret));
    	return (EXIT_FAILURE);
	}
	
	printf("Loaded %d signatures\n",state_info.signum);
	
	if( (ret = cl_build(cl_root_node)) ){
		printf("Error building engine; %s\n",cl_strerror(ret));
		cl_free(cl_root_node);
		return (EXIT_FAILURE);
	}
	time(&state_info.base_time);
	
	
	/* parse headers of databases */
	{
		char	cvdname[256];
		struct cl_cvd * cvd_header;
		
		/* main.cvd */
		snprintf(cvdname,256,"%s/main.cvd",AVbases);
		cvd_header = cl_cvdhead(cvdname);
		if(!cvd_header){
			printf("error opening %s\n",cvdname);
			return (EXIT_FAILURE);
		}
		state_info.m_ver = cvd_header->version;
		state_info.m_sig = cvd_header->sigs;
		state_info.m_fl = cvd_header->fl;
		state_info.m_btime = cvd_header->stime;
		cl_cvdfree(cvd_header);
		/* daily.cvd */
		snprintf(cvdname,256,"%s/daily.cvd",AVbases);
		cvd_header = cl_cvdhead(cvdname);
		if(!cvd_header){
			printf("error opening %s\n",cvdname);
			return (EXIT_FAILURE);
		}
		state_info.d_ver = cvd_header->version;
		state_info.d_sig = cvd_header->sigs;
		state_info.d_fl = cvd_header->fl;
		state_info.d_btime = cvd_header->stime;
		cl_cvdfree(cvd_header);
		
	}
	
	
	
	
	/* start the pool!!! and have no exit!!! - from here for sure ;0) */
	thread_pool_start( tpp );
}
