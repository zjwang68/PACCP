
#include "price.h"

template <class DType>
class leaf
{
  public:
                leaf();
	        ~leaf();
                void SetUp();           //set up some parameters
                void Set_ID(int n);     //set the ID of the leaf node
                int Get_ID();           //Get the ID of leaf node
		void Set_Traf_rate(double arate);  //set the traffic rate linked to TOR
	        double Traf_rate();     //return the Traf rate linked to the TOR
                void SetTOR(int n);     //link the leaf node to a TOR
                int TOR_ID();              //show the TOR ID linked to the leaf node
                void Set_Prog_delay(double delay);  //set propagation delay from TOR
                double Prog_delay();   //return propagation delay from TOR
                void Set_pkt_size(int n); //set max packet size 
                int  Pkt_size();         //show pmax pkt size
		bool IsDes(int ds);     //return true if des is not a recving flow
	        void  Bit_Recv(int n, int sr, int fnum);  //set number of bits to be sent for sending to source sr
                int Bit_Sent(int ds);           // show the number of bits to send for destination ds  
                int Num_Send_Flow();             // return number of recv flows
                int Num_Recv_Flow();             // return number of send flows		
                //initail TCP traffic for destination ds at time t with qostype (0, no deadline, 1 with deadline)				
                bool Initial_TCP(double t, int ds, int nbits); 
                void End_TCP(int n, double tm, int fl);         //end of a TCP session
                bool Send_pkt_TCP(pkt p, int fl);     //send out a packet to des use TCP
                int Recv_pkt_TCP(pkt p);  //receive a packet from des using TCP
                double NextTime();         //return next event time;
                void AddTimeList(pkt p);   //add pkt into event list
                int Run_Leaf(pkt &p);       
		void SetHost(int n);       //Set the host class 
               //Run events in leaf node 1: send pkt 2: rec pkt 3: timeout
                void Show_Data();            //show all data in the leaf node    
                int HostClass();            //return hostclass				

    private:
                int id;              //ID of the leaf node
                int pkt_sz;          //maxumum packet size 
	        double  traf_rate;   // line rate trafic generate rate
                double prog_delay;    // propagation delay from TOR
                int tor_num;         // the  ID of TOR connected to the leaf node
				int hostclass;       //the class of hosts: 1: BE 2: DS and 3 MRG
				
		//for sending TCP parameters
		int bit_num[MAXF];         // the number of pkts to send
                int bit_sent[MAXF];        // number of pkts sent out 
	        int tcp_wd_sz[MAXF];      // tcp sending window size 
                int bit_acked[MAXF];     //tcp acked receiving bits number 
		int tcp_thd[MAXF];        //TCP window size threshold 
                int tcp_cong[MAXF];       //TCP congestion avoidance increase factor, record ack# received 
	        int last_pkt_sz[MAXF];          //the last pkt size
	        int retran_ID[MAXF];      //record the last retransmission pkt ID
	        double retran_time[MAXF];  //record the last retransmission pkt time
		int rep_ack[MAXF];        //number of repeated ack 
		int ecn_count[MAXF];       //count # of ecn bits in a RTT
		double alpha[MAXF];          //alpha value of d2tcp and dctcp
		double deadtime[MAXF];       //deadline setup for qos flow
		double avg_rtt[MAXF];      //average measured rtt;
		 
		//for receving TCP parameters
        int bit_received[MAXF];    //last bit received for receiving TCP
        int tcp_ack_num[MAXF];     // tcp acked received byte
        int bit_to_rec[MAXF];      //expected bits to receive for receving TCP
        int send_flow_id[MAXF];     //the flow id of sendng flow
        int recv_flow_id[MAXF];     //the flow id of receiving flow
        int send_flow[MAXF];      //sending flow number mapping to destination
	bool ecn_flag[MAXF];       // enc flag indicating most recent ack is ecn enabled or not
				
	int recv_flow[MAXF];      //receiving flow number mapping to source
  	int num_send_flow;        //number of active sending flow
	int num_recv_flow;        //number of active receving flows
        double nexttime;    //time for next event
	double cong_time[MAXF];   //last congestion time for a flow
        List<int> ASF;      //List all available flow array slot for sending flow
        List<int> ARF;       //List all available flow array slot for receiving flow
        List<pkt> TM;  
        //timer list of event (0: send packet 1: receive packt 2: data time out 3:ack timeout)
        List<int> MP[MAXF];  //stor missed pkt number
       List<pkt>  dataq;     // doutgoing data queue for sending data
};

template <class DType>
leaf<DType>::leaf()
{ 
    traf_rate=prog_delay=0; id=pkt_sz=tor_num=0; 
    TM; dataq;  nexttime=INF; 
    //SetUp();
}


template <class DType>
leaf<DType>::~leaf()
{
  traf_rate=prog_delay=0; id=pkt_sz=tor_num=0; num_send_flow=num_recv_flow=0;
    for(int i=0;i<MAXF; i++) 
  { bit_num[i]=bit_sent[i]=bit_received[i]=bit_acked[i]=bit_to_rec[MAXF]=0; 
    alpha[i]=0; retran_ID[i]=0; retran_time[i]=0;
    tcp_wd_sz[i]=tcp_ack_num[i]=rep_ack[i]=tcp_thd[i]=tcp_cong[i]=ecn_count[i]=0; ecn_flag[i]=false;
    send_flow[i]=recv_flow[i]=send_flow_id[i]=recv_flow_id[i]=0; MP[i].kill(); last_pkt_sz[i]=0; 
  }
  TM.kill(); ASF.kill(); ARF.kill(); dataq.kill();  nexttime=INF; 
}

template <class DType>
void leaf<DType>::SetUp()
{
  for(int i=0;i<MAXF; i++) 
  { bit_num[i]=bit_sent[i]=bit_received[i]=bit_acked[i]=bit_to_rec[i]=0; 
    alpha[i]=0; retran_ID[i]=0; retran_time[i]=0; avg_rtt[i]=0;
    tcp_wd_sz[i]=tcp_ack_num[i]=rep_ack[i]=tcp_thd[i]=tcp_cong[i]=ecn_count[i]=0; ecn_flag[i]=false; 
    send_flow[i]=recv_flow[i]=send_flow[i]=recv_flow_id[i]=-1; last_pkt_sz[i]=0; MP[i];
  }
 for (int i=0; i<MAXF; i++) {ASF.EnQueue(i); ARF.EnQueue(i);}
  num_send_flow=num_recv_flow=0;
  
}

template <class DType>
void leaf<DType>::Set_ID(int n) { id=n;}

template <class DType>
void leaf<DType>::SetHost(int n) {hostclass=n;}

template <class DType>
int leaf<DType>::Get_ID() { return id;}

template <class DType>
void leaf<DType>::Set_Traf_rate(double arate) {traf_rate=arate; }

template <class DType>
double leaf<DType>::Traf_rate()
{ return traf_rate;}

template <class DType>
void leaf<DType>::SetTOR( int n)
{ tor_num=n; }

template <class DType>
int leaf<DType>::TOR_ID() { return tor_num;}

template <class DType>
int leaf<DType>::HostClass() { return hostclass; }

template <class DType>
void leaf<DType>::Set_Prog_delay(double d) { prog_delay=d;}

template <class DType>
double leaf<DType>::Prog_delay() { return prog_delay; }

template <class DType>
void leaf<DType>:: Set_pkt_size( int n) { pkt_sz=n;}

template <class DType>
int leaf<DType>::Pkt_size() { return pkt_sz;}

template <class DType>
double leaf<DType>::NextTime() {return nexttime;}

template <class DType>  //return true if ds is on the recv_flow table
bool leaf<DType>::IsDes(int sr) 
{ for(int i; i<MAXF; i++) { if(recv_flow[i]==sr) return true;}
   return false;
}

template <class DType>
int leaf<DType>::Num_Send_Flow() { return num_send_flow;}

template <class DType>
int leaf<DType>::Num_Recv_Flow() { return num_recv_flow;}

//exchange the flow number and number of bits to send between source and destionation
template <class DType> //set the number of bytes to recieve for source node src
void leaf<DType>::Bit_Recv(int n, int sr, int fnum)
{ int i,j;
  
  for(j=0;j<MAXF; j++) {
     if(recv_flow_id[j]==fnum) { return; }  
  }

  if(!ARF.Empty()){ ARF.SetToTail(); i=*ARF.Access(); ARF.Remove();  
     bit_to_rec[i]=n; recv_flow[i]=sr; recv_flow_id[i]=fnum;  num_recv_flow++;
     bit_received[i]=0; tcp_ack_num[i]=0;  ecn_flag[i]=false; //initial receiving state
     if(num_recv_flow>max_recv_flow) { max_recv_flow=num_recv_flow;} 
  }
 }

template <class DType> //return the number of bytes to send for destination ds
int leaf<DType>::Bit_Sent(int ds) 
{ for(int i=0;i<MAXF;i++) { if(send_flow[i]==ds) { return bit_num[i];} } }

//start a TCP session at tm
template <class DType>
bool leaf<DType>::Initial_TCP( double tm, int ds, int nbits)
{ pkt p;
  int i, last;
  bool bl;

  bl=true;
  if(num_send_flow>=MAXF) { return false; }
  if(ASF.Empty()) { return false;}
  
  //find the array at i entry to store flow information 
  ASF.SetToHead(); i=*ASF.Access(); send_flow[i]=ds; ASF.Remove();
  tot_num_pkt+=nbits; tot_flow_gen++; //record the total traffic
  send_flow_id[i]=fnumber;  fnumber++; //set unique flow id

  //initial TCP sending parameters
  tcp_wd_sz[i]=IWD;  if(hostclass>0) { tcp_wd_sz[i]=IWD*2;}
  last=nbits%pkt_sz; alpha[i]=0; ecn_count[i]=0;
  retran_ID[i]=0; retran_time[i]=0; avg_rtt[i]=RTT;
  if(last==0) { bit_num[i]=int(nbits/pkt_sz);  last_pkt_sz[i]=pkt_sz; }
  else { bit_num[i]=int(nbits/pkt_sz)+1; last_pkt_sz[i]=last; }
  if(last<=4) { last_pkt_sz[i]=4;}
  bit_sent[i]=0; tcp_thd[i]=INF; tcp_cong[i]=0; rep_ack[i]=0; num_send_flow++;
  cong_time[i]=0; 
 

  if(hostclass==2) { //set up deadline of qos flow
      if(nbits<=8000) { deadtime[i]=tm+sdead-RTT; } //+100.*nbits/SRATE; } //size of 100KBytes
	  else if(nbits<=160000) {deadtime[i]=tm+mdead-RTT+100.*nbits/MRATE; } //size of 1Mbytes
	  else { deadtime[i]=tm+100.*nbits/TRATE-RTT; }    //set TRATE as traget rate, change to deadline in micro second
  }
  else { deadtime[i]=INF; }  //no deadline flow, set deadline to INF
  
  if(max_send_flow<num_send_flow) { max_send_flow=num_send_flow; }

  //add first data pkt to out going queue
  p.type=0; p.src=id; p.des=ds; p.tm=tm+pkt_sz/traf_rate; p.ecn=0;
  p.ID=0; p.sz=asz;  p.SYN=1; p.flowsize=bit_num[i];
  //if(p.ID==bit_num[i]) {p.sz=last_pkt_sz[i]; }
  p.out=tor_num; p.in=id; p.flow=send_flow_id[i];  
  bl=Send_pkt_TCP(p, i);

  return bl;
}

template <class DType> //finish a sending (n=1） or a receving flow (n=0) at time tm 
void leaf<DType>::End_TCP( int n, double tm, int f) 
{ 
  pkt pk;
  int i,key, sr,ds,fl,index;
  double xx;
  List<pkt> cp;

  if(n==0) { //end a receiving flow
    ecn_flag[f]=false;
    tcp_ack_num[f]=0;  bit_received[f]=0; bit_to_rec[f]=0; fl=recv_flow_id[f]; 
    MP[f].kill(); num_recv_flow--; recv_flow[f]=-1; recv_flow_id[f]=-1; ARF.EnQueue(f);
   }
  else if(n==1) { //end a sending flow
     //for(i=0;i<6;i++) {if(bit_num[f]*pkt_sz+last_pkt_sz[f]<=fsc1[i]) { lfnumb[i]--;} }
	 alpha[f]=deadtime[f]=retran_time[f]=0; ecn_count[f]=retran_ID[f]=0; avg_rtt[i]=0;
	 tcp_wd_sz[f]=0; bit_acked[f]=0; bit_num[f]=0; bit_sent[f]=0;  last_pkt_sz[f]=0;
         tcp_thd[f]=0; tcp_cong[f]=0; rep_ack[f]=0; num_send_flow--; fl=send_flow_id[f];
         send_flow[f]=-1; send_flow_id[f]=-1; ASF.EnQueue(f); //tot_lfn--;
   }

    index=0;
    if(!TM.Empty()) { //remove all even for the flow except the receving data/ack/congestion
                     cp.kill(); TM.SetToHead(); pk=*TM.Access(); 
                     if(pk.flow!=fl || pk.type==4 || pk.type==5 || pk.type==8 ) 
                     {  cp.EnQueue(pk); if(pk.type==0 || pk.type==1) {index=1;} }
		     while(TM.Fore()) {pk=*TM.Access(); 
			   if( pk.flow!=fl || pk.type==4 || pk.type==5 || pk.type==8) 
			   { cp.Insert(pk); if(pk.type==0 || pk.type==1) { index=1;} }
            }
                     TM.kill(); TM=cp; cp.kill();
       }

    if(!dataq.Empty()) { //remove all send pkts belong to the ended flow from queue 
              cp.kill();  dataq.SetToHead(); pk=*dataq.Access(); 
              if(pk.flow!=fl) { cp.EnQueue(pk);}
              while(dataq.Fore()) { pk=*dataq.Access(); if(pk.flow!=fl) { cp.Insert(pk); } }
              dataq.kill(); dataq=cp; cp.kill();
     }
   
    
    if(n==1 && index==0 && !dataq.Empty()) 
    { dataq.SetToHead(); pk=*dataq.Access(); pk.tm=tm+pk.sz/traf_rate; AddTimeList(pk); }
    
}


template <class DType>
void leaf<DType>::AddTimeList(pkt p)
{
  double tm, tim;
  int tp;
  tm=p.tm;

  if(TM.Empty()) { TM.EnQueue(p); nexttime=tm; return; }

  TM.SetToHead(); tim=TM.Access()->tm;
  tp=p.type;
 
  if(tp==0 || tp==1)  { //send data, search from head
      if(tm<tim) { TM.EnQueue(p); nexttime=tm; return;}
      while(TM.Fore()) {
                if(tm<TM.Access()->tm) {TM.Back(); TM.Insert(p); nexttime=tim; return;}
             } 
        TM.Insert(p); nexttime=tim; return;             
  }
  else { //other event, search from tail
     TM.SetToTail();
     if(tm>=TM.Access()->tm){ TM.Insert(p); nexttime=tim;  return;}
     while(TM.Back()){ if(tm>=TM.Access()->tm) {TM.Insert(p);  nexttime=tim; return; }}
     TM.EnQueue(p);  nexttime=tm;
    }
  
}

template <class DType> //send pkt stored in array entry n
bool leaf<DType>::Send_pkt_TCP(pkt p, int n)
{ 
   int k, num;
   
   k=p.type;
  if(k==1) {  if(dataq.Empty()) {dataq.EnQueue(p);  AddTimeList(p); }
              else { dataq.SetToTail();  dataq.Insert(p); }
	      return true;
	   } //sending ack pkt

   num=tcp_wd_sz[n]-(bit_sent[n]-bit_acked[n]);
   if(bit_num[n]<=bit_sent[n]) {  return false; } //No data to sent

  if(num>0 ) { //send  data packet sucessfully
       if(dataq.Empty()) {dataq.EnQueue(p);  AddTimeList(p);} //  cout<<"here 14"<<endl; print_pkt(p);} 
       else { dataq.SetToTail();  dataq.Insert(p);}// cout<<"Here 15"<<endl;} 
	   
       return true;
   }

  return false;  //fail to send pkt

}

template <class DType>
int leaf<DType>::Recv_pkt_TCP(pkt p)
{
   int n,nn,sr,ds,ff,acknumb,mrate, weit;
   int i,j,k,key,index,ii;
   double frac, dvalue, pvalue, gvalue, tc, tm,tt;
   bool ackflag;
   pkt pp, pk, pck;
   List<pkt>  cp;

   n=p.type; nn=p.ID; gvalue=1./16.;
   sr=p.src; ds=p.des;  ff=-1;
   tm=p.tm;
   
   for(i=0;i<MAXF;i++) //find the pkt belong to a flow
   { if(n==4) { if(recv_flow[i]==sr) { ff=i; if(recv_flow_id[ff]!=p.flow){ cout<<"wrong flow"<<endl; return 0;} 
                                             else {break;} 
                                      } //recv data pkt
     }
    else if(n==5){ if(send_flow[i]==sr) {ff=i; if(send_flow_id[ff]!=p.flow) { cout<<"Wrong flow"<<endl; return 0;}
                                             else { break;}
                                         } //recv ack pkt
         }
   }
   
   // cout<<" ffffffffffffffffff"<<ff<<endl<<endl<<endl;
   if(ff==-1) { cout<<" The pkt does not belong any flow"<<endl; return 0;} //the pkt does not belong to any flow
   
   if(p.SYN==1) //send SYN+ack
   { p.src=ds; p.des=sr; p.type=1; p.sz=asz; p.out=tor_num; p.in=id; p.ecn=0; p.SYN=2; p.ID=0;
     p.tm=tm+p.sz/traf_rate; Send_pkt_TCP(p,ff); 
     return 10;
    }
	
   if(p.SYN==2) //acked SYN, send first data pkt
   { p.type=0; p.src=ds; p.des=sr; p.ecn=0; p.SYN=0;
     p.ID=1; p.sz=pkt_sz; if(p.ID==bit_num[ff]) {p.sz=last_pkt_sz[ff]; }
     p.tm=tm+p.sz/traf_rate;  p.out=tor_num; p.in=id; p.flow=send_flow_id[ff];  
	 //remove SYN timer
	 
     if(!TM.Empty()){ TM.SetToHead(); pp=*TM.Access(); //remove SYN retrans timer 
            if( pp.flow==p.flow && pp.type==2 && pp.SYN==1 ) 
            {  TM.Remove(); }//remove all retran  data pkt
            else { while(TM.Fore()) { pp=*TM.Access();
                       if( pp.flow==p.flow && pp.type==2 && pp.SYN==1 ) 
                       {  TM.Remove(); break; }
		           }
            }
       }

         Send_pkt_TCP(p, ff);
	 return 10;
   } 

   if(ds!=id) { cout<<"Get a wrong des packet for leaf node!!"<<id<<endl; return 0;}
 
   key=1;
   if(n==4)  {//data packet
     if(bit_to_rec[ff]==0) { return 0;} //unknow pkt, drop it
     if(p.ecn==0) { if(ecn_flag[ff]==true) { ackflag=true; ecn_flag[ff]=false; } //need to send ack with ecn bit changed
	                else { ackflag=false;}
     }
      else { if(ecn_flag[ff]==true) { ackflag=false; }
             else { ackflag=true; ecn_flag[ff]=true; }  //need to send ack with ecn bit changed
     }
     if(nn==bit_received[ff]+1) {//in order  data pkt
           if(tcp_ack_num[ff]==bit_received[ff] && nn<bit_to_rec[ff]) { //in order pkt 
		        if(ackflag==false) { //no ack need to send
		           bit_received[ff]=nn; 
                   p.tm=tm+tm_ack; p.type=3;
                   p.src=ds; p.des=sr; p.sz=asz; p.out=tor_num; p.in=id; p.SYN=0;
		           AddTimeList(p); //set timer for sending ack;
				}
				else { //send ack with ecn bit change state, ecn bit as itself
  				    p.src=ds; p.des=sr; p.type=1; p.sz=asz; p.out=tor_num; p.in=id; p.SYN=0;
                    p.tm=tm+p.sz/traf_rate; p.SYN=0; Send_pkt_TCP(p,ff);
                    tcp_ack_num[ff]=nn;  	bit_received[ff]=nn; 
				}
                 return key;
           }
           else if(tcp_ack_num[ff]==bit_received[ff]-1 || (tcp_ack_num[ff]==bit_received[ff] && nn>=bit_to_rec[ff])) 
           {//the second unacked data pkt or the last data pkt, send ack out now
             p.src=ds; p.des=sr; p.type=1; p.sz=asz; p.out=tor_num; p.in=id; 
             p.tm=tm+p.sz/traf_rate; p.SYN=0;
		 if(ackflag==true) 
		 { pck=p; pck.ID=p.ID-1; pck.ecn=(p.ecn+1)%2; Send_pkt_TCP(pck,ff);} //send 2 ack pkts due to ecn state change
			 
             if(!TM.Empty()){  //remove ack timer 
               TM.SetToHead(); pp=*TM.Access();
               if(pp.type==3 && pp.flow==p.flow && pp.ID==bit_received[ff]) {TM.Remove();}
               else { 
                  while(TM.Fore()) {pp=*TM.Access(); 
                    if(pp.type==3 && pp.flow==p.flow && pp.ID==bit_received[ff]) {TM.Remove(); break;}
                  }
                }
             }
	         Send_pkt_TCP(p,ff);
             tcp_ack_num[ff]=nn; bit_received[ff]=nn; 
             return key;
          }
          else { //resent  last ack, some pkt missed, ecn bit as it is
            bit_received[ff]=nn; p.src=ds; p.des=sr; p.type=1; p.sz=asz; p.out=tor_num; p.in=id; 
            p.ID=tcp_ack_num[ff];  p.tm=tm+p.sz/traf_rate; p.SYN=0;
            if(MP[ff].Empty()) { p.ID=nn; tcp_ack_num[ff]=nn;}//no missed pkt
            Send_pkt_TCP(p,ff);
            return key;
         }
      }//end if(nn==bit_received+1
    else { //out of order pkt 
          p.src=ds; p.des=sr; p.type=1; p.sz=asz; p.out=tor_num; p.in=id; 
          p.tm=tm+p.sz/traf_rate; p.SYN=0; p.ID=tcp_ack_num[ff];

          if(nn>bit_received[ff]+1){ //some pkt missed from last received pkt
               if(MP[ff].Empty()) { p.ID=bit_received[ff]; 
                 if( tcp_ack_num[ff]==bit_received[ff]-1) { tcp_ack_num[ff]=bit_received[ff]; 
                    if(!TM.Empty()) { TM.SetToTail(); pk=*TM.Access();   //remove ack timer
                        if(pk.ID==p.ID && pk.type==3 && pk.flow==p.flow) {TM.Remove();}
                        else { while(TM.Back()) { pk=*TM.Access(); 
                                  if(pk.ID==p.ID && pk.type==3 && pk.flow==p.flow) { TM.Remove(); break;} }
                             }
                        }
                  }
               }
               else {p.ID=tcp_ack_num[ff]; }

              for(k=bit_received[ff]+1; k<nn; k++) { MP[ff].EnQueue(k);} //add missed pkt to list
              Send_pkt_TCP(p,ff); bit_received[ff]=nn;
              return key;
           }
        else { //recover a missed pkt or receive a repeated data ptk
             if(nn<=tcp_ack_num[ff]) { return key;}
             if(MP[ff].Empty()) {// repeated data packet, missed ack, resend ack
                if(tcp_ack_num[ff]<bit_received[ff]) {
                  tcp_ack_num[ff]=bit_received[ff];  p.ID=tcp_ack_num[ff]; 
                  if(!TM.Empty()) { TM.SetToTail(); pk=*TM.Access(); //remove ack timer
                         if(pk.ID==p.ID && pk.type==3 && pk.flow==p.flow) { TM.Remove(); }
                         else { while(TM.Back()) { pk=*TM.Access(); 
                                   if(pk.ID==p.ID && pk.type==3 && pk.flow==p.flow) { TM.Remove(); break;} }
                        }
                   }//end if(!TM.Empty())
                }
              }
              else { out_order++; 
                 MP[ff].SetToHead();  //remove the missed pkt from MP list
                 if(nn==*MP[ff].Access()) { MP[ff].Remove();}
                 else { while(MP[ff].Fore()) { if(nn==*MP[ff].Access()) {MP[ff].Remove(); break;}}}

                 if(MP[ff].Empty()) {//no more missed pkt, gap is filled
                        tcp_ack_num[ff]=bit_received[ff];  p.ID=bit_received[ff];}
                 else { MP[ff].SetToTail(); k=*MP[ff].Access();
                        if(k>nn) { tcp_ack_num[ff]=k-1;}
                        p.ID=tcp_ack_num[ff]; 
                      }
               }

              Send_pkt_TCP(p,ff);
              return key;
         }
       }
   }//end if (n==4)
   else  if(n==5){ //ack packet
        if(bit_num[ff]==0 ) { return 0;}
        if(nn>=bit_num[ff]) //normal act with temination
		{ End_TCP(1, p.tm, ff);  ii=nn-bit_acked[ff]; sent_pkt1[id]+=ii;  dest[ds][sr]=false; key=2; return key;} 
        
	 //set target rate for qos flow
	// if(deadtime[ff]<tm || deadtime[ff]==INF) { mrate=0; } //BE flow or qos flow with passed deadline 
    // else { mrate=(int) (((bit_num[ff]-nn-1)*pkt_sz+last_pkt_sz[ff])*avg_rtt[ff]/pkt_sz/(deadtime[ff]-tm)); }
   if(hostclass==0) { weit=1; mrate=0; }
   else if(hostclass==1)  {weit=WT; mrate=0;}
   else { weit=1; mrate=(int) (((bit_num[ff]-nn-1)*pkt_sz+last_pkt_sz[ff])*avg_rtt[ff]/pkt_sz/(deadtime[ff]-tm));}
   //else { weit=1; mrate=(int)(TRATE*avg_rtt[ff]*1.0/pkt_sz/100); }
	 
        if(nn<bit_acked[ff]) { return key; } //discard the repeated ack
        else if(nn==bit_acked[ff]) { //repeated ack
             if(rep_ack[ff]<RP_ACK-1) //no retransimision yet
			 {rep_ack[ff]++;  return key; }  
            else {
                    //check if the  pkt transmission time is less RTT
                      if(!TM.Empty()) {TM.SetToTail(); pk=*TM.Access();
                          if(pk.ID==nn+1 && pk.flow==p.flow) {if((tm-pk.tm+tm_data)<avg_rtt[ff]) { rep_ack[ff]=0; return key;}}
                          while(TM.Back()) {pk=*TM.Access();
                            if(pk.ID==nn+1 && pk.flow==p.flow) {if((tm-pk.tm+tm_data)<avg_rtt[ff]) { rep_ack[ff]=0; return key;}}
                            } 
                      }
                     
                    //retransmit data pkt 
                    rep_ack[ff]=0; p.src=ds; p.des=sr; p.type=0;  leaf_cong++;//retramisson data pkt
                    if(bit_acked[ff]==bit_num[ff]-1) {p.sz=last_pkt_sz[ff]; }
                    else { p.sz=pkt_sz;}					
		    p.ID=bit_acked[ff]+1; p.tm=tm+p.sz/traf_rate; 
                    p.out=tor_num; p.in=id; p.ecn=0; p.SYN=0;
					
		    //window size adjusted due to three repeated acks
		    if(tcp_wd_sz[ff]<tcp_thd[ff]) { tcp_wd_sz[ff]=(int)(tcp_wd_sz[ff]*(1-pow(2,-weit))); }
                    else { tcp_wd_sz[ff]=(int)(tcp_wd_sz[ff]/2+(weit-1));}  
                    if(tcp_wd_sz[ff]<IWD) {tcp_wd_sz[ff]=IWD;}
                    tcp_thd[ff]=tcp_wd_sz[ff];       tcp_cong[ff]=0; 
	            bit_sent[ff]=bit_acked[ff];
				  		  
                  if(!TM.Empty()) { index=0;    //set for TM list check if any data in outgoing queue
                   //remove all data  pkt and data retransmission timer
                     cp.kill(); TM.SetToHead(); pk=*TM.Access(); k=pk.type;
                     if(k==1 ||k==3 ||  k==4 || k==5 || pk.flow!=p.flow)
                      { cp.EnQueue(pk); if(k==0 || k==1) { index=1;} }
		          while(TM.Fore()) {pk=*TM.Access(); k=pk.type;
                       if(k==1 || k==3|| k==4 || k==5 || pk.flow!=p.flow) 
                       { cp.Insert(pk); if(k==0 || k==1) {index=1;} }
                     }
                     TM.kill(); TM=cp; cp.kill();
                    }

                   //remove the data pkt from dataq
                   if(!dataq.Empty()) {dataq.SetToHead(); pk=*dataq.Access();
                      if(pk.flow==p.flow) { dataq.Remove();}
                      else { while(dataq.Fore()) { if(pk.flow==p.flow) { dataq.Remove(); break;} } }
                   }

                  if(index==0 && !dataq.Empty()) { dataq.SetToHead(); pk=*dataq.Access();
                    pk.tm=p.tm+pk.sz/traf_rate; AddTimeList(pk);}
                  
                 Send_pkt_TCP(p,ff); 
                 
                  return key;
              }
           }//end if(nn==bit_acked)
        else {  //normal ack packet adjust sending window size
	          ii=nn-bit_acked[ff]; sent_pkt1[id]+=ii;
	          if(tcp_wd_sz[ff]<tcp_thd[ff]) { //slow start
		      if(tcp_wd_sz[ff]<mrate) { tcp_wd_sz[ff]+=(int)(ii*(2*rcos-pow(2,-weit+1))+get_random()); }
                      else { tcp_wd_sz[ff]+=(int)(ii*(2-pow(2, -weit+1))+get_random()); }
		}					  
              else { //congestion avoidance phase
	         if(tcp_wd_sz[ff]<mrate) { tcp_wd_sz[ff]+=(int)(ii*(rcos-1)+get_random()); 
                       if(tcp_wd_sz[ff]>mrate) { tcp_wd_sz[ff]=mrate;}
                  }
                 else {  tcp_cong[ff]+=ii;                
                          if(tcp_cong[ff]>=tcp_wd_sz[ff]) { tcp_cong[ff]-=tcp_wd_sz[ff]; tcp_wd_sz[ff]+=weit; }
                        }	
	            }
				
	        if(tcp_wd_sz[ff]>MTSZ) { tcp_wd_sz[ff]=MTSZ; }
                rep_ack[ff]=0; bit_acked[ff]=nn;
                if(bit_sent[ff]<nn) {bit_sent[ff]=nn;}

                cp.kill();
                if(!TM.Empty()){TM.SetToHead(); pp=*TM.Access(); //remove retrans timer id<=nn 
                    if(pp.ID==nn && pp.flow==p.flow && pp.type==2) //update rtt
					{  tt=tm-pp.tm+tm_data;  mrtt+=tt; num_rtt++;
                       if(avg_rtt[ff]==0) { avg_rtt[ff]=tt;}
                       else { avg_rtt[ff]=(avg_rtt[ff]+tt)/2.; }
				    } 
                    index=0;
                    if(pp.ID>nn || pp.flow!=p.flow || pp.type==4 || pp.type==5 ) 
                    {cp.EnQueue(pp); if(pp.type==0 || pp.type==1) { index=1;}}//remove all retran  data pkt
                    while(TM.Fore()) { pp=*TM.Access();
                       if(pp.ID==nn && pp.flow==p.flow && pp.type==2) 
					   { tt=tm-pp.tm+tm_data;  mrtt+=tt; num_rtt++;
                         if(avg_rtt[ff]==0) { avg_rtt[ff]=tt;}
                         else { avg_rtt[ff]=(avg_rtt[ff]+tt)/2.; }
				       } //update RTT
                       if( pp.ID>nn || pp.flow!=p.flow || pp.type==4 || pp.type==5 ) 
                       { cp.Insert(pp); if(pp.type==0 || pp.type==1) {index=1;} }
                     }
                       TM. kill(); TM=cp; cp.kill();
                    }

               //remove the transmiting data from dataq with id<=nn
               if(!dataq.Empty()){dataq.SetToHead(); pp=*dataq.Access();
                   if(pp.ID<=nn && pp.flow==p.flow) { dataq.Remove();}
                   else { while(dataq.Fore()) 
                            { pp=*dataq.Access(); if(pp.ID<=nn && pp.flow==p.flow) { dataq.Remove(); break;} }
                        }
                }

               //add send pkt to queue if no pkt in the queue
               if(bit_sent[ff]>=bit_num[ff]) { return true;}
               if(index==0 && !dataq.Empty()) { dataq.SetToHead(); pk=*dataq.Access();  
                   pk.tm=p.tm+p.sz/traf_rate; AddTimeList(pk); }

                //chck if the flow has a data pkt in the queue or not
		   if(!dataq.Empty()) { dataq.SetToHead(); pp=*dataq.Access(); 
                    if( pp.flow==p.flow) { return key;}
                    while(dataq.Fore()){  pp=*dataq.Access(); if(pp.flow==p.flow) {return key;} }
                }

               p.src=ds; p.des=sr; p.type=0; p.ecn=0; p.SYN=0;
	       if(bit_sent[ff]==bit_num[ff]-1) { p.sz=last_pkt_sz[ff];}
           else { p.sz=pkt_sz;}			   
	       p.ID=bit_sent[ff]+1;  p.out=tor_num;  p.in=id;  
               p.tm=tm+pkt_sz/(double)traf_rate; 
               Send_pkt_TCP(p,ff);
 
               return key;
          }
     }
     else { } //receive control pkt

   return 0;
}

/***************************************************************
run leaf node events: 0: send packet 1: receive packet 
2:data packet retran time out 3: timeout for  send ack 
4: innitial TCP traffic 5: no action
****************************************************************/
template <class DType>
int leaf<DType>::Run_Leaf(pkt &p)
{  int i,k,et,tp,num,fl, index;
   pkt pp, pk;
   List<pkt> cp;

    et=10;

   if(TM.Empty()) { nexttime=INF; return et;}

   TM.SetToHead();
   p=*TM.Access();
   TM.Remove();
   pk=p;
   tp=p.type;
   fl=-1;
   
   if(p.SYN==1 && p.type==4) { Bit_Recv(p.flowsize, p.src, p.flow); } //set up a receiving flow
   
   for(i=0;i<MAXF;i++) {
	   if(tp==0 || tp==2 ) //0 send data, 2 retran data, 
	   {if(send_flow[i]==p.des) {fl=i;  break;} }
	   
	   if(tp==5  ) //recv ack,receive congestion pkt
	   { if(send_flow[i]==p.src) { fl=i; if(p.flow!=send_flow_id[i]) { return et; } break;} }
	   
	   if(tp==1 || tp==3 ) //1 send ack, 3 ack timeout, 
	   { if(recv_flow[i]==p.des) {fl=i;  break;} } 
	   
	   if(tp==4) //recv data
	   { if(recv_flow[i]==p.src) {fl=i; if(p.flow!=recv_flow_id[i]) { return et; } break;} }
   }
   
   if(tp==0 || tp==1 || tp==2 || tp==3 || tp==4 || tp==5 ) //cannot find entry for flow
   { if(fl==-1) { if(TM.Empty()) {nexttime==INF;} 
                   else { TM.SetToHead(); pp=*TM.Access(); nexttime=pp.tm; }
                   return et;
                }  //closed flow pkts, drop pkts
   }

   if(tp==0 ) { //send data packet
        if(!dataq.Empty()) { dataq.SetToHead(); dataq.Remove(); 
           bit_sent[fl]=p.ID; num=tcp_wd_sz[fl]-(bit_sent[fl]-bit_acked[fl]); sent_pkt[id]++;
           //set anothe pkt to time list
           if(!dataq.Empty())
           { dataq.SetToHead(); pp=*dataq.Access(); pp.tm=p.tm+pp.sz/traf_rate;
             AddTimeList(pp);
            }
		   if(p.SYN!=1) { //add next pkt to queue
                     if(pk.ID<bit_num[fl] && num>0) {//add the next pkt to queue
	                        if(pk.ID==bit_num[fl]-1) { pk.sz=last_pkt_sz[fl]; }
	                        else  {pk.sz=pkt_sz;}
                            pk.ID=pk.ID+1; pk.tm=pk.tm+pk.sz/traf_rate; pk.out=tor_num; pk.in=id; p.ecn=0; p.SYN=0;
                       Send_pkt_TCP(pk,fl);
				}
            }
         }
       pk=p; pk.tm=pk.tm+tm_data; pk.type=2; AddTimeList(pk);  //set timer for retran
       et=0;
   }
   else if(tp==1) {//send ack pkt 
      if(!dataq.Empty()) { dataq.SetToHead(); dataq.Remove();}
      if(p.ID==bit_to_rec[fl]) { End_TCP(0,p.tm,fl); }
      if(!dataq.Empty()) { dataq.SetToHead(); pp=*dataq.Access(); pp.tm=p.tm+pp.sz/traf_rate;
                           AddTimeList(pp);
       }
      et=1;
   }
   else if(tp==2) {  //timeout, retransimt all not acked data pkts for fl
      if(pk.SYN==1) { //retransmit SYN pkt
           if(!dataq.Empty()) { dataq.SetToHead(); pp=*dataq.Access(); 
               if(pp.flow==pk.flow && pp.SYN==1) { dataq.Remove(); }
               else { while(dataq.Fore()) { 
                      pp=*dataq.Access(); if(pp.flow==p.flow && pp.SYN==1) { dataq.Remove(); break; }
                   }
              }
            } 
	   pk.tm=p.tm+pk.sz/traf_rate; pk.type=0;
           Send_pkt_TCP(pk, fl);
     }
	 else {  cp.kill(); index=0;
       if(!TM.Empty()) { TM.SetToHead(); pp=*TM.Access(); k=pp.type;
           if(k==1 || k==3 || k==4 || k==5 ||  pp.flow!=p.flow)
	       { cp.EnQueue(pp); if(k==0 || k==1) { index=1;} }
           while(TM.Fore()) { pp=*TM.Access(); k=pp.type; //keep all incoming and sending ack pkts
              if(k==1 || k==3 || k==4 || k==5  || pp.flow!=p.flow)  
              { cp.Insert(pp); if(k==0 || k==1) { index=1; } }
	       }
           TM.kill(); TM=cp; cp.kill();
       }
    
    //remove sending data from dataq with ID>=p.ID
     if(!dataq.Empty()) { dataq.SetToHead(); pp=*dataq.Access();
       if(pp.flow==p.flow && pp.ID>=p.ID ) {dataq.Remove();}
       else { while(dataq.Fore()) { pp=*dataq.Access(); 
                   if(pp.flow==p.flow && pp.ID>=p.ID) {dataq.Remove(); break;}}
           }
    }

    if(index==0 && !dataq.Empty()) 
    { dataq.SetToHead(); pp=*dataq.Access(); pp.tm=p.tm+pp.sz/traf_rate; AddTimeList(pp); }

    pk.type=0;  pk.tm=p.tm+pk.sz/traf_rate; pk.out=tor_num; pk.in=id; p.ecn=0; p.SYN=0;
    tcp_thd[fl]=tcp_wd_sz[fl]/2; if(tcp_thd[fl]<IWD) { tcp_thd[fl]=IWD;}
    tcp_wd_sz[fl]=IWD; tcp_cong[fl]=0;
    bit_sent[fl]=pk.ID-1;  leaf_cong++;
    Send_pkt_TCP(pk,fl);
    et=2;
	 }
  }
  else if(tp==3) { //transmit ack pkt after time out
    pk.type=1; pk.tm=p.tm+pk.sz/(double)traf_rate; pk.out=tor_num; pk.in=id; 
    Send_pkt_TCP(pk,fl);
    et=3;
   }
  else  if(tp==4 || tp==5 ) { //receive pkt 4: data 5: ack
     if(Recv_pkt_TCP(p)==2) { et=7;} //receive the last ack pkt, end TCP session
     else { et=tp;}
  }
 
   if(TM.Empty()) { nexttime=INF; }
   else {TM.SetToHead(); pp=*TM.Access(); nexttime=pp.tm;}

   return et;
}

template <class DType>
void leaf<DType>::Show_Data() {
  int i=0;
  pkt p;

  cout<<"$$$$$$ Leaf node "<<id<<" pkt sz: "<<pkt_sz<<" tran rate: "<<traf_rate;
  cout<<" TOR: "<<tor_num<<" nexttime: "<<nexttime<<endl;
  cout<<"Number of receiving flow: "<<num_recv_flow<<" number of sending flow: "<<num_send_flow<<endl;
 
 
  if(!ASF.Empty()) {ASF.SetToHead(); cout<<"Available send entry "<<*ASF.Access();
     while(ASF.Fore()) { cout<<" -- "<<*ASF.Access();}
	 cout<<endl;
  }
  
  if(!ARF.Empty()) {ARF.SetToHead(); cout<<"Available recv entry:"<<*ARF.Access();
     while(ARF.Fore()) { cout<<" -- "<<*ARF.Access();}
	 cout<<endl;
  }

  
  for(i=0;i<MAXF;i++) {
  if(send_flow[i]!=-1){
     cout<<" For sending flow "<<i<<" des: "<<send_flow[i]<<" fnum: "<<send_flow_id[i];
     cout<<" tot-pkt to send: "<<bit_num[i]<<" pkt sent:"<<bit_sent[i]<<" tcp_wd_sz: "<<tcp_wd_sz[i]<<endl;
     cout<<"pkt acked:"<<bit_acked[i]<<" tcp thres "<<tcp_thd[i]<<" tcp rep_ack: "<<rep_ack[i];
     cout<<" Des: "<<send_flow[i]<<" tcp cong: "<<tcp_cong[i]<<" lsz: "<<last_pkt_sz[i]<<endl;
     cout<<" aplha: "<<alpha[i]<<" ecn #: "<<ecn_count[i]<<" deadline: "<<deadtime[i]<<endl;
  }
  if(recv_flow[i]!=-1){
   cout<<"For recving flow "<<i<<" fnum: "<<recv_flow_id[i]<<" src: "<<recv_flow[i]<<" pkt_to_received: "<<bit_to_rec[i]<<" pkt acked: "<<tcp_ack_num[i]<<"  pkt recved: "<<bit_received[i]<<endl;
   }
  }
  
  cout<<"List all Event: "<<endl;
  if(!TM.Empty()) { i=0;
      TM.SetToHead(); p=*TM.Access(); print_pkt(p);
     //cout<<"Source: "<<p.src<<" Des: "<<p.des<<" Type: "<<p.type<<" out:"<<p.out<<" fnum: "<<p.flow;
     //cout<<" pkt ID: "<<p.ID<<" pkt size: "<<p.sz<<" time: "<<p.tm<<" in: "<<p.in<<" ecn: "<<p.ecn<<endl;
     while(TM.Fore()) { p=*TM.Access(); i++; print_pkt(p);
    // cout<<"Source: "<<p.src<<" Des: "<<p.des<<" Type: "<<p.type<<" out:"<<p.out<<" fnum: "<<p.flow;
    // cout<<" pkt ID: "<<p.ID<<" pkt size: "<<p.sz<<" time: "<<p.tm<<" ecn: "<<p.ecn<<endl;
     if(i>10) { break;}
    }
   }
   else { cout<<" No event for the leaf node."<<endl;}

  cout<<"List all data in outgoing queue:"<<endl;
  if(!dataq.Empty()) { dataq.SetToHead(); p=*dataq.Access(); print_pkt(p); i=0;
      while(dataq.Fore()) { p=*dataq.Access(); print_pkt(p); i++; if(i>20) {break;} }
  }

  cout<<"List all missed pkt ID:"<<endl;
  for(i=0;i<MAXF;i++) { 
   if(!MP[i].Empty()) { MP[i].SetToHead();  cout<<" flow slot: "<<i<<"  "; cout<<*MP[i].Access()<<"  ";
        while(MP[i].Fore()) {cout<<*MP[i].Access()<<"  "; if(i%5==0) {cout<<endl;}}
     } 
  }

  cout<<endl<<"&&&&&&&&&&&&&&&&&&&&End of show data*********************"<<endl;

   return;
}
