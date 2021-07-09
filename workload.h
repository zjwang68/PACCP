#ifndef YUNXIANG_WU_WORKLOAD__
#define YUNXIANG_WU_WORKLOAD__
int get_next_time_usec(double arate);
double gen_rand();
int get_size_flow_byte(int traffic_type);
double get_random();
int get_flow_deadline_ms(long int size_flow);
void init_sigaction(void);
void timer_interval(int usec);
#endif
