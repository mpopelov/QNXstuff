/* (c) MikeP, 2005 */

#include "qavird.h"




struct	Qavird_info AVinfo;
int		AVfd;
int		ret;


void print_qav_info(){
	
	char	ttime[26];
	
	printf("\n******* Qavird status *******\n");
	ctime_r(&AVinfo.startup,ttime);
	printf("Daemon  started on: %s",ttime);
	printf("Requests processed: %u\n",AVinfo.fdone);
	printf("Viruses   detected: %u\n",AVinfo.virfound);
	ctime_r(&AVinfo.base_time,ttime);
	printf("AV bases loaded on: %s",ttime);
	printf("Signatures in base: %d\n",AVinfo.signum);
	printf("\n--- main.cvd ---\n");
	ctime_r(&AVinfo.m_btime,ttime);
	printf("Version   : %d\n",AVinfo.m_ver);
	printf("Built on  : %s",ttime);
	printf("Signatures: %d\n",AVinfo.m_sig);
	printf("Func.level: %d\n",AVinfo.m_fl);
	printf("\n--- daily.cvd ---\n");
	printf("Version   : %d\n",AVinfo.d_ver);
	ctime_r(&AVinfo.d_btime,ttime);
	printf("Built on  : %s",ttime);
	printf("Signatures: %d\n",AVinfo.d_sig);
	printf("Func.level: %d\n\n",AVinfo.d_fl);
	return;
}







int main(){
	
	int		retsize;
	int		retval;
	char	buffer[256];
	
	AVfd = open( "/dev/Qav", O_RDWR );
	if( AVfd == -1 ){
		perror("AVconnect");
		return (EXIT_FAILURE);
	}
	
	retsize = sizeof(AVinfo);
	ret = devctl( AVfd, QAVIRD_CMD_INFO, &AVinfo, retsize, &retval);
	if(ret != EOK){
		perror("INFO");
		return (EXIT_FAILURE);
	}
	print_qav_info();
	
	/* force daemon to reload databases */
	ret = devctl( AVfd, QAVIRD_CMD_AVBASERELOAD, NULL, 0, &retval);
	if(ret != EOK){
		perror("INFO");
		return (EXIT_FAILURE);
	}
	if(retval != QAV_CLEAN){
		printf("Error reloading virus bases\n");
		return (EXIT_FAILURE);
	}
	printf("Reloading antivirus bases successful\n");
	
	/* now reread info */
	retsize = sizeof(AVinfo);
	ret = devctl( AVfd, QAVIRD_CMD_INFO, &AVinfo, retsize, &retval);
	if(ret != EOK){
		perror("INFO");
		return (EXIT_FAILURE);
	}
	print_qav_info();
	
	return 0;
}

