#include "qavird.h"




struct	Qavird_info AVinfo;
int		AVfd;
int		ret;


char * Virfile = "/root/en0";/**/
/*char * Virfile = "/root/clamav/Viruses/raw/Letter.zip";/**/
/*char * Virfile = "/usr/bin/CC";/**/
/*char * Virfile = "/usr/bin/bcb";/**/




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
	printf("--- main.cvd ---\n");
	ctime_r(&AVinfo.m_btime,ttime);
	printf("Version   : %d\n",AVinfo.m_ver);
	printf("Built on  : %s",ttime);
	printf("Signatures: %d\n",AVinfo.m_sig);
	printf("Func.level: %d\n",AVinfo.m_fl);
	printf("--- daily.cvd ---\n");
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
	
	printf("Getting info on the AntiVirus\n");
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
	
	/* scan the file for virus */
	snprintf(buffer,256,"%s",Virfile);
	ret = devctl( AVfd, QAVIRD_CMD_SCANFILE, buffer, 256, &retval);
	if(ret != EOK){
		perror("SCANFILE");
		return (EXIT_FAILURE);
	}
	
	/* now see what are the results */
	switch(retval){
		case QAV_CLEAN:
			printf("file %s is clean\n",buffer);
			break;
			
		case QAV_INTERNAL:
			printf("Internal AV error:\n%s\n",buffer);
			break;
			
		case QAV_VIRUS:
			printf("Virus %s detected\n",buffer);
			break;
	}
	
	/* now reload the database */
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
	
	/* now retest for viruses */
	retsize = sizeof(AVinfo);
	ret = devctl( AVfd, QAVIRD_CMD_INFO, &AVinfo, retsize, &retval);
	if(ret != EOK){
		perror("INFO");
		return (EXIT_FAILURE);
	}
	printf("Reloaded bases\n");
	print_qav_info();
	
	/* scan the file for virus */
	snprintf(buffer,256,"%s",Virfile);
	ret = devctl( AVfd, QAVIRD_CMD_SCANFILE, buffer, 256, &retval);
	if(ret != EOK){
		perror("SCANFILE");
		return (EXIT_FAILURE);
	}
	
	/* now see what are the results */
	switch(retval){
		case QAV_CLEAN:
			printf("file %s is clean\n",buffer);
			break;
			
		case QAV_INTERNAL:
			printf("Internal AV error:\n%s\n",buffer);
			break;
			
		case QAV_VIRUS:
			printf("Virus %s detected\n",buffer);
			break;
	}

	
	return 0;
}
