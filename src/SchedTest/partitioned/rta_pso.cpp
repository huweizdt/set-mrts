//2022.1.5 by wyg


#include <rta_pso.h>
#include <types.h>
#include <set>
#include <vector>
#include <string>
#include <sched_test_base.h>
#include <sched_test_factory.h>
#include <iteration-helper.h>
#include <math-helper.h>
#include <param.h>
#include <processors.h>
#include <resources.h>
#include <sort.h>
#include <tasks.h>
#include <sched_test_base.h>
#include <sched_test_factory.h>
#include <time.h>
#include <cstdlib>
#include<math.h>

#define solution_size 20
// using namespace std;
using std::set;
using std::vector;

class Task;
class TaskSet;
class DAG_Task;
class DAG_TaskSet;
class Resource;
class ResourceSet;
class Param;

ulong best_jitter=0;
ulong jitter_arry[solution_size];


ulong RTA_PSO::max_jitter(int t)
{
  ulong jitter_temp=0;
  int i=0;
  foreach(taskset.get_tasks(),ti)
  {
     if(i<t)
     jitter_temp+=ti->get_jitter();
     else
     jitter_temp+=ti->get_response_time();
  }
  return jitter_temp;
}

void RTA_PSO::jitter_fit(int t)
{
if (t>=taskset.get_taskset_size()) 
  { 
  ulong jitter_total=0;
   foreach(taskset.get_tasks(),ti)
   jitter_total+=ti->get_jitter();
   if(jitter_total>best_jitter)
    {
      int i=0;
      best_jitter=jitter_total;
      cout<<"now best jitter is "<<best_jitter<<endl;
      foreach(taskset.get_tasks(),ti)
      {
        jitter_arry[i]=ti->get_jitter();
        i++;
          cout<<"task NO. "<<ti->get_id()<<"jitter:" <<ti->get_jitter()<<endl;
      }
    }
  }
 else {
  int i=0;
  foreach(taskset.get_tasks(),ti){ 
  	    if(t==i)
  	   	{
  		if(ti->get_status()==false)
  		{
  			for (ulong jitter=0;jitter<=ti->get_response_time();jitter+=(ti->get_wcet())) 
  			{ 
  				ti->set_jitter(jitter); 
  				if((response_time(taskset,(*ti))<=ti->get_deadline())&&(best_jitter<(max_jitter(t)))) ;
   				jitter_fit(t+1);
    			}
    			break;
    		}

    	         else if(response_time(taskset,(*ti))<=ti->get_deadline()){
   	        //else {
    	         jitter_fit(t+1);
    	         break;
    	         }
   		}
   	   i++;
  	}
  	}
     
}



ulong RTA_PSO::interference(const BPTask& task, ulong interval) {
  return task.get_wcet() *
         ceiling((interval + task.get_jitter()), task.get_period());
}


ulong RTA_PSO::response_time(BPTaskSet tasks,const BPTask& ti) {
  // uint t_id = ti.get_id();
  ulong test_end = ti.get_deadline();
  ulong test_start = ti.get_total_blocking() + ti.get_wcet();
  ulong response = test_start;
  ulong demand = 0;
  ulong total_interference = 0;
  while (response <= test_end) {
    demand = test_start;
    total_interference = 0;
     foreach(tasks.get_tasks(), tx)                               
    if (tx->get_priority() < ti.get_priority()){
      if (tx->get_partition() == ti.get_partition()) {
         //cout<<"Task:"<<tx->get_id()<<" < Task:"<<ti.get_id()<<endl;
        total_interference += interference((*tx), response);
      }
    }

    demand += total_interference;
  //  cout<<"demand is"<<demand<<endl;
    //cout<<"deadline is"<<test_end<<endl;
    if (response == demand)
      return response + ti.get_jitter();
    else
      response = demand;
  }
  return test_end + 100;
}

bool RTA_PSO::alloc_schedulable() {
  ulong response_bound;
  foreach(taskset.get_tasks(), ti) {
    if (ti->get_partition() == 0XFFFFFFFF) 
    continue;
    response_bound = response_time(taskset,(*ti));
    if (response_bound <= ti->get_deadline())
      ti->set_response_time(response_bound);
    else
      return false;
  }
  return true;
}




double RTA_PSO::fitness(int* schedule)
{
   double balance=0;
   double utilization_mean=0;
   double utilization_sum=taskset.get_utilization_sum();
   double speedfactor=0;
   int t_num=0;
   int processor_num=processorset.get_processor_num();
   double p_utilization[processor_num]={0};
  foreach(taskset.get_tasks(),ti)
  { 
    p_utilization[schedule[t_num]]+=ti->get_utilization();
    t_num++;
    
  }
  
  foreach(processorset.get_processors(),pi)
  {speedfactor+=pi->get_speedfactor();}
  utilization_mean=utilization_sum/speedfactor;
  for(int i=0;i<processor_num;i++)
  {
      balance+=pow((p_utilization[i]-utilization_mean),2);
  }
      balance=sqrt(balance);
  return balance;
}


void RTA_PSO::add_sort(int** solution_arry,int* new_solution,int num_tasks,int arrySize)
{
  int temp1[num_tasks];
  int temp2[num_tasks];
  int i=0,j=0,k=0;
 for(i=0;i<arrySize;i++) 
 {
  bool same_solution=true; 

    for(k=0;k<num_tasks;k++)
    {
    if(new_solution[k]!=solution_arry[i][k])
       same_solution=false;}

 if((fitness(new_solution)<=fitness(solution_arry[i]))&& (same_solution==false))
  {
    for(k=0;k<num_tasks;k++)
    temp1[k]=solution_arry[i][k];
    for(k=0;k<num_tasks;k++)
    solution_arry[i][k]=new_solution[k];
    for(j=i;j<arrySize-1;j++)
       {
         for(k=0;k<num_tasks;k++)
         temp2[k]=solution_arry[j+1][k];
         for(k=0;k<num_tasks;k++)
         solution_arry[j+1][k]=temp1[k];
         for(k=0;k<num_tasks;k++)
         temp1[k]=temp2[k];
       }
    break;
  }
 }
}






RTA_PSO::RTA_PSO(BPTaskSet taskset, ProcessorSet processorset,ResourceSet resourceset)
{
  this->taskset = taskset;
  this->processorset = processorset;
  this->resourceset = resourceset;
  this->resourceset.update(&(this->taskset));
  this->processorset.update(&(this->taskset), &(this->resourceset));
  this->taskset.RM_Order();
  this->processorset.init();  
  this->taskset.display();
} 








int** RTA_PSO::PSO(int num_partical,int num_epoch)
{
   srand(time(NULL));
   int num_tasks=this->taskset.get_taskset_size();
   int num_processors= processorset.get_processor_num();
   float V[num_partical][num_tasks];
   int   X[num_partical][num_tasks];
   int temp_pos[num_tasks];
   int pbest_pos[num_partical][num_tasks];
   int gbest_pos[num_tasks]={0};
   float weight=0.4;
   float c1=0.4,c2=0.6,r1,r2;
   int** solution_arry =new int*[20];
   for(int i = 0; i < 20; i++)
   {solution_arry[i] = new int[num_tasks];}
   int i=0,j=0;
   int times=0;
   
     for( i=0;i<20;i++)                             //initial start
   {for(j=0;j<num_tasks;j++)
   {
    solution_arry[i][j]=0;
   }}
   
   for( i=0;i<num_partical;i++)
   {for(j=0;j<num_tasks;j++)
   {
     X[i][j]=rand()%num_processors;
     V[i][j]=(float)(rand()%num_processors);
   }}
  
   cout<<"initial X is"<<endl;  
   for( i=0;i<num_partical;i++)
   {for(j=0;j<num_tasks;j++)
   {
     cout<<X[i][j]<<"\t";
   }
   cout<<endl;
   }
   
   cout<<"initial V is"<<endl;
   for( i=0;i<num_partical;i++)
   {for(j=0;j<num_tasks;j++)
   {
     cout<<V[i][j]<<"\t";
   }
   cout<<endl;
   }
   
       
  for(i=0;i<num_partical;i++)                             
  {
    
    for(j=0;j<num_tasks;j++)
    {temp_pos[j]=X[i][j];
    pbest_pos[i][j]=X[i][j];
    }
    
    if(fitness(temp_pos)>fitness(gbest_pos))
      {for(j=0;j<num_tasks;j++)
      gbest_pos[j]=temp_pos[j];}                                  //initial over
  }
  
  
  

  for(times=0 ; times<num_epoch ; times++)                       //iteration start
  {
    for(i=0;i<num_partical;i++)                                  
    {
      r1=(float)rand()/RAND_MAX;
      r2=(float)rand()/RAND_MAX;
      for(int j=0;j<num_tasks;j++)
      { 
      
      V[i][j]=weight*V[i][j]+c1*r1*(float)(pbest_pos[i][j]-X[i][j])+c2*r2*(float)(gbest_pos[j]-X[i][j]);   //iteration function
           
      X[i][j]=(int)(((float)X[i][j]+V[i][j])+0.5);               //no point
      if(X[i][j]>num_processors-1)                               //boundary of processor number
        X[i][j]=num_processors-1;
        else if(X[i][j]<0)
        X[i][j]=0;      
        
          
     if((j>=num_tasks/2)&&(X[i][j-num_tasks/2]==X[i][j]))        // PB tasks on different processor
         {        
           if(X[i][j]>=num_processors/2)
           X[i][j]--;
           else
           X[i][j]++;          
         }
        }
        
      add_sort(solution_arry,X[i],num_tasks,20);
    }
     
     for( i=0;i<num_partical;i++)
     { if(fitness(temp_pos)>fitness(pbest_pos[i]))               //upgrating
        for(j=0;j<num_tasks;j++)
        pbest_pos[i][j]=temp_pos[j];

     if(fitness(temp_pos)>fitness(gbest_pos))
        for(j=0;j<num_tasks;j++)
        gbest_pos[j]=temp_pos[j]  ;
     }    
     
     cout<<"*******************round "<<times<< " output list is**********************"<<endl;
     for(i=0;i<20;i++)
     {for(j=0;j<num_tasks;j++)
     {
      cout<<solution_arry[i][j]<<" ";
      if(j==num_tasks/2-1)
      cout<<endl;
     }
     cout<<"fitness:"<<fitness(solution_arry[i])<<endl;
     }
     
     
  }

  return solution_arry;
}


void RTA_PSO::partition(int* solution_arry)
{
   int i=0;
   foreach(taskset.get_tasks(),ti)
   {if(i<solution_arry)
   	{
   	  ti->set_partition(solution_arry[i],1);
  	 i++;
        cout<<"task"<<ti->get_id()<<"partition to"<<ti->get_partition()<<endl;
   	}
   }	
}

  BPTaskSet RTA_PSO::choose_solution(int** solution_arry,int arrySize)
{
     int t=0;
     int i=0;
     for(i=0;i<arrySize;i++)
     {
     partition(solution_arry[i]);
     foreach(taskset.get_tasks(),ti)
     cout<<"task"<<ti->get_id()<<"partition to"<<ti->get_partition()<<endl;
     
     if(RTA_PSO::alloc_schedulable())
     {
     cout<<"solution NO."<<i+1<<" is scheduable"<<endl;
     cout<<"fitness of the choosen solution is"<<fitness(solution_arry[i])<<endl;
     jitter_fit(0);
     return taskset;}}
     cout<<"unschedulable"<<endl;
}









