#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void sh_wait(int com)
{
	int tm=0;
	tm = atoi(commandArgv[com]);
	sleep(tm); 
	printf("\nWe have wait for %d seconds.\n",tm);
	fflush(stdout);
}

void read_tsc(void)
{
	u32 low=0,high=0;
	asm volatile("rdtsc":"=a"(low),"=d"(high));

	printf("The tsc val is 0x%x%x\n",high,low);
}
