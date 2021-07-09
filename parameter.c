#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <getopt.h>
#include <sys/shm.h>
#include <sched.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "parameter.h"


// the information servers
char ip_host[16]="127.0.0.1";//the ip address of local host
char ip_servers[NUM_SERVERS][16] = {{IP_SERVER},{IP_SERVER},{IP_SERVER},{IP_SERVER}};
int port_server = PORT_SERVER;
int num_threads=NUM_THREADS;
int counter_flows=0;//counte the number of opened flows
int num_flows = 1;// the number flows in this workload 
int type_traffic=2; // workload type: 1 for web search; 2 for data mining
double arate=60;// how many flows should arrive in each second


int size0 = 100*1024;
int deadline0_ms = 4;
int size1 = 1014*1024;
int deadline1_ms = 6;
int Rmin = 1024*1024;
int switch_non_deadline = 0;// 0 : having no non-deadline flow; 1 : having non-deadline flows.
int ratio_non_deadline = 10;//  ration_non_deadline% flows are the non-deadline.

char path_result[256] = "./result.txt";
char specification[256] = "unknow";


int specification_configuration()
{
        printf("\n--%s=value or -%c  value: the value for size0.", OPT_LONG_SIZE0, OPT_SHORT_SIZE0);
        printf("\n--%s=value or -%c  value: the value for size1.", OPT_LONG_SIZE1, OPT_SHORT_SIZE1);
        printf("\n--%s=value or -%c  value: the deadline for 0~size0.", OPT_LONG_DEADLINE0, OPT_SHORT_DEADLINE0);
        printf("\n--%s=value or -%c  value: the deadline for size0~size1.", OPT_LONG_DEADLINE1, OPT_SHORT_DEADLINE1);
        printf("\n--%s=value or -%c  value: the value for Rmin.", OPT_LONG_RMIN, OPT_SHORT_RMIN);

        printf("\n--%s=value or -%c  value: the number of flows to test.", OPT_LONG_NUM_FLOWS, OPT_SHORT_NUM_FLOWS);

        printf("\n--%s=value or -%c  value: the TCP port of TCP server.", OPT_LONG_TCP_PORT, OPT_SHORT_TCP_PORT);
        printf("\n--%s=value or -%c  value: the path of the result.", OPT_LONG_PATH, OPT_SHORT_PATH);

        printf("\n--%s=value or -%c  value: the type of workload.", OPT_LONG_TYPE_TRAFFIC, OPT_SHORT_TYPE_TRAFFIC);
        printf("\n--%s=value or -%c  value: the arrival rate of the workload.", OPT_LONG_ARATE, OPT_SHORT_ARATE);

        printf("\n--%s=value or -%c  value: the switch for non-deadline flows.", OPT_LONG_SWITCH_NON_DEADLINE, OPT_SHORT_SWITCH_NON_DEADLINE);
        printf("\n--%s=value or -%c  value: if(the num%ratio_non_deadline==0 && switch_non_deadline==1), the flow is the non-deadline.", OPT_LONG_RATIO_NON_DEADLINE, OPT_SHORT_RATIO_NON_DEADLINE);

#define A(x) printf("\n--%s=value or -%c  value: the IP address of TCP server%d.", OPT_LONG_IP_ADDRESS##x, OPT_SHORT_IP_ADDRESS##x, x)
	A(0);
	A(1);	
	A(2);
	A(3);	
	A(0);
	A(4);	
	A(5);
	A(6);
	A(7);	

	printf("\n");
        printf("\nA example: ./a.out --ipServer=192.168.1.19 --portServer=8887 --pathResult=./xx.txt");
        return 0;
}


int parse_configuration(int argc, char **argv)
{
        int opt, opt_idx, num_options;

        static const struct option long_opt[] = {
		{OPT_LONG_NUM_FLOWS,     2, 0,  OPT_SHORT_NUM_FLOWS},

                {OPT_LONG_IP_HOST,     2, 0,  OPT_SHORT_IP_HOST},

                {OPT_LONG_IP_ADDRESS0,     2, 0,  OPT_SHORT_IP_ADDRESS0},
                {OPT_LONG_IP_ADDRESS1,     2, 0,  OPT_SHORT_IP_ADDRESS1},
                {OPT_LONG_IP_ADDRESS2,     2, 0,  OPT_SHORT_IP_ADDRESS2},
                {OPT_LONG_IP_ADDRESS3,     2, 0,  OPT_SHORT_IP_ADDRESS3},
                {OPT_LONG_IP_ADDRESS4,     2, 0,  OPT_SHORT_IP_ADDRESS4},
                {OPT_LONG_IP_ADDRESS5,     2, 0,  OPT_SHORT_IP_ADDRESS5},
                {OPT_LONG_IP_ADDRESS6,     2, 0,  OPT_SHORT_IP_ADDRESS6},
                {OPT_LONG_IP_ADDRESS7,     2, 0,  OPT_SHORT_IP_ADDRESS7},


                {OPT_LONG_TCP_PORT,     2, 0,  OPT_SHORT_TCP_PORT},
                {OPT_LONG_PATH,     2, 0,  OPT_SHORT_PATH},

		{OPT_LONG_SIZE0,     2, 0,  OPT_SHORT_SIZE0},
		{OPT_LONG_SIZE1,     2, 0,  OPT_SHORT_SIZE1},
		{OPT_LONG_DEADLINE0,     2, 0,  OPT_SHORT_DEADLINE0},
		{OPT_LONG_DEADLINE1,     2, 0,  OPT_SHORT_DEADLINE1},
		{OPT_LONG_RMIN,     2, 0,  OPT_SHORT_RMIN},

		{OPT_LONG_TYPE_TRAFFIC,     2, 0,  OPT_SHORT_TYPE_TRAFFIC},
		{OPT_LONG_ARATE,     2, 0,  OPT_SHORT_ARATE},
		{OPT_LONG_SWITCH_NON_DEADLINE,     2, 0,  OPT_SHORT_SWITCH_NON_DEADLINE},
		{OPT_LONG_RATIO_NON_DEADLINE,     2, 0,  OPT_SHORT_RATIO_NON_DEADLINE},
		{OPT_LONG_SPECIFICATION,     2, 0,  OPT_SHORT_SPECIFICATION},
		{OPT_LONG_NUM_THREADS,     2, 0,  OPT_SHORT_NUM_THREADS},

                {0,         0,                 0,  0 }
        };

        num_options=0;
        optind =0;
        optarg =NULL;
printf("\n\n");
        while((opt = getopt_long(argc, argv, "n:i:", long_opt, &opt_idx)) != EOF)
        {
                switch(opt)
                {
			case OPT_SHORT_NUM_FLOWS:
		#ifndef DEBUG_YXWU__
				printf("\n%s=%s",OPT_LONG_NUM_FLOWS,optarg);
	#endif
                                num_flows = atoi(optarg);
                                num_options++;
                                break;
                        case OPT_SHORT_IP_HOST:
		#ifndef DEBUG_YXWU__
                                printf("\n%s=%s",OPT_LONG_IP_HOST,optarg);
	#endif
                                strcpy(ip_host, optarg);
                                num_options++;
                                break;

                        case OPT_SHORT_IP_ADDRESS0:
		#ifndef DEBUG_YXWU__
                                printf("\n%s=%s",OPT_LONG_IP_ADDRESS0,optarg);
	#endif
                                strcpy(&(ip_servers[0][0]), optarg);
                                num_options++;
                                break;
                        case OPT_SHORT_IP_ADDRESS1:
		#ifndef DEBUG_YXWU__
                                printf("\n%s=%s",OPT_LONG_IP_ADDRESS1,optarg);
	#endif
                                strcpy(&(ip_servers[1][0]), optarg);
                                num_options++;
                                break;
                        case OPT_SHORT_IP_ADDRESS2:
		#ifndef DEBUG_YXWU__
                                printf("\n%s=%s",OPT_LONG_IP_ADDRESS2,optarg);
	#endif
                                strcpy(&(ip_servers[2][0]), optarg);
                                num_options++;
                                break;
                        case OPT_SHORT_IP_ADDRESS3:
		#ifndef DEBUG_YXWU__
                                printf("\n%s=%s",OPT_LONG_IP_ADDRESS3,optarg);
	#endif
                                strcpy(&(ip_servers[3][0]), optarg);
                                num_options++;
                                break;
                        case OPT_SHORT_IP_ADDRESS4:
		#ifndef DEBUG_YXWU__
                                printf("\n%s=%s",OPT_LONG_IP_ADDRESS4,optarg);
	#endif
                                strcpy(&(ip_servers[4][0]), optarg);
                                num_options++;
                                break;
                        case OPT_SHORT_IP_ADDRESS5:
		#ifndef DEBUG_YXWU__
                                printf("\n%s=%s",OPT_LONG_IP_ADDRESS5,optarg);
	#endif
                                strcpy(&(ip_servers[5][0]), optarg);
                                num_options++;
                                break;
                        case OPT_SHORT_IP_ADDRESS6:
		#ifndef DEBUG_YXWU__
                                printf("\n%s=%s",OPT_LONG_IP_ADDRESS6,optarg);
	#endif
                                strcpy(&(ip_servers[6][0]), optarg);
                                num_options++;
                                break;
                        case OPT_SHORT_IP_ADDRESS7:
		#ifndef DEBUG_YXWU__
                                printf("\n%s=%s",OPT_LONG_IP_ADDRESS7,optarg);
	#endif
                                strcpy(&(ip_servers[7][0]), optarg);
                                num_options++;
                                break;

			case OPT_SHORT_TCP_PORT:
		#ifndef DEBUG_YXWU__
                                printf("\n%s=%s",OPT_LONG_TCP_PORT,optarg);
	#endif
                                port_server = atoi(optarg);
                                num_options++;
                                break;
                        case OPT_SHORT_PATH:
		#ifndef DEBUG_YXWU__
                                printf("\n%s=%s",OPT_LONG_PATH,optarg);
	#endif
                                strcpy(path_result, optarg);
                                num_options++;
                                break;


			case OPT_SHORT_SIZE0:
		#ifndef DEBUG_YXWU__
                                printf("\n%s=%s",OPT_LONG_SIZE0,optarg);
	#endif
                                size0 = atoi(optarg);
                                num_options++;
                                break;
			case OPT_SHORT_SIZE1:
		#ifndef DEBUG_YXWU__
                                printf("\n%s=%s",OPT_LONG_SIZE1,optarg);
	#endif
                                size1 = atoi(optarg);
                                num_options++;
                                break;
			case OPT_SHORT_DEADLINE0:
		#ifndef DEBUG_YXWU__
                                printf("\n%s=%s",OPT_LONG_DEADLINE0,optarg);
	#endif
                                deadline0_ms = atoi(optarg);
                                num_options++;
                                break;
			case OPT_SHORT_DEADLINE1:
		#ifndef DEBUG_YXWU__
                                printf("\n%s=%s",OPT_LONG_DEADLINE1,optarg);
	#endif
                                deadline1_ms = atoi(optarg);
                                num_options++;
                                break;
			case OPT_SHORT_RMIN:
		#ifndef DEBUG_YXWU__
                                printf("\n%s=%s",OPT_LONG_RMIN,optarg);
	#endif
                                Rmin = atoi(optarg);
                                num_options++;
                                break;

			case OPT_SHORT_TYPE_TRAFFIC:
		#ifndef DEBUG_YXWU__
                                printf("\n%s=%s",OPT_LONG_TYPE_TRAFFIC,optarg);
	#endif
                                type_traffic = atoi(optarg);
                                num_options++;
                                break;
                        case OPT_SHORT_ARATE:
		#ifndef DEBUG_YXWU__
                                printf("\n%s=%s",OPT_LONG_ARATE,optarg);
	#endif
                                arate = atoi(optarg);
                                num_options++;
                                break;

			case OPT_SHORT_SWITCH_NON_DEADLINE:
		#ifndef DEBUG_YXWU__
                                printf("\n%s=%s",OPT_LONG_SWITCH_NON_DEADLINE,optarg);
	#endif
                                switch_non_deadline = atoi(optarg);
                                num_options++;
                                break;
                        case OPT_SHORT_RATIO_NON_DEADLINE:
		#ifndef DEBUG_YXWU__
                                printf("\n%s=%s",OPT_LONG_RATIO_NON_DEADLINE,optarg);
	#endif
                                ratio_non_deadline = atoi(optarg);
                                num_options++;
                                break;

                	case OPT_SHORT_SPECIFICATION:
		#ifndef DEBUG_YXWU__
                                printf("\n%s=%s",OPT_LONG_SPECIFICATION,optarg);
	#endif
                                strcpy(specification, optarg);
                                num_options++;
                                break;
                	case OPT_SHORT_NUM_THREADS:
                                num_threads = atoi(optarg);
		#ifndef DEBUG_YXWU__
                                printf("\n%s=%s=%d",OPT_LONG_NUM_THREADS,optarg,num_threads);
	#endif
                                num_options++;
                                break;
                        default:
                                printf("Unknow option!!!!see the specification below~\n");
                                specification_configuration();
                                exit(EXIT_FAILURE);
                }
        }
        printf("\nThe number of parameters in total: %d\n",num_options);
}



