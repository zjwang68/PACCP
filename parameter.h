#ifndef YUNXIANG_WU_RING__
#define YUNXIANG_WU_RING__


#define PORT_SERVER   8887 // the port of the TCP server
#define IP_SERVER  "192.168.1.185" // the ip address of the TCP server
#define NUM_THREADS 1
#define NUM_QUEUES 100// the max number of threads; each thread has one queue;
#define NUM_SERVERS 4
#define MAX_NUM_THREADS 100

//#define DEBUG_YXWU__ 1
//#define DEBUG_EVENT__ 1

//definition for parameter
#define OPT_LONG_SIZE0 "size0"
#define OPT_SHORT_SIZE0 'a'

#define OPT_LONG_SIZE1 "size1"
#define OPT_SHORT_SIZE1 'b'

#define OPT_LONG_DEADLINE0 "deadline0"
#define OPT_SHORT_DEADLINE0 'c'

#define OPT_LONG_DEADLINE1 "deadline1"
#define OPT_SHORT_DEADLINE1 'd'

#define OPT_LONG_RMIN "Rmin"
#define OPT_SHORT_RMIN 'e'

#define OPT_LONG_NUM_FLOWS "numFlows"
#define OPT_SHORT_NUM_FLOWS 'f'


#define OPT_LONG_TCP_PORT "portServer"
#define OPT_SHORT_TCP_PORT 'h'

#define OPT_LONG_PATH "pathResult"
#define OPT_SHORT_PATH 'i'

#define OPT_LONG_TYPE_TRAFFIC "typeTraffic"
#define OPT_SHORT_TYPE_TRAFFIC 'j'

#define OPT_LONG_ARATE "aRate"
#define OPT_SHORT_ARATE 'k'

#define OPT_LONG_SWITCH_NON_DEADLINE "switchNonDeadline"
#define OPT_SHORT_SWITCH_NON_DEADLINE 'l'

#define OPT_LONG_RATIO_NON_DEADLINE "ratioNonDeadline"
#define OPT_SHORT_RATIO_NON_DEADLINE 'm'


#define OPT_LONG_IP_ADDRESS0 "ipServer0"
#define OPT_SHORT_IP_ADDRESS0 'g'
#define OPT_LONG_IP_ADDRESS1 "ipServer1"
#define OPT_SHORT_IP_ADDRESS1 'n'
#define OPT_LONG_IP_ADDRESS2 "ipServer2"
#define OPT_SHORT_IP_ADDRESS2 'o'
#define OPT_LONG_IP_ADDRESS3 "ipServer3"
#define OPT_SHORT_IP_ADDRESS3 'p'
#define OPT_LONG_IP_ADDRESS4 "ipServer4"
#define OPT_SHORT_IP_ADDRESS4 'q'
#define OPT_LONG_IP_ADDRESS5 "ipServer5"
#define OPT_SHORT_IP_ADDRESS5 'r'
#define OPT_LONG_IP_ADDRESS6 "ipServer6"
#define OPT_SHORT_IP_ADDRESS6 's'
#define OPT_LONG_IP_ADDRESS7 "ipServer7"
#define OPT_SHORT_IP_ADDRESS7 't'

#define OPT_LONG_IP_HOST "ipHost"
#define OPT_SHORT_IP_HOST 'u'


#define OPT_LONG_SPECIFICATION "specification"
#define OPT_SHORT_SPECIFICATION 'v'

#define OPT_LONG_NUM_THREADS "numThreads"
#define OPT_SHORT_NUM_THREADS 'w'

int specification_configuration();
int parse_configuration(int argc, char **argv);
int gen_traffic_possion(int type_traffic, int num_flows, double arate);


#endif
