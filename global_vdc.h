
#include "SPINE_pr.h"

List<int> T_LF, T_TOR, T_SP;  //timing list for  leaf, tor and spine nodes
leaf<int> LF[NL];             //define NL number of leaf nodes
tor<int> TR[NT];             //define NT number of tor nodes
spine<int> SP[NS];          //define NS number of spine nodes

void AddLeafTime(int n)
{
  if(T_LF.Empty()) {T_LF.EnQueue(n); return; }
  
  int i;
   
   T_LF.SetToTail();
   while(1) {
      i=*T_LF.Access();
      if(LF[n].NextTime()>=LF[i].NextTime()) { T_LF.Insert(n); return;}
      if(!T_LF.Back()) { break; }
     }

    T_LF.EnQueue(n);
}

void NetworkSetup()
{  int i,j,k,num;
   double xx,tm;

  fnumber=0; max_send_flow=max_recv_flow=0; numb_cong=0; //tot_lfn=0;
  
   
  for (i=0;i<NL; i++) {  //set up  parameters for NL leaf nodes
     k=i%NH; 
     if(k<NBE) { j=0;}
     else if(k<NDS) { j=1; }
     else {j=2;}
     LF[i].SetUp();				 LF[i].SetHost(j);
     LF[i].Set_ID(i);            LF[i].Set_Traf_rate((double)BL);
     LF[i].SetTOR((int)i/NH);    LF[i].Set_Prog_delay((double)t_lt);
     LF[i].Set_pkt_size(DSZ);    
  }

  for (i=0; i<NT; i++) { //set up parameters for NT TORs
    // TR[i].SetUp();
     TR[i].Set_ID(i); TR[i].Set_Leaf_Rate((double)BL);
     TR[i].Set_Spine_Rate((double)BC); TR[i].Set_Delay_Leaf((double)t_lt);
     TR[i].Set_Delay_Spine((double)t_st);  
   }

  for(int i=0; i<NS; i++) { //set up parameters for NS spines
    SP[i].SetUp();
    SP[i].Set_ID(i);  SP[i].Set_Rate((double)BC); 
    SP[i].Set_Delay((double) t_st); 
  }

  for(i=0;i<NL;i++)  //initial flow start data
     for(j=0;j<NL; j++) { flow_start_time[i][j]=-1; flow_size[i][j]=0;}
	 
	 
  for(i=0;i<NS;i++)
	  for(j=0;j<NT;j++) { busy_time[i][j]=busy_start[i][j]=overall_time[i][j]=0; }
}
 

void AddTORTime(int n)
{
  if(T_TOR.Empty()){ T_TOR.EnQueue(n); return; }
  
  int i; 
  T_TOR.SetToTail();
  while(1) {
      i=*T_TOR.Access();
      if(TR[n].NextTime()>=TR[i].NextTime()) {T_TOR.Insert(n); return; }
      if(!T_TOR.Back()) { break;}
    }
  
   T_TOR.EnQueue(n);
}

void AddSpineTime(int n)
{ if(T_SP.Empty()) {T_SP.EnQueue(n); return;}
  
  int i;
  T_SP.SetToTail();
  while(1) {
    i=*T_SP.Access();
    if(SP[n].NextTime()>=SP[i].NextTime()) { T_SP.Insert(n); return;}
    if(!T_SP.Back()) {break;}
  }
   T_SP.EnQueue(n);

}

bool New_TCP_Flow(int & src, int &des, int & nbits, int tp)
{
	int i,j,k;
	double x;
         
		
    k=0;  
	while(1) { i=(int)(NL*get_random()); k++; if(k>NL*1000) { return false;}
                   if(LF[i].Num_Send_Flow()<MAXF) { src=i; break;} }
	while(1) {j=(int)(NL*get_random()); k++; if(k>NL*2000) {return false;}
                  if((j/NH)!=(i/NH)) {if(LF[j].Num_Recv_Flow()<MAXF && dest[i][j]==false) {des=j; break;} }}
	
	nbits=gen_traffic(tp);  //tp==0 self define tp==1, webserach tp==2 data mining
	dest[i][j]=true;
		 
	return true;
}

void Show_Time_List()
{ int i,n;

  i=0;
  cout<<"Show time list of leaf node: "<<endl;
  if(T_LF.Empty()) {cout<<"No event for leaf nodes."<<endl; }
  else {
     T_LF.SetToHead(); n=*T_LF.Access(); 
     cout<<" Leaf: "<<n<<" even at: "<<LF[n].NextTime()<<endl;
     while(T_LF.Fore()) { n=*T_LF.Access(); cout<<" Leaf: "<<n<<" a event at  "<<LF[n].NextTime()<<endl; } //i++; if(i>5) break; }
  }
 
  cout<<"show time list of tor node:"<<endl;
  if(T_TOR.Empty()) { cout<<" No event for tor nodes."<<endl;}
  else {
     T_TOR.SetToHead(); n=*T_TOR.Access();
     cout<<" TOR: "<<n<<" event at "<<TR[n].NextTime()<<endl;
     while(T_TOR.Fore()) { n=*T_TOR.Access(); cout<<" TOR: "<<n<<" "<<TR[n].NextTime()<<endl; }
   } 

  cout<<"Show time list of spine node:"<<endl;
  if(T_SP.Empty()) { cout<<" No event for spine node."<<endl; }
  else {
     T_SP.SetToHead(); n=*T_SP.Access();
     cout<<" Spine: "<<n<<" event at "<<SP[n].NextTime()<<endl;
    while(T_SP.Fore()) {n=*T_SP.Access(); cout<<" Spine: "<<n<<" "<<SP[n].NextTime()<<endl;}
   }

}

