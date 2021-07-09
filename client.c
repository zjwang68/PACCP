#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
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
#include <sys/epoll.h>
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

#include "workload.h"
#include "parameter.h"

#define SIOC_TCP_PARAMETER      0x894d
 
pthread_t pthread[NUM_THREADS];

int counter_threads=0;
int num_connect=0;
int num_response=0;

extern char ip_servers[][16];
extern int num_flows;
extern int num_threads;
extern int port_server;
extern int type_traffic;
extern double arate;
extern char ip_host[];

extern char path_result[];
extern char specification[];

pthread_mutex_t lock_global = PTHREAD_MUTEX_INITIALIZER;

struct queue_thread
{
	int id_thread;
	struct flow_information *flow_buffer;
	int index_fetch;// index for fetching
	int index_insert;// for inserting
	int max_num_flows;//// the max number of flows in each queue
	pthread_mutex_t lock;//lock for shared variable
//for statistic
	
	int insert;
	int fetch;
	int connection;
	int send;
	int receive;
}queue_threads[NUM_QUEUES];

struct flow_information
{
	long int size_flow;
	int deadline;
	struct timespec time_start;
	struct timespec time_connection;
        struct timespec time_end;
        long int time_ns_complete;//the time to complete the flow.
        long int time_ns_connection;//the time to complete the flow.
	char *ip_server;
	char * buf;// data buffer	
	int pos;	
};
struct
{
	int insert;
	int fetch;
	int connection;
	int send;
	int receive;
}statistic[NUM_QUEUES];

struct arg_flow
{
        int deadline;
        int size;
};

int init_statistic()
{
	int i=0;
	for(i=0; i<num_threads; i++)
	{
		queue_threads[i].insert=0;
		queue_threads[i].fetch=0;
		queue_threads[i].connection=0;
		queue_threads[i].send=0;
		queue_threads[i].receive=0;
	}
}

int printf_statistic()
{
	int i=0;
        for(i=0; i<num_threads; i++)
        {
		struct queue_thread *q;
		q=&(queue_threads[i]);	
		printf("\n%s insert=%d,fetch=%d, connection=%d,send=%d,receive=%d,size=%d, pos=%d",ip_host, queue_threads[i].insert, queue_threads[i].fetch,  queue_threads[i].connection, queue_threads[i].send,  queue_threads[i].receive, q->flow_buffer[q->receive].size_flow, q->flow_buffer[q->receive].pos);
        }
}



int alloc_flow_buf(struct flow_information * flow_inf)
{
	if(flow_inf ==NULL)
		return -1;

	flow_inf->buf = malloc(flow_inf->size_flow);
	if(flow_inf->buf == NULL)
	{
		printf("\n alloc memory for flow_inf->buf fail!!!!");
		exit(-1);
		return -1;
	}
	memset(flow_inf->buf, 6, flow_inf->size_flow);//2018-8-26
	return 0;
}

int free_flow_buf(struct flow_information *flow_inf)
{
	if(flow_inf->buf!=NULL)
		free(flow_inf->buf);
}

int insert_flow(long int size_flow, int deadline, struct queue_thread *q)
{
	int ret;
	static int j=0;
	pthread_mutex_lock(&(q->lock));
	if(q->index_insert == q->max_num_flows)
	{
		pthread_mutex_unlock(&(q->lock)); 
		printf("\n error :the ring is full!");
		return -1;
	}
	q->flow_buffer[q->index_insert].size_flow=size_flow;
	q->flow_buffer[q->index_insert].pos=0;
	q->flow_buffer[q->index_insert].deadline=deadline;
	//ret = alloc_flow_buf( &(q->flow_buffer[q->index_insert]) );///do not forget the free_flow_buf()
	//printf("\n\ninsert queue=%p num_current=%d index_fetch=%d flow addr=%p buf addr=%p",q,q->index_insert,q->index_fetch,&(q->flow_buffer[q->index_insert]),(q->flow_buffer[q->index_insert]).buf);	

	q->flow_buffer[q->index_insert].ip_server = &(ip_servers[j][0]);//2018-8-24	
	j++;
	if(j == NUM_SERVERS)
		j = 0;

	q->index_insert +=1;
q->insert++;	

	pthread_mutex_unlock(&(q->lock)); 
}

struct flow_information * fetch_flow(struct queue_thread *q)
{
	struct flow_information * p;

	pthread_mutex_lock(&(q->lock));
	if(q->index_fetch == q->index_insert)
	{
		pthread_mutex_unlock(&(q->lock)); 
		return NULL;
	}
//printf("\n\nfetch flow index_fetch=%d num_flow_current=%d",q->index_fetch,q->index_insert);
	p =&(q->flow_buffer[q->index_fetch]);
	q->index_fetch +=1;

q->fetch++;	

	pthread_mutex_unlock(&(q->lock)); 
	return p;
}

int create_queues(struct queue_thread *q, int num_queues, int max_num_flows)
{
	int i, j;
	for(i=0;i < num_queues;i++)
	{
		q[i].index_insert=0;
		q[i].max_num_flows = max_num_flows;
		q[i].index_fetch = 0;// the number for fetching
		pthread_mutex_init(&(q[i].lock),NULL);
		q[i].flow_buffer = (struct flow_information *)malloc(max_num_flows*sizeof(struct flow_information));
		if(q[i].flow_buffer==NULL)
		{
			printf("\n error : alloc memory fail for queues");
			return -1;
		}
		for(j=0;j<max_num_flows;j++)
		{
			q[i].flow_buffer[j].size_flow =0;	
			q[i].flow_buffer[j].deadline =0;	
			q[i].flow_buffer[j].time_ns_complete =0;	
			q[i].flow_buffer[j].time_ns_connection =0;
			q[i].flow_buffer[j].buf =NULL; //2018-8-26	
		}
		
	}
	return 0;
}

int destroy_queues(struct queue_thread *q, int num_queues)
{
	int i, j;
	for(i=0;i<num_queues;i++)
	{
		pthread_mutex_destroy(&(q[i].lock));
	}
}

int implement_flow(struct queue_thread *q,int port, char *ip, char *buf_send, long int len_send, char *buf_receive, long int max_len_receive,struct flow_information *flow_buffer)
{
	struct sockaddr_in servaddr;
	long int connect_time, receive_response_time; 
	int sockfd, n, nwrite, size_flow, ret;

	size_flow=len_send;
printf("implement begin\n");
	//first sizeof(long int) store the length of this flow
        long int *psz;
        psz = (long int *)(&(buf_send[0]));
        *psz = htonl(len_send);


	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if(sockfd < 0)
	{
		printf("\ncreate socket fail");
		exit(-1);
	}
////////////////////////////
	struct arg_flow xx;
        xx.deadline= flow_buffer->deadline;
        xx.size= flow_buffer->size_flow;
        //ioctl(sockfd, SIOC_TCP_PARAMETER, &xx);
/////////////////////
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &servaddr.sin_addr);
	servaddr.sin_port = htons(port);
	clock_gettime(CLOCK_MONOTONIC, &(flow_buffer->time_start));
	ret = connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	if(ret < 0)
	{
		char str[1000];
                sprintf(str, "%s->%s  port=%d: connect fail+++",ip_host,ip,port_server);
                perror(str);
	}
printf("\nconnect success");
	clock_gettime(CLOCK_MONOTONIC, &(flow_buffer->time_connection));
	flow_buffer->time_ns_connection = 1000000000*(flow_buffer->time_connection.tv_sec-flow_buffer->time_start.tv_sec)+(flow_buffer->time_connection.tv_nsec-flow_buffer->time_start.tv_nsec);

	q->connection++;
	
//	int pos = 0;
	while(len_send>0)
	{
		
    		nwrite = send(sockfd, buf_send + flow_buffer->pos, len_send,MSG_DONTWAIT);
		if(nwrite>0)
		{
			len_send =len_send - nwrite;
			flow_buffer->pos +=nwrite;
		}
		if(nwrite<0)
		{
		}
	}


	q->send++;


	while(1)
	{ 
		//n = read(sockfd, buf_receive, max_len_receive);
		n = recv(sockfd, buf_receive, max_len_receive,MSG_DONTWAIT);
		if(n>0)
		{
			//if(buf_receive[0]=='y')
				//printf("\ni receive a y\n");
			break;
		}
		else
		{
		}
	}
	
	q->receive++;

	close(sockfd);
	clock_gettime(CLOCK_MONOTONIC, &(flow_buffer->time_end));
	flow_buffer->time_ns_complete = 1000000000*(flow_buffer->time_end.tv_sec-flow_buffer->time_start.tv_sec)+(flow_buffer->time_end.tv_nsec-flow_buffer->time_start.tv_nsec);

	printf("\n connction time is %ld,receive response time is %ld", flow_buffer->time_ns_connection, flow_buffer->time_ns_complete);

	return 0;
}

//insert the flow to different theads in order
int gen_flow(long int size_flow, int deadline)
{
	static int k=0;//for thread or queue number

	//printf("\n gen_flow");	

	insert_flow(size_flow, deadline, &(queue_threads[k]));

	k++;

	if(k == num_threads)
		k = 0;
}

int gen_traffic_possion(int gen_type_traffic, int gen_num_flows, double gen_arate)
{
        long int gen_size_flow, i, gen_deadline;
        long int interval_usec;
//printf("\ngen_arate=%f", gen_arate);
        for(i=0; i < gen_num_flows; i++)
        {
                gen_size_flow = 1073741824;//get_size_flow_byte(gen_type_traffic);

                gen_deadline = get_flow_deadline_ms(gen_size_flow);
		
printf("\n\nflow size=%d\n\n",gen_size_flow);
                gen_flow(gen_size_flow, gen_deadline);

                interval_usec = get_next_time_usec(gen_arate);//2018-8-26
//2018-8-26
                timer_interval(interval_usec);//init_time(interval_usec);  //if using init_time, must call the init_sigaction() at the func_thread0() in client.c

                //printf("\narate=%f,size_flow=%d deadline=%d interval=%ld", gen_arate, gen_size_flow, gen_deadline, interval_usec);
        }
        return 0;

}

void *func_thread(void *arg)
{
	int ret;
	struct flow_information *flow;
	struct queue_thread *q;
	time_t time_now, time_start;
	char buf_receive[1000];
	q=(struct queue_thread *)arg;

//printf("\nthread start: thread id=%d, arg addr=%p, max_num_flows=%d",q->id_thread,arg,q->max_num_flows);

pthread_mutex_lock(&lock_global);
	counter_threads++;
pthread_mutex_unlock(&lock_global);

while(1)
{
        time(&time_start);
FETCH:
        time(&time_now);
        if(((time_now - time_start) >= 30) && (q->index_insert==q->index_fetch))
                goto END;
        flow = fetch_flow(q);
        if(flow!=NULL)
        {
//              printf("\n\nthread runing: thread id=%d, arg addr=%p, max_num_flows=%d,flow addr=%p ip_server=%s, port=%d,size=%ld, deadline=%d buf addr=%p",q->id_thread,arg,q->max_num_flows,flow, flow->ip_server,port_server, flow->size_flow,flow->deadline,flow->buf);

		ret = alloc_flow_buf( flow );///do not forget the free_flow_buf()

                implement_flow(q,port_server, flow->ip_server, flow->buf, flow->size_flow, buf_receive, 1000, flow);

		if(flow != NULL)
			free_flow_buf(flow);
                //time(&time_start);
                time_start = time_now;
        }
	sched_yield();//2018-8-28
        goto FETCH;
}


END:
pthread_mutex_lock(&lock_global);
		counter_threads--;
pthread_mutex_unlock(&lock_global);
}




int create_threads(pthread_t *pthread, int num_threads)
{
	int i, j, ret;
	void * func[num_threads];

	for(i=0; i < num_threads; i++)
	{
		func[i] = func_thread;
		ret=pthread_create(&pthread[i], NULL, func[i], &(queue_threads[i]));
		if(ret != 0)
		{
			printf("\nERROR: main: fail to create thread\n");
			return -1;
		}
		queue_threads[i].id_thread=i;
		printf("\ncreate thread %d, arg=%p",i, &(queue_threads[i]));
		
	}

	return i;
}


void record_result(char *logfile)
{
	FILE *fd;
	int i, j;
	char *message;
	struct queue_thread *q;
	struct flow_information *flow;
	char str[512];
//	remove(logfile);     
	fd = fopen(logfile, "w+");
 
	if(fd == NULL)
	{
        	printf("error :can not open the log file");
	        printf("error :can not open the log file");
        	exit(-1);
    	}
	else
	{
		for(i=0; i < num_threads; i++)
		{
			q = &(queue_threads[i]);
			for(j=0; j < q->index_insert; j++)
			{
				flow = &(q->flow_buffer[j]);
				sprintf(str, "%-16ld%16ld%16ld%16ld%16ld:%ld%16ld:%ld%16ld:%ld\n", flow->size_flow, flow->deadline, flow->time_ns_complete, flow->time_ns_connection, flow->time_start.tv_sec,flow->time_start.tv_nsec, flow->time_connection.tv_sec, flow->time_connection.tv_nsec, flow->time_end.tv_sec, flow->time_end.tv_nsec);	
        			fputs(str, fd);
			}
			
		}

	}
     
	fclose(fd);
}

int main(int argc, char *argv[])
{
	int i,j,max_num_flows;
	int tmp;
	
	//get the start time for the name of record file
	time_t time_start;
	time(&time_start);
	struct tm *pt;
	pt = localtime(&time_start);

	srand((unsigned)time(NULL)); 

//----------------parameter configuration---------------------------
//parameter configuration
	parse_configuration(argc, argv);

//--------------------end---------------------
	max_num_flows = num_flows;// the max number of flows in each queue 
	for(i=0;i<num_threads;i++)
	{
		queue_threads[i].flow_buffer=NULL;	
		queue_threads[i].id_thread=i;
	}

	init_statistic();

	create_queues(queue_threads, num_threads, max_num_flows);

	create_threads(pthread, num_threads);

//-------------------------insert flows into the queues----------------------------------
	gen_traffic_possion(type_traffic, num_flows, arate);
//-----------------------end--------------------------------------------	

	// wait for all threads or flows 
	sleep(3);
	while(1)
	{
	
		pthread_mutex_lock(&lock_global);
		tmp=counter_threads;
		pthread_mutex_unlock(&lock_global);

		if(tmp==0)
			break;
		sched_yield();
		sleep(6);
		//printf("\n%s ,counter_threads=%d,num_connect =%d,num_response=%d",ip_host, counter_threads,num_connect,num_response);
		//printf_statistic();
	}
	//record the results
	sprintf(path_result, "/home/xiangge/result/%s-%s-%d-%d-%d-%d-%d-%d", specification,ip_host, 1900 + pt->tm_year, 1 + pt->tm_mon, pt->tm_mday, pt->tm_hour, pt->tm_min, pt->tm_sec);	
	//record_result(path_result);
	printf_statistic();
	printf("\n All results have been recorded in file : %s", path_result);

        destroy_queues(queue_threads, num_threads);
        
	printf("\ncounter_threads=%d,num_connect =%d\n",counter_threads,num_connect);
}
