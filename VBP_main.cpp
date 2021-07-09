
/*##########################################################
This program simulate a two tier data center
##########################################################*/

#include <iomanip>
#include <math.h>
#include <fstream>
#include <iostream>
#include "global_vdc.h"

using namespace std;

int main(int argc, char** argv)
{
        int i,ii,j,k,kk,n,run,code,key,ll,lll,nn,nnn,mm;
        int sr, ds, nbit, ftype;
        long jj, f_flow;
        pkt p;
        double  tm,t,tt,tim, runtime,RTM,RSET;
        int  n99,n999,finish_time[5000];  //finish time interval from 1ms to 20 s
        int total_flow, flow_num, tot_sf, tot_bf,qostype;
        double total_time, tot_stime, tot_btime, t_time,now_time,temp_sum;
	double fdeadline[NL][NL],qload, qload_t,last_qtime,tot_qtime, xx,xxx;
	int count_flow[NL][NL], sflow_st, mflow_st, bflow_st, ct_sflow, ct_mflow, ct_bflow;
	ofstream  myfile, outf;

        myfile.open("vwb_70"); 
	outf.open("vmwb_70");
        ftype=1;    //ftype: 1: websearch 2: datamining
        tor_cong=spine_cong=out_order=leaf_cong=tor_leaf_cong=0;
        tot_num_pkt=tot_flow_gen=0; flow_num=tot_sf=tot_bf=0; 
        mrtt=0; num_rtt=0; RTM=0; ll=0; lll=0; f_flow=0; 
	total_time=tot_stime=tot_btime=t_time=total_flow=0; last_qtime=tot_qtime=0;
	sflow_st=mflow_st=bflow_st=ct_sflow=ct_mflow=ct_bflow=0;
        RSET=4000000; //2 second;

        srand(time(NULL));
        NetworkSetup();  //set up parameters for network and intial tcp traffic
         		
         for(i=0;i<5000;i++) {finish_time[i]=0;}
         for(i=0;i<NL;i++){
           for(j=0;j<NL;j++) { dest[i][j]=false; count_flow[i][j]=0; fdeadline[i][j]=0; }
         }
          runtime=0; run=4; // fl<<runtime<<"  ";
          cout<<"Start!!!!"<<endl;
      
         jj=0;
         while(runtime<endtime) { //cout<<run<<"   "<<key<<"  time: "<<runtime<<endl;
         switch (run) { 
            case (1):   //Leaf node event
                   code=LF[key].Run_Leaf(p);
                   if(code==0 || code==1) {//sucess send data/ack pkt to TOR
                     k=p.out; tm=LF[key].Prog_delay();  p.tm+=tm; 
                     if(code==0) { p.type=4;}   //send data pkt to tor
                     else {p.type=5;}           //send ack to tor 
                     tt=TR[k].NextTime(); 
                     TR[k].AddTimeList(p); 

		 /*  if(code==0) {  kk=p.des;  //put the number of bits  to destination
	              if(p.ID==1) { n=LF[key].Bit_Sent(kk); LF[kk].Bit_Recv(n, p.src, p.flow); }
		    }  */
						   
                     //update TOR time list
                     if(tt==INF) { AddTORTime(k);}  
                     else if(tt>TR[k].NextTime()) {
                        if(T_TOR.Empty()){ AddTORTime(k);}
                        else { T_TOR.SetToHead(); j=*T_TOR.Access(); ii=0;
                              if(j==k) {T_TOR.Remove(); AddTORTime(k); ii++;}
                              else { while(T_TOR.Fore()){ j=*T_TOR.Access();
                                        if(j==k) { T_TOR.Remove(); AddTORTime(k); ii++; break;}
                                     }
                             }
                             if(ii==0) { AddTORTime(k); }
                        }//end else
                     }

                   }//end if(code==0)

                   if(code==7) { //finish a tcp flow, count the finish time
                       t=flow_start_time[p.des][p.src]; 
                  if(t>0) { tim=p.tm-t; n=(int)(tim/400);  
                       if(n>4999) { cout<<"out of bound"<<n<<endl; n=4999;}
                       finish_time[n]++;
		       nn=flow_size[p.des][p.src];  nnn=LF[key].HostClass(); mm=0;
		       outf<<nn<<"  "<<nnn<<"  "<<tim<<"  ";
		       if(nn<=8000) {tot_sf++; tot_stime+=tim; 
				     if(count_flow[p.des][p.src]==1) { 
					      if(runtime<=fdeadline[p.des][p.src]) { sflow_st++; mm=1;}
					      ct_sflow++; count_flow[p.des][p.src]=0; fdeadline[p.des][p.src]=0;
					 }
				 }
				 else if(flow_size[p.des][p.src]<=160000) {
					 if(count_flow[p.des][p.src]==1) { 
					      if(runtime<=fdeadline[p.des][p.src]) { mflow_st++; mm=1;}
					      ct_mflow++; count_flow[p.des][p.src]=0; fdeadline[p.des][p.src]=0;
					 }
				 }
				 else {
					 if(count_flow[p.des][p.src]==1) { 
					             if(runtime<=fdeadline[p.des][p.src]) { bflow_st++; mm=1;}
					             ct_bflow++; count_flow[p.des][p.src]=0; fdeadline[p.des][p.src]=0; 
								}
					if(flow_size[p.des][p.src]>=800000) {tot_bf++; tot_btime+=tim; } 
			    }
					          total_flow++; total_time+=tim; t_time+=tim; flow_num++;
					          flow_start_time[p.des][p.src]=-1; 
						
			outf<<mm<<endl;
                      }
                      }

                   tm=LF[key].NextTime();
                   if(tm<INF) { AddLeafTime(key);}  //add next event to list
             break; 

             case (2):  //TOR event
                 code=TR[key].Run_Tor(p); 

                 if(code==0 || code==1 ) { //sucess send data/ack/bad/congestion pkt to leaf 
                    j=p.src;  k=p.des; tm=TR[key].Delay_Leaf(); p.tm+=tm;
                    if(code==0) {p.type=4;} //send data pkt to leaf
                    else if(code==1) {p.type=5;} //send data pkt to spine
		    					
                    tt=LF[k].NextTime();
                    LF[k].AddTimeList(p);

                    //update leaf time list and add the new pkt to leaf event list
                    if(tt==INF) { AddLeafTime(k); }
                    else if(tt>LF[k].NextTime()){
                        if(T_LF.Empty()) {AddLeafTime(k);}
                        else { T_LF.SetToHead(); j=*T_LF.Access(); ii=0;
                                  if(j==k) { T_LF.Remove(); AddLeafTime(k); ii++;}
                                  else { while(T_LF.Fore()) { j=*T_LF.Access();
                                          if(j==k) {T_LF.Remove(); AddLeafTime(k); ii++; break;}
                                         }
                                   }
                            if(ii==0) {AddLeafTime(k); }
                        }//end else
                     }           
                   }//end if
                else if(code==2 || code==3 ) { //sucess send data/ack pkt  to spine
                       k=p.out; tm=TR[key].Delay_Spine();  p.tm+=tm;
                       if(code==2) {p.type=2;}  //send data pkt to spine
                       else if(code==3) {p.type=3;} //send ack pkt to spine
		       
                       tt=SP[k].NextTime(); 
                       SP[k].AddTimeList(p);
					   
                       //update spine time list
                      if(tt==INF) { AddSpineTime(k); }
                      else if(tt>SP[k].NextTime()) {
                         if(T_SP.Empty()) {AddSpineTime(k); }
                         else { T_SP.SetToHead(); j=*T_SP.Access(); ii=0;
                                if(j==k) { T_SP.Remove(); AddSpineTime(k); ii++; }
                                else { while(T_SP.Fore()) { j=*T_SP.Access();
                                          if(j==k) {T_SP.Remove(); AddSpineTime(k); ii++; break;}
                                       }
                                    }
                                if(ii==0) {AddSpineTime(k); }
                            }//end else
                       }
                  }//end else if(code==2
                  
                   tm=TR[key].NextTime();
                   if(tm<INF) { AddTORTime(key); } //add next event for tor
             break;

              case (3): //Spine event
                  code=SP[key].Run_Spine(p);
                  if(code==0 || code==1 ) {  //send  data/ack/bad/cong pkt to tor
                      k=p.out; tm=SP[key].Prog_Delay();  p.tm+=tm;
                      if(code==0) { p.type=6; }       //send data pkt to tor
                      else if(code==1) { p.type=7; }  //send ack pkt to tor
		      
                      tt=TR[k].NextTime();  //record the orginal nexttime
                      TR[k].AddTimeList(p); //send pkt to TOR k

                     //update tor time list
                     if(tt==INF) {AddTORTime(k); }  //new event is the only event 
                     else if (tt>TR[k].NextTime()) {  //new event happens eariler than old even
                         if(T_TOR.Empty()) {AddTORTime(k); } //should not happen
                         else { T_TOR.SetToHead(); j=*T_TOR.Access(); ii=0;
                                if(j==k) { T_TOR.Remove(); AddTORTime(k); ii++;}
                                else { while(T_TOR.Fore()) { j=*T_TOR.Access();
                                       if(j==k) { T_TOR.Remove(); AddTORTime(k); ii++; break;}
                                       }
                                }
                         }//end else
                        if(ii==0) {AddTORTime(k); } //no old event for TOR k
                       }
                   }//end if(code==0

                      tm=SP[key].NextTime();
                      if(tm<INF) { AddSpineTime(key); }//add next event for spine
               break;

	        case (4): //generate new tcp traffic
	                  //initial first flow websearch traffic 
	         if(New_TCP_Flow(sr,ds,nbit,ftype)==true) { qostype=0;
		    if(LF[sr].HostClass()==2) { count_flow[sr][ds]=1; //counted as qos flow
			   if(nbit<8000) { fdeadline[sr][ds]=runtime+sdead; } //+100.*nbit/SRATE;  }
			   else if(nbit<160000) {fdeadline[sr][ds]=runtime+mdead +100.*nbit/MRATE;  }
			   else { fdeadline[sr][ds]=runtime+100.*nbit/TRATE; }
			}
	            if(LF[sr].Initial_TCP(runtime,ds, nbit)==false)
                     { f_flow++; cout<<"failed to initial new tcp"<<endl;} 
	            else { flow_start_time[sr][ds]=runtime;  flow_size[sr][ds]=nbit;
			      tt=LF[sr].NextTime(); 
                     
                     //update leaf time list and add the new pkt to leaf event list
                     if(T_LF.Empty()) {AddLeafTime(sr);}
                     else { T_LF.SetToHead(); j=*T_LF.Access(); ii=0; 
                                  if(j==sr) { T_LF.Remove(); AddLeafTime(sr); ii++;}
                                  else { while(T_LF.Fore()) { j=*T_LF.Access();
                                          if(j==sr) {T_LF.Remove(); AddLeafTime(sr); ii++; break;}
                                         }
                                   }
                            if(ii==0) {AddLeafTime(sr); }
                       }//end else { T_LF
                      }//end else
                    }
                    else {f_flow++; }
                   // fl<<sr<<" "<<ds<<" "<<nbit<<endl;
                    RTM=runtime+get_next_time(tcp_rate);
                    //fl<<RTM<<" ";
                                          
          }//end switch

            now_time=runtime; run=4; runtime=RTM;
           
           if(!T_LF.Empty()) { T_LF.SetToHead(); key=*T_LF.Access(); 
              if(runtime>LF[key].NextTime()) { runtime=LF[key].NextTime(); run=1; }}
           
           if(!T_TOR.Empty()) { T_TOR.SetToHead(); kk=*T_TOR.Access();
               if(runtime>TR[kk].NextTime()) {runtime=TR[kk].NextTime(); run=2; key=kk; } }

           if(!T_SP.Empty()) { T_SP.SetToHead(); kk=*T_SP.Access();
               if(runtime>SP[kk].NextTime()) { runtime=SP[kk].NextTime(); run=3; key=kk;} }
			
	  if(runtime>5999999 && runtime<=6000000) { //recount the performance metrics at 2 seconds
	       tot_flow_gen=total_flow=tot_sf=tot_bf=0;  total_time=tot_stime=tot_btime=t_time=0;
               ct_sflow=ct_mflow=ct_bflow=sflow_st=mflow_st=bflow_st=0;
	       for(i=0;i<5000;i++) {finish_time[i]=0;}
	   }
			   
           jj++;  /* ll++; 
         if(runtime>39410 && ll>=lll) {ll=0;
            cout<<jj<<"  continue?(0 exit)"<<endl;  cin>>lll; if(lll==0) { exit(0); }
          //  Show_Time_List();  //if(key!=81) { LF[81].Show_Data(); }
           // cout<<"&&&more data? (0 exit)"<<endl; cin>:11
           // >j; if(j==0) {exit(0);}
             LF[15].Show_Data(); LF[191].Show_Data();
	  // LF[5].Show_Data(); LF[10].Show_Data(); LF[14].Show_Data(); 
          //    cout<<"&&&more data? (0 exit)"<<endl; cin>>j; if(j==0) {exit(0);}
          //  cout<<"***** more data? (0 no)***********"<<endl; cin>>j; if(j==0) { exit(0);}
          //    TR[3].Show_Data();   // TR[3].Show_Rate();
          //  cout<<"$$$$$More Data 0 exit"<<endl; cin>>j; if(j==0) {exit(0);}
             //SP[key%NS].Show_Data();  //cout<<"TTTTTTTTTTT"<<endl;
          //  SP[0].Show_Data();  SP[1].Show_Data(); SP[2].Show_Data(); SP[3].Show_Data(); 
          // cout<<"More data?(0 exit)"<<endl; cin>>j; if(j==0) { exit(0);}
          //  SP[4].Show_Data(); SP[5].Show_Data(); SP[6].Show_Data(); SP[7].Show_Data(); 
           cout<<endl<<"Runtime: "<<runtime<<" code: "<<run<<" key: "<<key<<endl;
          }
          */
           
          if(run==1) {T_LF.SetToHead(); T_LF.Remove();}
          else if(run==2) {T_TOR.SetToHead(); T_TOR.Remove(); }
          else if(run==3) {T_SP.SetToHead(); T_SP.Remove(); }
          else if (run==4) {}
          else {cout<<"No event, finish"<<endl; return 0;}
          
         if(now_time>runtime) { Show_Time_List(); LF[key].Show_Data();
	            cout<<" wrong: now time "<<now_time<<" runtime: "<<runtime<<" code: "<<run<<" Key: "<<key<<endl; 
                    break;
          }  
		
		 		 
	  if(jj%9000000==0 && total_flow!=0) {
           cout<<endl<<"runtime: "<<runtime<<"  "<<" Failed flow generate: "<<f_flow<<" Total flow generated: "<<tot_flow_gen<<endl; 
	   cout<<" Total finished flow: "<<total_flow<<" average finish time: "<<total_time/total_flow<<endl;
           cout<<" Total small flow: "<<tot_sf<<" average finish time: "<<tot_stime/tot_sf<<endl;
	   cout<<" Total big flow: "<<tot_bf<<" average finish time: "<<tot_btime/tot_bf<<endl;
           cout<<" Total number of bytes generated: "<<tot_num_pkt<<endl;
           cout<<"Period flow #: "<<flow_num<<" Period average finish time: "<<t_time/flow_num<<endl<<endl;
           cout<<"Spine cong: "<<spine_cong<<" Tor to spine cong: "<<tor_cong<<" tor to leaf cong: "<<tor_leaf_cong<<endl;
           cout<<"Leaf retran#:"<<leaf_cong<<" out order pkt: "<<out_order<<endl;
           cout<<" RTT: "<<mrtt/num_rtt<<" leaf cong: "<<numb_cong<<endl; 
           cout<<" Max # send flow a node: "<<max_send_flow<<" Max recv flow a node: "<<max_recv_flow<<endl;
	   cout<<" # of qos flows short: "<<ct_sflow<<" mid: "<<ct_mflow<<" big: "<<ct_bflow<<endl;
	   cout<<" Satisfied qos tot: "<<1.0*(sflow_st+mflow_st+bflow_st)/(ct_sflow+ct_mflow+ct_bflow)<<" short: "<<1.0*sflow_st/ct_sflow;
	   cout<<" mid: "<<1.0*mflow_st/ct_mflow<<" Big: "<<1.0*bflow_st/ct_bflow<<endl;
	  n99=n999=0; temp_sum=0;
          for(i=0;i<5000;i++) {
             if(finish_time[i]!=0) {   temp_sum+=finish_time[i];
					  if(n99==0 && temp_sum/total_flow>=0.99) { n99=i; }
					  if(n999==0 && temp_sum/total_flow>=0.999) { n999=i; }
		     }
         }
		 cout<<" 99th percentile is: "<<n99*0.4<<" 999th percentile is: "<<n999*0.4<<endl;
		 
	   xx=xxx=0;
	   for(i=0;i<NS;i++){ 
	       for(j=0;j<NT;j++){ xx+=busy_time[i][j]; xxx+=(runtime-overall_time[i][j]); } 
	  }
	   qload_t=(xx-tot_qtime)/NS/NT/(runtime-last_qtime);
           tot_qtime=xx; qload=tot_qtime/xxx;
           cout<<"queue load percentage: "<<qload<<" load in this period: "<<qload_t<<endl;
           cout<<"&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&"<<endl;
           last_qtime=runtime;   t_time=0; flow_num=0;
          }
        }//end while

		  	  
          k=0; n99=n999=0; temp_sum=0;
          for(i=0;i<5000;i++) {
             if(finish_time[i]!=0) { //cout<<" "<<i<<"  "<<finish_time[i]; k++;
                      //myfile<<" "<<i<<"  "<<finish_time[i]<<endl;}
                      //if(k%10==0) { k++; cout<<endl; }
					  temp_sum+=finish_time[i];
					  if(n99==0 && temp_sum/total_flow>=0.99) { n99=i; }
					  if(n999==0 && temp_sum/total_flow>=0.999) { n999=i; }
		     }
         }
		 
        myfile<<"parameters for this simulation are:" <<endl;
	 myfile<<"NS: "<<NS<<" NT: "<<NT<<" NH: "<<NH<<" NL "<<NL<<" QD: "<<QD<<" QC: "<<QC<<endl;
	 myfile<<"Route_timer: "<<route_timer<<" tcp rate: "<<tcp_rate<<" MAXF: "<<MAXF<<" tm_data:"<<tm_data<<endl;
	 myfile<<" BC: "<<BC<<" BL: "<<BL<<endl;
	 myfile<<" DSZ: "<<DSZ<<" asz: "<<asz<<" IWD: "<<IWD<<" CNT: "<<CNT<<endl;
         myfile<<" sdead: "<<sdead<<" mdead: "<<mdead<<" TRATE: "<<TRATE<<" rcos: "<<rcos<<endl;

          cout<<endl<<" The total umber of flows: "<<total_flow<<" small flow: "<<tot_sf<<" big flow: "<<tot_bf<<endl;
          myfile<<endl<<"ToTal number of flows: "<<total_flow<<" small flow: "<<tot_sf<<" big flow: "<<tot_bf<<endl;
          cout<<" Average flow finish time: "<<total_time/total_flow<<endl;
          myfile<<" Average small flow finish time: "<<tot_stime/tot_sf<<endl;
          myfile<<" Average big flow finish time: "<<tot_btime/tot_bf<<endl;
          cout<<" Average small flow finish time: "<<tot_stime/tot_sf<<endl;
          cout<<" Average big flow finish time: "<<tot_btime/tot_bf<<endl;
          myfile<<" Average flow finish time: "<<total_time/total_flow<<endl;
	  cout<<"Total pkt: "<<tot_num_pkt<<" out order pkt: "<<out_order<<endl;
	  myfile<<"Total pkt: "<<tot_num_pkt<<" out order pkt: "<<out_order<<endl;
          myfile<<"Measured average RTT: "<<mrtt/num_rtt<<endl;
	  cout<<" Satisfied qos tot: "<<1.0*(sflow_st+mflow_st+bflow_st)/(ct_sflow+ct_mflow+ct_bflow)<<" short: "<<1.0*sflow_st/ct_sflow;
   cout<<" mid: "<<1.0*mflow_st/ct_mflow<<" Big: "<<1.0*bflow_st/ct_bflow<<endl;
	   myfile<<" Satisfied qos tot: "<<1.0*(sflow_st+mflow_st+bflow_st)/(ct_sflow+ct_mflow+ct_bflow)<<" short: "<<1.0*sflow_st/ct_sflow;
	   myfile<<" mid: "<<1.0*mflow_st/ct_mflow<<" Big: "<<1.0*bflow_st/ct_bflow<<endl;
          myfile<<" Max send flow: "<<max_send_flow<<"  Max recv flow: "<<max_recv_flow<<endl;
          myfile<<" queue load percentage: "<<qload<<" load in last period: "<<qload_t<<endl;
		  myfile<<" 99th percentile is: "<<n99*0.4<<" 999th percentile is: "<<n999*0.4<<endl;
		  cout<<" 99th percentile is: "<<n99*0.4<<" 999th percentile is: "<<n999*0.4<<endl;
          myfile.close();

          return 0;

}       

