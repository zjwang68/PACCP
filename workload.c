#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>  
#include <sys/socket.h>  
#include <stdio.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <unistd.h>  
#include <string.h>  
#include <stdlib.h>  
#include <fcntl.h>  
#include <sys/shm.h>  
#include <math.h>  
#include <sys/timerfd.h>
#include <signal.h>
#include "workload.h"
#include "parameter.h"
extern int size0;
extern int deadline0_ms;
extern int size1;
extern int deadline1_ms;
extern int Rmin;
extern int switch_non_deadline;// not use now//0 : having no non-deadline flow; 1 : having non-deadline flows.
extern int ratio_non_deadline;// if ratio_non_deadline==8  , 80% of the flows are the non-deadline.




int flag_timer;//used for timer


void timer_interval(int usec)
{
	struct timeval tv;
        int retval;
        tv.tv_sec = 0;
        tv.tv_usec = usec;

        select(0, NULL, NULL, NULL, &tv);
	return;
}


 	 
/* init */
void init_time(int usec)
{
	struct itimerval val;
 	 
	val.it_value.tv_sec = 0;
	val.it_value.tv_usec = usec;
	val.it_interval = val.it_value;
	setitimer(ITIMER_PROF, &val, NULL);
	flag_timer = 1;
	while(flag_timer == 1 )
	{
		//sched_yield();		
	}

	printf("\ntime expired=%d",usec);
}


/* signal process */
void timeout_info(int signo)
{
	struct itimerval val;

        val.it_value.tv_sec = 0;
        val.it_value.tv_usec = 0;
        val.it_interval = val.it_value;
        setitimer(ITIMER_PROF, &val, NULL);
	printf("\ntimer process");
	flag_timer = 0;
}

/* init sigaction */
void init_sigaction(void)
{
 	struct sigaction act;
 	 
	act.sa_handler = timeout_info;
	act.sa_flags   = 0;
	sigemptyset(&act.sa_mask);
	sigaction(SIGPROF, &act, NULL);
}

int get_flow_deadline_ms(long int size_flow)
{
	double deadline_ms;
static int num=1;
	num++;
if(num > 10)
	num=1;
//printf("\n num=%d ",num);
	if(/*switch_non_deadline==1 && */(num <= ratio_non_deadline))
	{
//printf( "ratio_non_deadline=%d deadline=0\n",ratio_non_deadline);
//printf("\n non deadline flow");
		return 0;
	}	
//printf("\n");
	if(size_flow < size0)
	{
		deadline_ms = deadline0_ms;
	}
	else if(size_flow < size1)
	{
		deadline_ms = deadline1_ms;
	}
	else
	{
		//deadline_ms = (size_flow*8)*1000/(Rmin*1000000);
		deadline_ms = (size_flow)/((double)Rmin*125);
	}	
	return (int)deadline_ms;
}


/**
For 2x3 (2 spine and 3 TOR), each link is 1Gbps
The flow arrival rate for the whole system is:

For data-mining workload, the flow arrival rate (arate: number of flows persecond)
Load    20%  30%  40%  50%  60%  70%  80%  90%
arate   51   76   102  127  153  178  204  230

For web-search workload, 
Load    20%  30%  40%  50%  60%  70%  80%  90%
arate   42   63   85   106  127  149  170  191
**/

/****************************************************
Random double number generator Function between 0 and 1.
***************************************************/
double get_random()
{
  unsigned int xx;
  double y;

  y = (double)rand()/(double)RAND_MAX;
  if(y == 1.0) { y -= 0.0000000001;}

  return y;
}

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Next Request Generator. arate: the request arrival rate: the number of flows per second.
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
int get_next_time_usec(double arate)
{
	double  dice, tm;
	int tm_usec;
	dice = get_random();
	if(dice == 0.0) 
	{ 
		dice = 10E-100;
	}

	tm = -log(dice)/arate;

	tm_usec = tm*1000000;

   return tm_usec;
}



/* get a random number between 0.0~1.0 */
double gen_rand()
{
	double r;
	r=(float)(rand()%10000)/10000;
	return r;
}

int c11=0,c12=0,c13=0,c14=0,c15=0;


int get_size_flow_byte(int traffic_type)
{
	double r1,r2;
	int bits_num;
	////Web search distribution (each unit is 100 bytes)	
	if(traffic_type==1)
	{
		r1=gen_rand();

		if(r1 < 0.3)
		{
			bits_num =10+(int )(90*r1);
			c11++;
		}
		else if(r1 < 0.6)
		{
			bits_num =100+(int )(900*r1);
			c12++;
		}
		else if(r1 < 0.75)
		{
			bits_num =1000+(int )(9000*r1);
			c13++;
		}
		else if(r1 < 0.99)
		{
			bits_num =10000+(int )(90000*r1);
			c14++;
		}
		else
		{
			bits_num =100000+(int )(4000000*r1);
			c15++;
		}
	}
	else if(traffic_type==2)
	{
		r2=gen_rand();

		if(r2 < 0.4)
			bits_num =1+(int )(9*r2);
		else if(r2 < 0.75)
			bits_num =10+(int )(90*r2);
		else if(r2 < 0.85)
			bits_num =100+(int )(9000*r2);
		else if(r2 < 0.99)
			bits_num =10000+(int )(90000*r2);
		else
			bits_num =100000+(int )(4000000*r2);
	}
	else
		return -1;

//printf("\nr1=%f",r1);
	if(bits_num < sizeof(long int))
	{
		bits_num = sizeof(long int)+2;	
	
	}
	bits_num = bits_num*100;//conver to bytes
	return bits_num;
}
