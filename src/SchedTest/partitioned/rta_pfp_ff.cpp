// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <math-helper.h>
#include <rta_pfp_ff.h>

RTA_PFP_FF::RTA_PFP_FF()
    : PartitionedSched(false, RTA, FIX_PRIORITY, NONE, "", "NONE") {}

RTA_PFP_FF::RTA_PFP_FF(TaskSet tasks, ProcessorSet processors,
                       ResourceSet resources)
    : PartitionedSched(false, RTA, FIX_PRIORITY, NONE, "", "NONE") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;

  this->resources.update(&(this->tasks));
  this->processors.update(&(this->tasks), &(this->resources));

  // this->tasks.RM_Order();
  this->processors.init();
}

ulong RTA_PFP_FF::interference(const Task& task, ulong interval) {
  return task.get_wcet() *
         ceiling((interval + task.get_jitter()), task.get_period());
}

/*
ulong RTA_PFP_FF::interference(Task& task, ulong interval)
{
        return task.get_wcet() * ceiling((interval + task.get_response_time() -
task.get_wcet()), task.get_period());
}
*/

ulong RTA_PFP_FF::response_time(const Task& ti) {
  // uint t_id = ti.get_id();
  ulong test_end = ti.get_deadline();
  ulong test_start = ti.get_total_blocking() + ti.get_wcet();
  ulong response = test_start;
  ulong demand = 0;
  while (response <= test_end) {
    demand = test_start;

    ulong total_interference = 0;

    foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
      if (th->get_partition() == ti.get_partition()) {
        // cout<<"Task:"<<th->get_id()<<" < Task:"<<ti.get_id()<<endl;
        total_interference += interference((*th), response);
      }
    }

    demand += total_interference;

    if (response == demand)
      return response + ti.get_jitter();
    else
      response = demand;
  }
  return test_end + 100;
}

bool RTA_PFP_FF::alloc_schedulable() {
  ulong response_bound;
  /*
          foreach(tasks.get_tasks(), task)
          {
                  cout<<"Task"<<task->get_id()<<":
     partition:"<<task->get_partition()<<endl;
          }
  */

  // for (uint t_id = 0; t_id < tasks.get_taskset_size(); t_id ++)
  foreach(tasks.get_tasks(), ti) {
    if (ti->get_partition() == 0XFFFFFFFF) continue;
    // cout<<"RTA Task:"<<ti->get_id()<<endl;
    response_bound = response_time((*ti));

    if (response_bound <= ti->get_deadline())
      ti->set_response_time(response_bound);
    else
      return false;
  }
  return true;
}

bool RTA_PFP_FF::is_schedulable() {
  /*
          cout<<"After sorting."<<endl;
          foreach(tasks.get_tasks(), task)
          {
                  long slack = task->get_deadline();
                  slack -= task->get_wcet();
                  cout<<"Task:"<<task->get_id()<<" Slack:"<<slack<<endl;
                  cout<<"----------------------------"<<endl;
          }
  */

  uint p_num = processors.get_processor_num();
  foreach(tasks.get_tasks(), ti) {
    // cout<<"<==========Task"<<ti->get_id()<<"==========>"<<endl;
    if (!BinPacking_FF(&(*ti), &tasks, &processors, &resources, TEST)) {
      /*
                              cout<<"=====fail====="<<endl;
                              foreach(tasks.get_tasks(), task)
                              {
                                      cout<<"Task:"<<task->get_id()<<"
         Partition:"<<task->get_partition()<<"
         priority:"<<task->get_priority()<<"
         U:"<<task->get_utilization()<<endl;
                                      cout<<"----------------------------"<<endl;
                              }

                              for(uint k = 0; k < p_num; k++)
                              {
                                      cout<<"Processor "<<k<<"
         utilization:"<<processors.get_processors()[k].get_utilization()<<endl;
                                      foreach(processors.get_processors()[k].get_taskqueue(),
         t_id)
                                      {
                                              Task& task =
         tasks.get_task_by_id(*t_id); cout<<"task"<<task.get_id()<<"\t";
                                      }
                                      cout<<endl;
                              }
      */
      return false;
    }
  }
  return true;
}
