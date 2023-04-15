// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <math.h>
#include <math-helper.h>
#include <rta_pfp_ro_feasible.h>

using std::max;

RTA_PFP_RO_FEASIBLE::RTA_PFP_RO_FEASIBLE()
    : PartitionedSched(false, RTA, FIX_PRIORITY, DPCP, "",
                       "Resource-Oriented") {}

RTA_PFP_RO_FEASIBLE::RTA_PFP_RO_FEASIBLE(TaskSet tasks, ProcessorSet processors,
                                         ResourceSet resources)
    : PartitionedSched(false, RTA, FIX_PRIORITY, DPCP, "",
                       "Resource-Oriented") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;

  this->resources.update(&(this->tasks));
  this->processors.update(&(this->tasks), &(this->resources));

  this->tasks.RM_Order();
  this->processors.init();
}

RTA_PFP_RO_FEASIBLE::~RTA_PFP_RO_FEASIBLE() {}

bool RTA_PFP_RO_FEASIBLE::is_schedulable() {
  if (condition_1() && condition_2() && condition_3())
    return true;
  else
    return false;
}

bool RTA_PFP_RO_FEASIBLE::condition_1() {
  if ((processors.get_processor_num() - tasks.get_utilization_sum()) >= _EPS) {
    return true;
  } else {
    // cout<<"Condition 1 false."<<endl;
    return false;
  }
}

bool RTA_PFP_RO_FEASIBLE::condition_2() {
  foreach(tasks.get_tasks(), task) {
    if (task->get_utilization() > 1) {
      // cout<<"Condition 2 false."<<endl;
      return false;
    }
  }
  return true;
}

bool RTA_PFP_RO_FEASIBLE::condition_3() {
  foreach(tasks.get_tasks(), ti) {
    foreach(ti->get_requests(), request_q) {
      ulong deadline = ti->get_deadline();
      uint q = request_q->get_resource_id();
      ulong sum = 0;
      ulong max = 0;
      foreach_task_except(tasks.get_tasks(), (*ti), tl) {
        if (tl->get_deadline() <= ti->get_deadline() ||
            !(tl->is_request_exist(q)))
          continue;

        ulong L_l_q = tl->get_request_by_id(q).get_max_length();
        if (L_l_q > max) max = L_l_q;
      }

      sum += max;

      foreach(tasks.get_tasks(), th) {
        if (th->get_deadline() >= ti->get_deadline() ||
            !(th->is_request_exist(q)))
          continue;

        sum += DBF_R(*th, q, deadline);
      }

      if (static_cast<double>(sum) / deadline > 1) {
        // cout<<"LHS:"<<(double)sum/deadline<<" deadline:"<<deadline<<endl;
        // cout<<"Condition 3 false."<<endl;
        return false;
      }
    }
  }
  return true;
}

ulong RTA_PFP_RO_FEASIBLE::DBF_R(const Task& ti, uint r_id, ulong interval) {
  ulong deadline = ti.get_deadline();
  ulong period = ti.get_period();
  const Request& request = ti.get_request_by_id(r_id);
  ulong wccs = request.get_num_requests() * request.get_max_length();

  ulong DBF = ((interval - deadline) / period + 1) * wccs;
  return max((ulong)0, DBF);
}

bool RTA_PFP_RO_FEASIBLE::alloc_schedulable() {}
