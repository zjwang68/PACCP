#include<errno.h>
#include<assert.h>
#include<string.h>
#include<sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
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

#define NUM_EVENTS 60000
#define MAX_EPOLL 10000
#define MAX_NUM_READ 65536
#define IP_SERVER "192.168.1.175"
#define PORT_SERVER 8887

int counter_flows=0;
int counter_connection=0;
int counter_receive=0;
int counter_send=0;

char ip_server[16] = IP_SERVER;
int port_server = PORT_SERVER;

struct flow_in
{
	int fd;
	int pos;
	int size;// flow size
};
struct statistic
{
//for statistic
	int num_connection;
	int num_receive;
	int num_send;

}statistic;

static struct flow_in * new_flow(int fd)
{
	struct flow_in *p;
	p = malloc(sizeof(struct flow_in));
	p->fd = fd;
	p->pos=0;

	return p;	
}

static int delete_flow(struct flow_in *p)
{
	free(p);
}
 
static void usage(const char* proc)
{
	assert(proc);
	printf("usage: %s [ip] [port]\n",proc);
}
 
static int set_nonblock(int fd)
{
	int fl = fcntl(fd,F_SETFL);
	fcntl(fd,F_SETFL,fl|O_NONBLOCK);
}
 
int my_read(int fd,char* buf,int len)
{
	assert(buf);
	ssize_t total = 0;
	ssize_t s = 0;
	while((s = read(fd,&buf[total],len - 1 - total)) > 0&&errno != EAGAIN)
	{
		total += s;
	}
 
	return total;
}
 
int start_up(char* ip,int port)
{
	assert(ip);
	assert(port > 0);
 
	int sock = socket(AF_INET,SOCK_STREAM,0);
	if(sock < 0)
	{
		perror("socket");
		exit(1);
	}

	statistic.num_connection=0;
	statistic.num_send=0;
	statistic.num_receive=0;
	
	struct sockaddr_in local;

	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	local.sin_addr.s_addr = inet_addr(ip);
 
	if(bind(sock,(struct sockaddr*)&local,sizeof(local)) < 0)
	{
		perror("bind");
		exit(2);
	}
 
	if(listen(sock,8000) < 0)
	{
		perror("listen");
		exit(3);
	}
 
	return sock;
}

int parse_configuration(int argc, char **argv)
{
        int opt, opt_idx, num_options;

#define OPT_LONG_IP_ADDRESS "ipServer"
#define OPT_SHORT_IP_ADDRESS 'i'
#define OPT_LONG_TCP_PORT "portServer"
#define OPT_SHORT_TCP_PORT 'p'

        static const struct option long_opt[] = {
                {OPT_LONG_IP_ADDRESS,     2, 0,  OPT_SHORT_IP_ADDRESS},
                {OPT_LONG_TCP_PORT,     2, 0,  OPT_SHORT_TCP_PORT},
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
                        case OPT_SHORT_IP_ADDRESS:
                                printf("\n%s=%s",OPT_LONG_IP_ADDRESS,optarg);
                                strcpy(ip_server, optarg);
                                num_options++;
                                break;
			case OPT_SHORT_TCP_PORT:
                                printf("\n%s=%s",OPT_LONG_TCP_PORT,optarg);
                                port_server = atoi(optarg);
                                num_options++;
                                break;
                        default:
                                printf("Unknow option!!!!see the specification below~\n");
                                specification_configuration();
                                exit(EXIT_FAILURE);
                }
        }
        printf("\nThe number of parameters in total: %d",num_options);
}

int specification_configuration()
{

        printf("\n%-30s%s","--ipServer=value or -i  value:","the IP address of TCP server.");
        printf("\n%-30s%s","--portServer=value or -p  value:","the TCP port of TCP server.");

        printf("\nA example: ./a.out --ipServer=192.168.1.19 --portServer=8887");
        return 0;
}
 
int main(int argc,char* argv[])
{
long int num_receive_bytes=0;
time_t time_start, time_now;

	parse_configuration(argc, argv);
	int listen_sock = start_up(ip_server, port_server);
	int epfd = epoll_create(MAX_EPOLL);
	if(epfd < 0)
	{
		perror("epoll_create");
		return 2;
	}
	
	struct epoll_event ev;
	struct flow_in *p;
	ev.events = EPOLLIN;
	ev.data.fd = listen_sock;
	epoll_ctl(epfd,EPOLL_CTL_ADD,listen_sock,&ev);
	int nums = 0;
 
	struct epoll_event ready_events[NUM_EVENTS];
	int timeout =0; //-1;
	
time(&time_start); 
	while(1)
	{
time(&time_now); 
	
		switch(nums = epoll_wait(epfd,ready_events,NUM_EVENTS,timeout))
		{
			case 0:
				if((time_now-time_start)>3)
				{
					time_start=time_now;
					//printf("\ntimeout..connection=%d,receive=%d,send=%d, receive_bytes=%ld",statistic.num_connection,statistic.num_receive, statistic.num_send, num_receive_bytes);
				}
				break;
			case -1:
				perror("epoll_wait");
				break;
			default:
					time_start=time_now;
					int i = 0;
					for(;i < nums; i++)
					{
						int fd = ready_events[i].data.fd;
						if(fd == listen_sock && ready_events[i].events & EPOLLIN)
						{
							struct sockaddr_in client;
							socklen_t len = sizeof(client);
							int new_fd = accept(listen_sock,(struct sockaddr*)&client,&len);
							if(new_fd < 0)
							{
								perror("accept");
								continue;
							}
 
							//printf("get a new client:%s:%d\n",inet_ntoa(client.sin_addr),ntohs(client.sin_port));
							p = new_flow(new_fd);
 
							ev.events = EPOLLIN/*|EPOLLET*/;//2018-8-27
							ev.data.ptr = p;
							p->fd = new_fd;
							p->pos = 0;
							statistic.num_connection++;
							set_nonblock(new_fd);
							epoll_ctl(epfd,EPOLL_CTL_ADD,new_fd,&ev);
						}
						else//packet receiving and sending
						{
							
							p = ready_events[i].data.ptr;
							fd = p->fd;
							if(ready_events[i].events & EPOLLIN)
							{
								char buf[MAX_NUM_READ];

								ssize_t s = read(fd,buf,MAX_NUM_READ);
								if(s > 0)
								{
								num_receive_bytes+=s;
									if(p->pos == 0)
									{
										p->size = ntohl(*((long int *)(buf)));
									}

									p->pos +=s;
									//buf[s] = 0;
									//printf("client recieve bytes=%d/%ld #\n",p->pos,p->size);
									if(p->pos == p->size)
									{
										ev.events = EPOLLOUT/*|EPOLLET*/;
										ev.data.ptr = p;
										epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&ev);
										statistic.num_receive++;
										counter_flows++;
									}

									//printf("pos=%d,size=%d",p->pos,p->size);
								}
								else if(s == 0)
								{
									epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
									close(fd);
									delete_flow(p);
									perror("\n------------------s====0---------------client close...counter_flows=\n");
								}
								else
								{
									perror("i--------------------------------read");
								}
							}
							else if(ready_events[i].events & EPOLLOUT)
							{
								char buf[1024];
								int i;
								//sprintf(buf,"HTTP/1.0 200 OK\r\n\r\n<html><h2>hello</h2></html>");
								//sprintf(buf,"HTTP/1.0 200 OK\r\n\r\n<html><h2>hello</h2></html>");
								sprintf(buf,"yes");
								i =write(fd,buf,sizeof(buf));
								if(i <= 0)
									perror("\nwrite<0");
								//printf("\nsend y\n");
								//write(fd,buf,strlen(buf));

								statistic.num_send++;
								epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
								close(fd);
								delete_flow(p);
								//printf("\nclient close.++++..counter_flows=%d\n",counter_flows);
							}
							else
							{}
						}
					}
		}
	}
	return 0;
}

