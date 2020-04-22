/*
 * -={ MikeP }=-
 */

#include <sys/neutrino.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/syspage.h>

int main(){
	
	uint64_t cps, s_cycles, e_cycles;
	float secs; /* seconds between cycles measurement */
	
	ThreadCtl(_NTO_TCTL_IO, NULL);
	
	cps = SYSPAGE_ENTRY(qtime)->cycles_per_sec;
	printf("CPS is %lld as of SYSPAGE_ENTRY macro\n", cps);
	
	s_cycles = ClockCycles();
	delay(100);
	sleep(1);
	e_cycles = ClockCycles();
	
	secs = (((float)(e_cycles-s_cycles))/(float)cps);
	
	printf("Cycles between measurements: %lld\n", (e_cycles-s_cycles) );
	printf("Elapsed delay() in SECONDS as of cycles: %f\n", secs);
	return 0;
}
