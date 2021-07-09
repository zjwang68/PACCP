
#include "host_fdc.h"

template <class DType>
class tor
{
  public: 
                tor();
	        ~tor();
                void Set_ID(int n);     //set the ID of the tor node
                int Get_ID();           //Get the ID of tor node
		void Set_Leaf_Rate(double arate);  //set traffic rate to  leaf node
                void Set_Spine_Rate(double arate); //set traffic rate linked to spine node
	        double Leaf_Traf_Rate();     //return the Traf rate linked to the leaf node
                double Spine_Traf_Rate();   //return the traf rate linked to spine node
                void Set_Delay_Leaf(double delay);  //set propagation delay from leaf
                void Set_Delay_Spine(double delay); //set propagation delay from spine node
                double Delay_Leaf();         //Show the propag delay from leaf node
                double Delay_Spine();        //Show the prog delay from spine node
                bool Recv_pkt_Leaf(pkt p);     //Receive a packet  from leaf node
                bool Recv_pkt_Spine(pkt p);  //receive a packet from  spine node
                int  Find_Next_Hop(pkt p);       //find the out going queue of next hop 
                void Set_Ecmp_Start(int n);  //set the initial outgoing queue of ecmp
                double NextTime();         //return next event time;
                void AddTimeList(pkt p);   //add pkt into event list
                int Run_Tor(pkt &p);       //Run tor node event
                void Show_Data();            //show all data in the leaf node      

    private: 
                int id;              //ID of the roc
		double  leaf_rate;   // line rate from leaf
                double  spine_rate;  // line rate from spine
                double leaf_delay;    // propagation delay from leaf
                double spine_delay;  // propagation elay from spine
                double nexttime;    //time for next event
                int  des[NL][NL];       //routing table for NL leaf nodes
                double  dtimer[NL][NL];  //timer of NH route
                int ecmp_point;     //net new flow's next hop
                int lql[NH];        //leaf queue length
                int sql[NS];        //spine queue length
                List<pkt> qlf[NH];   //queue for NH leaf nodes
                List<pkt> qsp[NS];  //queue for NS spine nodes
                List<pkt> TM;  //List all events
        
};

template <class DType>
tor<DType>::tor()
{
  id=0; leaf_rate=spine_rate=0; leaf_delay=spine_delay=0; ecmp_point=0; nexttime=INF; TM;
  for(int i=0; i<NH; i++) { qlf[i]; lql[i]=0;}
  for(int i=0; i<NL; i++) { for(int j=0; j<NL; j++) { des[i][j]=-1; dtimer[i][j]=0;}}
  for(int i=0; i<NS; i++) {qsp[i]; sql[i]=0;}
}

template <class DType>
tor<DType>::~tor()
{
 id=0; leaf_rate=spine_rate=0; leaf_delay=spine_delay=0; ecmp_point=0;  nexttime=0; TM.kill();
 for(int i=0; i<NH; i++) {qlf[i].kill(); lql[i]=0;}
 for(int i=0; i<NS; i++) {qsp[i].kill(); sql[i]=0;}
 for(int i=0; i<NL; i++) { for(int j=0; j<NL; j++) { des[i][j]=-1; dtimer[i][j]=0;}}
}

template <class DType>
void tor<DType>::Set_ID(int n) { id=n;}

template <class DType>
int tor<DType>::Get_ID() { return id;}

template <class DType>
void tor<DType>::Set_Leaf_Rate(double arate) {leaf_rate=arate; }

template <class DType>
void tor<DType>::Set_Spine_Rate(double arate) {spine_rate=arate;}

template <class DType>
double tor<DType>::Leaf_Traf_Rate()
{ return leaf_rate;}

template <class DType>
double tor<DType>::Spine_Traf_Rate( )
{ return spine_rate; }


template <class DType>
void tor<DType>::Set_Delay_Leaf(double d) { leaf_delay=d; }

template <class DType>
void tor<DType>::Set_Delay_Spine(double d) { spine_delay=d;}

template <class DType>
double tor<DType>::Delay_Leaf()  { return leaf_delay;}

template <class DType>
double tor<DType>::Delay_Spine() { return spine_delay; }

template <class DType>
double tor<DType>::NextTime() {return nexttime;}

template <class DType>
void tor<DType>::AddTimeList(pkt p)
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
void tor<DType>::Set_Ecmp_Start(int n) { ecmp_point=n; }

template <class DType>
int tor<DType>::Find_Next_Hop(pkt p) 
{ int i,j,k;
  double tm;

  i=p.des; k=p.src; tm=p.tm;   ecmp_point=(int)(get_random()*NS); j=ecmp_point%NS;
  //ecmp_point++; j=ecmp_point%NS;

  if(des[k][i]==-1) { des[k][i]=j; dtimer[k][i]=tm; return j;}
  if((tm-dtimer[k][i])<route_timer) { dtimer[k][i]=tm;  return des[k][i]; }
  else {des[k][i]=j; dtimer[k][i]=tm;  return j;}

}

template <class DType>
bool tor<DType>::Recv_pkt_Leaf(pkt p)
{
   int i,j, sz, nhp;
   double tm;

   nhp=Find_Next_Hop(p);
   tm=p.tm; i=p.type; sz=p.sz; p.out=nhp; p.in=id;  //i==4 data pkt, 5 ack pkt
   if(i==4) { p.type=2;}  //recv data from leaf and send to spine
   else if(i==5) {p.type=3;}  //recv ack from leaf and send to spine

  if(sql[nhp]>=QD-p.sz && i==4 && p.SYN!=1) {tor_cong++; return false; } //drop a pkt 
  if(qsp[nhp].Empty()) { //add pkt to queue and make a schedule time 
        p.tm+=p.sz/spine_rate;    qsp[nhp].EnQueue(p);
        AddTimeList(p); 
   }
   else { 
    // if(sql[nhp]>=QC-p.sz && i==4) { p.ecn=1; } //set ecn bit to 1
     qsp[nhp].SetToTail(); qsp[nhp].Insert(p);
	}

  sql[nhp]+=p.sz;  //queue length increase 1


   return true;
}

template <class DType>
bool tor<DType>::Recv_pkt_Spine(pkt p)
{
  int i,j, sz, nhp;
  double tm;

  nhp=p.des%NH; //outgoing queue module by NH(number of hosts per tor)
  des[p.des][p.src]=p.in; dtimer[p.des][p.src]=p.tm; //record the spine id for source leaf node
  tm=p.tm; i=p.type; sz=p.sz; p.out=nhp; p.in=id;
  if(i==6) {p.type=0;}            //recv data pkt from spine and send to leaf
  else if(i==7) {p.type=1;}      //recv ack from spine and send to leaf

  if(lql[nhp]>=SQD-p.sz && i==6 && p.SYN!=1) { tor_leaf_cong++; return false;} //drop data pkt, queue full
  if(qlf[nhp].Empty()) { //add pkt to queue and make a schedule itme
     p.tm+=p.sz/leaf_rate; qlf[nhp].EnQueue(p);  AddTimeList(p); 
  }
  else {
	//  if(lql[nhp]>=QC-p.sz && i==6) { p.ecn=1;} //set ecn bit to 1  
	  qlf[nhp].SetToTail(); qlf[nhp].Insert(p);
	}
  
   lql[nhp]+=p.sz;  //queue length increase 1

  return true;

}

/***************************************************************
run leaf node events: 0: send data pkt to leaf 1: send ack to leaf
2: send data pkt to spine 3: send ack to spine  4: recv data from leaf 
5:recv ack from leaf  6: recv data from spine 7: recv ack from spine
****************************************************************/
template <class DType>
int tor<DType>::Run_Tor(pkt &p)
{  int  i,j,et,tp,num;
   pkt pp;

    et=8;

   if(TM.Empty()) { nexttime=INF; return et;}

   TM.SetToHead();
   p=*TM.Access();
   TM.Remove();
   tp=p.type; 
   i=p.out;

   if(tp==0 ) { //send data  to leaf return 0 if successful
       if(!qlf[i].Empty()) { 
          qlf[i].SetToHead(); 
          pp=*qlf[i].Access(); //if(p.ID!=pp.ID || p.type!=pp.type) { cout<<"wrong 1"<<endl;}
          qlf[i].Remove(); lql[i]-=p.sz; //remove the sent pkt
          if(!qlf[i].Empty()) { //set  sechdule timer for next pkt in the queue
             pp=*qlf[i].Access();  pp.tm=p.tm+pp.sz/leaf_rate; 
             AddTimeList(pp);
          }
         et=0;
      }
   }//end if(tp==0)
  else if(tp==1) { // send ack packet to leaf
      if(!qlf[i].Empty()){   
          qlf[i].SetToHead(); 
          pp=*qlf[i].Access(); // if(p.ID!=pp.ID || p.type!=pp.type) { cout<<"Wrong 2"<<endl;}
          qlf[i].Remove(); lql[i]-=p.sz; //remove pkt from queue
          if(!qlf[i].Empty()) { //set schedule  timer for next pkt in the queue
              pp=*qlf[i].Access(); pp.tm=p.tm+pp.sz/leaf_rate;
              AddTimeList(pp);
          }
          et=1;
       }
      }//end else if (tp==1)
  else if(tp==2) {  //send data to spine
    if(!qsp[i].Empty()) { 
        qsp[i].SetToHead(); 
        pp=*qsp[i].Access(); //if(p.ID!=pp.ID || p.type!=pp.type) { cout<<"Wrong 3"<<endl;}
        qsp[i].Remove();  sql[i]-=p.sz; //remove pkt from  spine queue
        if(!qsp[i].Empty()) {//set schedule timer for next pkt in the queue 
           pp=*qsp[i].Access(); pp.tm=p.tm+pp.sz/spine_rate;
           AddTimeList(pp); 
        }
        et=2;
     }
   }//end else if (tp==2)
  else if(tp==3) { //send ack to spine
    if(!qsp[i].Empty()) { 
       qsp[i].SetToHead(); 
       pp=*qsp[i].Access(); //if(pp.ID!=p.ID || pp.type!=p.type) {cout<<"Wrong 4"<<endl;}
       qsp[i].Remove(); sql[i]-=p.sz;  //remove pkt from spine queue
       if(!qsp[i].Empty()) { 
           pp=*qsp[i].Access(); pp.tm=p.tm+pp.sz/spine_rate;
           AddTimeList(pp);
       }
     et=3;
    }     
   }//end else if (tp==3)
  else  if(tp==4 || tp==5 ) { //receive data/ack pk from leaf
    Recv_pkt_Leaf(p);
    et=tp;
   }
  else if(tp==6 || tp==7) { //receive data/ack pkt from spine
    Recv_pkt_Spine(p);
    et=tp;
   }
 
   if(TM.Empty()) { nexttime=INF; }
   else { TM.SetToHead(); pp=*TM.Access(); nexttime=pp.tm; }

   return et;
}

template <class DType>
void tor<DType>::Show_Data() {
  int i=0;
  pkt p;

  cout<<"List all parameters of TOR below:"<<" node ID: "<<id<<" ecmp point: "<<ecmp_point<<endl;
  cout<<"Rate to spine: "<<spine_rate<<" Rate to leaf: "<<leaf_rate<<endl;
  cout<<"Dealy to spine: "<<spine_delay<<" delay to leaf: "<<leaf_delay<<endl;

  cout<<"List all Event: "<<endl;
  if(!TM.Empty()) { TM.SetToHead(); p=*TM.Access(); print_pkt(p);
     while(TM.Fore()) { p=*TM.Access(); print_pkt(p);}
   }
   else { cout<<" No event for the tor  node."<<endl;}
  
  cout<<"List all pkts in the queue:"<<endl;
  for(i=0; i<NH; i++) {
    cout<<"Leaf queue: "<<i<<" has "<<lql[i]<<" pkts in queue"<<endl;
    if(lql[i]!=0) { qlf[i].SetToHead(); p=*qlf[i].Access(); print_pkt(p);
                    while(qlf[i].Fore()) { p=*qlf[i].Access(); print_pkt(p);}
      }
  }

  for(i=0;i<NS; i++) {
      cout<<"Spine queue: "<< i<<" has "<<sql[i]<<" pks in queue"<<endl;
      if(sql[i]!=0) {qsp[i].SetToHead(); p=*qsp[i].Access(); print_pkt(p);
      while(qsp[i].Fore()) { p=*qsp[i].Access(); print_pkt(p); }
      }
  }

   return;
}


