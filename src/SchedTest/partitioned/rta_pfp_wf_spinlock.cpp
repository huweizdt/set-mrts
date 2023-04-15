// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <math-helper.h>
#include <math.h>
#include <rta_pfp_wf_spinlock.h>

using std::max;
using std::min;

RTA_PFP_WF_spinlock::RTA_PFP_WF_spinlock()
    : PartitionedSched(false, RTA, FIX_PRIORITY, SPIN, "", "spinlock") {}

RTA_PFP_WF_spinlock::RTA_PFP_WF_spinlock(TaskSet tasks, ProcessorSet processors,
                                         ResourceSet resources)
    : PartitionedSched(false, RTA, FIX_PRIORITY, SPIN, "", "spinlock") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;

  this->resources.update(&(this->tasks));
  this->processors.update(&(this->tasks), &(this->resources));

  // this->tasks.RM_Order();
  this->processors.init();
}

ulong RTA_PFP_WF_spinlock::interference(const Task& task, ulong interval) {
  return (task.get_wcet() + task.get_spin()) *
         ceiling((interval + task.get_jitter()), task.get_period());
}

void RTA_PFP_WF_spinlock::calculate_spin() {
  ulong spinning = 0;
  for (uint i = 0; i < tasks.get_taskset_size(); i++) {
    Task &task_i = tasks.get_tasks()[i];
    // cout<<"request num:"<<task_i.get_requests().size()<<endl;
    for (uint j = 0; j < task_i.get_requests().size(); j++) {
      const Request &request = task_i.get_requests()[j];
      uint id = request.get_resource_id();
      uint num = request.get_num_requests();
      ulong Sum = 0;
      for (uint processor_id = 0;
           processor_id < processors.get_processor_num(); processor_id++) {
        if (processor_id != task_i.get_partition()) {
          Processor &processor = processors.get_processors()[processor_id];
          ulong max_length = 0;
          foreach(processor.get_taskqueue(), t_id) {
            Task &task_k = tasks.get_task_by_id(*t_id);
            if (task_k.is_request_exist(id)) {
              const Request &request_k = task_k.get_request_by_id(id);
              if (max_length < request_k.get_max_length())
                max_length = request_k.get_max_length();
            }
          }
          Sum += max_length;
        }
      }
      spinning += num * Sum;
    }
    task_i.set_spin(spinning);
  }
}

void RTA_PFP_WF_spinlock::calculate_local_blocking() {
  for (uint i = 0; i < tasks.get_taskset_size(); i++) {
    Task &task_i = tasks.get_tasks()[i];
    ulong lb = 0;
    for (uint j = 0; j < task_i.get_requests().size(); j++) {
      const Request &request_i = task_i.get_requests()[j];
      uint id = request_i.get_resource_id();
      if (resources.get_resource_by_id(id).get_ceiling() <= i) {
        for (uint k = i + 1; k < tasks.get_taskset_size(); k++) {
          Task &task_k = tasks.get_tasks()[k];
          const Request &request_k = task_k.get_request_by_id(id);
          if (&request_k) {
            lb = max(lb, request_k.get_max_length());
          }
        }
      }
    }
    task_i.set_local_blocking(lb);
  }
}

ulong RTA_PFP_WF_spinlock::response_time(const Task& ti) {
  ulong test_end = ti.get_deadline();
  ulong test_start = ti.get_spin() + ti.get_local_blocking() + ti.get_wcet();
  ulong response = test_start;
  ulong demand = 0;
  while (response <= test_end) {
    demand = test_start;

    ulong total_interference = 0;
    foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
      if (ti.get_partition() == th->get_partition()) {
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

bool RTA_PFP_WF_spinlock::alloc_schedulable() {
  ulong response_bound;

  // for (uint t_id = 0; t_id < tasks.get_taskset_size(); t_id ++)
  foreach(tasks.get_tasks(), ti) {
    calculate_spin();
    calculate_local_blocking();

    if (ti->get_partition() == MAX_INT) continue;

    response_bound = response_time((*ti));

    if (response_bound <= ti->get_deadline())
      ti->set_response_time(response_bound);
    else
      return false;
  }
  return true;
}

bool RTA_PFP_WF_spinlock::is_schedulable() {
  foreach(tasks.get_tasks(), ti) {
    if (!BinPacking_WF(&(*ti), &tasks, &processors, &resources, TEST))
      return false;
  }
  return true;
}
