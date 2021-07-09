
#include "TOR_pr.h"

template <class DType>
class spine
{
  public:
                spine();
	        ~spine();
			void SetUp();              //initial array data
                void Set_ID(int n);     //set the ID of the spine node
                int Get_ID();           //Get the ID of spine node
		void Set_Rate(double arate);  //set traffic rate to  TOR 
	        double Traf_Rate();     //return Traf rate linked to the TOR 
                void Set_Delay(double delay);  //set propagation delay from TOR
                double  Prog_Delay();         //Show the propag delay from TOR
                bool Recv_pkt(pkt p);        //Receive a packet 
                double NextTime();         //return next event time;
                void AddTimeList(pkt p);   //add pkt into event list
                int Run_Spine(pkt &p);     //run spine event  
                void Show_Data();            //show all data in the spine node

    private:
                int id;              //ID of the spine
		double  rate;   // line rate to tor
                double delay;    // propagation delay to tor
                double nexttime;    //time for next event
				double q_start[NT];   //record queue starting time receving data
                int ql[NT];        //  spine queue length
                List<pkt> qsp[NT];   //queue for NT tor nodes
                List<pkt> TM;  //List all events
};

template <class DType>
spine<DType>::spine()
{
  id=0; rate=delay=0; nexttime=INF; TM;
  for(int i=0; i<NT; i++) { qsp[i]; }
}

template <class DType>
spine<DType>::~spine()
{
 id=0; rate=delay=0; nexttime=0; TM.kill();
 for(int i=0; i<NT; i++) {qsp[i].kill(); ql[i]=0;}
}

template <class DType>
void spine<DType>::SetUp() 
{ 
  for(int i=0; i<NT; i++) { ql[i]=0; q_start[i]=0;}
}

template <class DType>
void spine<DType>::Set_ID(int n) { id=n;}

template <class DType>
int spine<DType>::Get_ID() { return id;}

template <class DType>
void spine<DType>::Set_Rate(double arate) {rate=arate; }

template <class DType>
double spine<DType>::Traf_Rate()
{ return rate;}

template <class DType>
void spine<DType>::Set_Delay(double d) { delay=d; }

template <class DType>
double spine<DType>::Prog_Delay() { return delay;}

template <class DType>
double spine<DType>::NextTime() {return nexttime;}

template <class DType>
void spine<DType>::AddTimeList(pkt p)
{
  double tm, tim;
  int tp;
  tm=p.tm;

  if(TM.Empty()) { TM.EnQueue(p); nexttime=tm; return; }

  TM.SetToHead(); tim=TM.Access()->tm;
  TM.SetToTail(); 
  if(tm>=TM.Access()->tm){ TM.Insert(p); nexttime=tim;  return;}
  while(TM.Back()){ if(tm>TM.Access()->tm) {TM.Insert(p);  nexttime=tim; return; }}

  TM.EnQueue(p);  nexttime=tm;
  
}

template <class DType>
bool spine<DType>::Recv_pkt(pkt p)
{
  int i, nhp;
  double tm;

  nhp=p.des/NH; //outgoing queue divided by NH to get the tor id
  tm=p.tm; i=p.type; p.out=nhp; p.in=id;
  if(i==2) {p.type=0;}            //recv data from tor send to another tor
  else if(i==3) {p.type=1;}       //recv ack from tor and send to  another tor

  if(ql[nhp]>QD-p.sz && i==2 && p.SYN!=1) {spine_cong++; return false;} //drop data pkt, queue full
  if(qsp[nhp].Empty()) { //queue is empty, add pkt to queue and make a schedule itme
    p.tm+=p.sz/rate; qsp[nhp].EnQueue(p);  AddTimeList(p); 
	busy_start[id][nhp]=tm; if(overall_time[id][nhp]==0) {overall_time[id][nhp]=tm;}
  }
  else {
	//  if(ql[nhp]>=QC-p.sz && i==2) { p.ecn=1;}  // set ecn bit to 1
	  qsp[nhp].SetToTail(); qsp[nhp].Insert(p);
  }
  
   ql[nhp]+=p.sz;  //queue length increase 1

  return true;

}

/***************************************************************
run leaf node events: 0: send data pkt to TOR 1: send ack to TOR
2: recv data from TOR  3: recv ack from TOR
****************************************************************/
template <class DType>
int spine<DType>::Run_Spine(pkt &p)
{  int  i,j,et,tp,num;
   pkt pp;

    et=4;

   if(TM.Empty()) { nexttime=INF; return et;}

   TM.SetToHead();
   p=*TM.Access();
   TM.Remove();
   tp=p.type; 
   i=p.out;

   if(tp==0 ) { //send data  to tor  return 0 if successful
       if(!qsp[i].Empty()) { 
          qsp[i].SetToHead(); 
          pp=*qsp[i].Access(); if(pp.ID!=p.ID || pp.type!=p.type) {cout<<"Wrong S1"<<endl;}
          qsp[i].Remove(); ql[i]-=p.sz; //remove the sent pkt
          if(!qsp[i].Empty()) { //set  sechdule timer for next pkt in the queue
             pp=*qsp[i].Access();  pp.tm=p.tm+pp.sz/(double)BC; 
             AddTimeList(pp);
          }
         et=0;
      }
	  if(qsp[i].Empty()) {busy_time[id][i]+=(p.tm-busy_start[id][i]);}
   }//end if(tp==0)
  else if(tp==1) { // send ack packet to tor
      if(!qsp[i].Empty()){   
          qsp[i].SetToHead();
          pp=*qsp[i].Access(); if(pp.ID!=p.ID || pp.type!=p.type) {cout<<"Wrong S2"<<endl;}
          qsp[i].Remove(); ql[i]-=p.sz; //remove pkt from queue
          if(!qsp[i].Empty()) { //set schedule  timer for next pkt in the queue
              pp=*qsp[i].Access(); pp.tm=p.tm+pp.sz/(double)BC;
              AddTimeList(pp);
          }
          et=1;
       }
	   if(qsp[i].Empty()) {busy_time[id][i]+=(p.tm-busy_start[id][i]);}
      }//end else if (tp==1)
  else  if(tp==2 || tp==3 ) { //receive data/ack pk from leaf
    Recv_pkt(p);
    et=tp;
   }
 
   if(TM.Empty()) { nexttime=INF; }
   else { TM.SetToHead(); pp=*TM.Access(); nexttime=pp.tm; }

   return et;
}

template <class DType>
void spine<DType>::Show_Data() {
  int i=0;
  pkt p;

  cout<<"List all parameters of Spine below:"<< " Node ID: "<<id<<endl;
  cout<<" traf rate:"<<rate<<" delay: "<<delay<< " Nexttime: "<<nexttime<<endl;

  cout<<"List all Event: "<<endl;
  if(!TM.Empty()) { TM.SetToHead(); p=*TM.Access(); print_pkt(p);
    // cout<<"Source: "<<p.src<<" Des: "<<p.des<<" Type: "<<p.type<<" out: "<<p.out<<endl;
   //  cout<<"pkt ID: "<<p.ID<<" pkt size: "<<p.sz<<" time: "<<p.tm<<" in: "<<p.in<<endl;
     while(TM.Fore()) { p=*TM.Access(); print_pkt(p);
       // cout<<"Source: "<<p.src<<" Des: "<<p.des<<" Type: "<<p.type<<" out: "<<p.out<<endl;
       //  cout<<"pkt ID: "<<p.ID<<" pkt size: "<<p.sz<<" time: "<<p.tm<<" in: "<<p.in<<endl;
      }
   }
   else { cout<<" No event for the spine node."<<endl;}

  cout<<"List all pkts in the queue:"<<endl;
  for(i=0; i<NT; i++) { cout<<"Queue "<<i<<" has "<<ql[i]<<" pkts"<<endl;
  if(ql[i]!=0) { qsp[i].SetToHead(); p=*qsp[i].Access(); print_pkt(p);
      while(qsp[i].Fore()) {p=*qsp[i].Access(); print_pkt(p);}
    }
  }

   return;
}
