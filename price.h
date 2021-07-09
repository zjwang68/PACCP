
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <iostream>
#include "List.cpp"
#include <string.h>
#include <assert.h>

using namespace std;

//datacenter topology configuration
#define  NS  5               //Number of spine  switches
#define  NT  6               // Number of Top of rocks
#define  NH  40               //Number of leaf (server) nodes in a rock
#define  NL  240              //number of total leaf (server) nodes
#define  BC  400              //Banwidth(unit 100M) in core network between a spine and a tor
#define  BL  100              //Bandwith (unit 100M) between a tor and a host

//swicth queue sizes
#define  QD  48000           //max number of 100 bits in a core switch queue (400 pkts)
#define  QC  31200           //ECN enabled queue size of 100 bits in core switches (260 pkts)
#define  SQD  12000            //max number of 100 bits in a queue of core switches (100packets)
#define  SQC  7800             //ECN enabled queue size of 100 bits in core switches (65 packets)

//propagation delay in microsecond
#define t_lt     10         // micro seconds from host to tor
#define t_st     20         //microseconds from tor to spinw switch

//packet sizes in TCP flow
#define  DSZ     120        //max number of hundred bits in a data packet 
#define  asz     4          //number of hundred bits in an ACK packet

//TCP flow related parameters
#define tcp_rate   0.041     //tcp flow arrval rate in microseconds
#define IWD     2            //initial TCP window size
#define MAXF    10           //max number of concurret flows a host can send/receive
#define RP_ACK  3            //retranmist triggered by the number of repeated acks
#define MTSZ    200          //maxmimum number of TCP window size, i.e., full speed TCP window size
#define RTT     150          //Estimated initial round trip time (micro second) for a flow
#define  tm_data  1000       // time out time  of a data packet to retransmition (micro seconds) due to packet loss
#define tm_ack    400       // time out time of ACK (micro second)
#define route_timer 500     // path timeout 

//MRG flow related deadline/minimum rate paramters
#define sdead   1000      //small flow (size<=100KB) deadline (0.6 ms) 
#define mdead   1000     //mid flow (size<=10MB) deadline 

#define TRATE   2500     //big flow target rate ( unit in Mbps) 
#define SRATE   2500     //small flow target  rate (unit in Mbps)
#define MRATE   2500     //mid flow target rate (unit in Mbps)

//Ratio of different type of flows
#define CNT    100          //based value for ratio cacluation
#define PBE    60           //percentage of BE flows 
#define PDS    20           //percentage of DS flows
#define PMRG   20           //percentage of MRG flows

#define NBE    20           //a BE user if ID%NH<NBE 
#define NDS    30           //a DS user if ID%NH<NDS
#define NMRG   40           // a MRG user if ID%MRG<NMRG

//coefficient of MRG nad wieght 
#define rcos   3      //rcos for MRG users
#define WT     2      //weight value for DS users

//define an infinite number
#define INF    2000000000  //defined as infinite number


int     des_node[NL];       //flow destination of each host node
double  endtime=10000000.;        //the simulation time period (in microseconds)    

//parameters for queue measurement
double busy_time[NS][NT];     //record the busy time of each spine queue
double busy_start[NS][NT];    //record the period of start busy time
double overall_time[NS][NT];  //record the overall time of each spine queue

//count the send and acked packet in a flow
int sent_pkt[NL];    //number of pkt sent out
int sent_pkt1[NL];   //number of pkt acked


/************global parameters with for performance analysis******/
int tor_cong;                  //record #of congestions in tor
int tor_leaf_cong;             // record # of congestions in tor send to leaf
int spine_cong;                //record  #of congestions in spine
int leaf_cong;                 //record  #of congestions in leaf (number of retransmitions)
int out_order;                 //record # of pkts out of order
double tot_num_pkt;            //total # of pkts generated in simulation
int tot_flow_gen;              //total # of flows generated in simulation
double mrtt;                    //measured rtt time
int num_rtt;                   //number of times rtt measured
double mthd;                    //measured TCP threshold
int  num_mthd;                  //number of times mthd measured;
bool dest[NL][NL];               //source destination pair;    
int fnumber;                     //unique flow number for each flow
double flow_start_time[NL][NL];  //record the flow starting time
int flow_size[NL][NL];           //record the flow size
int max_send_flow;               //maximum number of send flow for a node
int max_recv_flow;               //maximum number of recv flow for a node
int numb_cong;                   


/**************************************************************
define struct of packet type: 0 data, 1 ack,  2 ack time out 
3: data retran time out tm: next process time
******************************************************************/
struct pkt {int src; int des; int type; int sz; double tm; int ID; int out; int in; int flow; int ecn; int SYN; int flowsize; int rbw;};

/****************************************************
Random double number generator Function between 0 and 1.
***************************************************/
double get_random()
{
  unsigned int xx;
  double y;

  y=(double)rand()/(double)RAND_MAX;
  if(y==1.0) { y-=0.0000000001;}

  return y;
}

/*****************************************************************
generate traffic  between 1M and 100 Mbytes
*******************************************************************/
int gen_traffic(int tp)
{
  int bits_num;
  double x,xx;
  
  //number of hundred bytes in a flow
   x=get_random(); xx=get_random();
   if(tp==0) {//generate self defined traffic, flow size equally get from 1 to SMALL # of 100bytes
      bits_num=1+(int)(100*xx);
  }
  else if(tp==1) //Web search distribution (unit 100 bits)
  {
     if(x<0.3) {bits_num=10+(int)(90*xx);} 
     else if(x<0.6) {bits_num=100+(int)(900*xx);}
     else if(x<0.75) {bits_num=1000+(int)(9000*xx);}
     else if(x<0.99) {bits_num=10000+(int)(90000*xx);}
     else {bits_num=100000+(int)(4000000*xx);}   	 
  }
  else if (tp==2) //data mining distribution
  { 
     if(x<0.4)      {bits_num=1+(int)(9*xx);} 
     else if(x<0.75) {bits_num=10+(int)(90*xx);}
     else if(x<0.85) {bits_num=100+(int)(9900*xx);}
     else if(x<0.99) {bits_num=10000+(int)(90000*xx);}
     else {bits_num=100000+(int)(4000000*xx);}  	
  }

   bits_num=bits_num*8; // cout<<"bits_num: "<<bits_num<<endl;
   
  return bits_num;
}


/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Next Request Generator.
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
double get_next_time(double arate)
{
   double  dice,tm;
   dice=get_random();
  if(dice==0.) { dice=10E-100;}
   tm=-log(dice)/arate;

   return tm;
}

/*****************************************
print pkacket information
*******************************************/
void print_pkt(pkt p)
{
  cout<<" pkt ID: "<<p.ID<<" Src: "<<p.src<<" Des: "<<p.des<<" out: "<<p.out<<" fnum: "<<p.flow<<" SYN: "<<p.SYN;
  cout<<" pkt Type: "<<p.type<<" time: "<<p.tm<<" Size: "<<p.sz<<" in: "<<p.in<<" ecn: "<<p.ecn<<" fsize: "<<p.flowsize<<endl;
}
