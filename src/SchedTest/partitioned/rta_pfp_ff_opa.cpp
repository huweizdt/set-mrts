// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <iteration-helper.h>
#include <math-helper.h>
#include <processors.h>
#include <resources.h>
#include <rta_pfp_ff_opa.h>
#include <tasks.h>

RTA_PFP_FF_OPA::RTA_PFP_FF_OPA()
    : PartitionedSched(false, RTA, FIX_PRIORITY, NONE, "", "NONE") {}

RTA_PFP_FF_OPA::RTA_PFP_FF_OPA(TaskSet tasks, ProcessorSet processors,
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

ulong RTA_PFP_FF_OPA::interference(const Task& task, ulong interval) {
  return task.get_wcet() *
         ceiling((interval + task.get_jitter()), task.get_period());
}

/*
ulong RTA_PFP_FF_OPA::interference(Task& task, ulong interval)
{
        return task.get_wcet() * ceiling((interval + task.get_response_time() -
task.get_wcet()), task.get_period());
}
*/

ulong RTA_PFP_FF_OPA::response_time(const Task& ti) {
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

bool RTA_PFP_FF_OPA::alloc_schedulable() {
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

bool RTA_PFP_FF_OPA::BinPacking_FF(Task* ti, TaskSet* tasks,
                                   ProcessorSet* processors,
                                   ResourceSet* resources, uint MODE) {
  // processors.sort_by_task_utilization(INCREASE);

  uint p_num = processors->get_processor_num();

  Processor& processor = processors->get_processors()[0];

  for (uint k = 0; k < p_num; k++) {
    // cout<<"Try to assign Task"<<ti.get_id()<<" on processor"<<k<<endl;
    Processor& processor = processors->get_processors()[k];

    if (processor.add_task(ti->get_id())) {
      ti->set_partition(processor.get_processor_id());
    } else {
      continue;
    }

    if (!OPA(ti)) {
      ti->set_partition(MAX_INT);
      processor.remove_task(ti->get_id());
      continue;
    } else {
      return true;
    }
  }

  return false;
}

bool RTA_PFP_FF_OPA::OPA(Task* ti) {
  uint p_id = ti->get_partition();
  set<uint> unassigned_queue;
  // cout<<"OPA Task:"<<ti.get_id()<<" on processor:"<<p_id<<endl;
  foreach(tasks.get_tasks(), tx) {
    if (p_id == tx->get_partition()) {
      // cout<<"Task:"<<tx->get_id()<<endl;
      unassigned_queue.insert(tx->get_id());
    }
  }

  for (int pi = unassigned_queue.size(); pi > 0; pi--) {
    bool flag = false;
    // cout<<"Try to assign priority "<<pi<<endl;
    foreach(unassigned_queue, t_id) {
      // cout<<"\ttask:"<<*t_id<<endl;;
      Task& tx = tasks.get_task_by_id(*t_id);
      tx.set_priority(pi);

      foreach(unassigned_queue, th_id) {
        if ((*th_id) == (*t_id)) continue;

        Task& th = tasks.get_task_by_id(*th_id);
        th.set_priority(0);
        // th.set_response_time(th.get_deadline());
      }

      if (tx.get_deadline() >= response_time(tx)) {
        // cout<<"\tsuccess."<<endl;
        flag = true;
        unassigned_queue.erase(tx.get_id());
        break;
      }
      // else
      // cout<<"\ffailed."<<endl;
    }

    if (!flag) {
      foreach(unassigned_queue, t_id) {
        Task& tx = tasks.get_task_by_id(*t_id);
        tx.set_priority(MAX_INT);
      }

      return flag;
    }
  }
  return true;
}

bool RTA_PFP_FF_OPA::is_schedulable() {
  // cout << "After sorting." << endl;
  // foreach(tasks.get_tasks(), task) {
  //   uint64_t slack = task->get_deadline();
  //   slack -= task->get_wcet();
  //   cout << "Task:" << task->get_id() << " Slack:" << slack << endl;
  //   cout << "----------------------------" << endl;
  // }

  foreach(tasks.get_tasks(), ti) {
    // cout<<"<==========Task"<<ti->get_id()<<"==========>"<<endl;
    if (!BinPacking_FF(&(*ti), &tasks, &processors, &resources, TEST))
      return false;
  }
  return true;
}
