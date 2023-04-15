// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <iteration-helper.h>
#include <math-helper.h>
#include <math.h>
#include <rta_gfp_ng.h>

using std::max;
using std::min;

RTA_GFP_NG::RTA_GFP_NG()
    : GlobalSched(false, RTA, FIX_PRIORITY, NONE, "", "NG") {}

RTA_GFP_NG::RTA_GFP_NG(TaskSet tasks, ProcessorSet processors,
                       ResourceSet resources)
    : GlobalSched(false, RTA, FIX_PRIORITY, NONE, "", "NG") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;
  this->tasks.RM_Order();

  this->resources.update(&(this->tasks));
  this->processors.update(&(this->tasks), &(this->resources));

  this->processors.init();
  // this->tasks.display();
}

int RTA_GFP_NG::interference_gap_decrease(Carry_in t1, Carry_in t2) {
  return t1.gap > t2.gap;
}

ulong RTA_GFP_NG::workload_nc(const Task& task, ulong interval) {
  ulong C_i = task.get_wcet();
  ulong T_i = task.get_period();
  return floor(interval / T_i) * C_i + min(C_i, interval % T_i);
}

ulong RTA_GFP_NG::workload_ci(const Task& task, ulong interval) {
  ulong C_i = task.get_wcet();
  ulong T_i = task.get_period();
  ulong R_i = task.get_response_time();
  ulong zero = 0;
  ulong a = min(C_i - 1,
                max(zero, max(zero, interval - C_i) % T_i - (T_i - R_i)));
                // cout << "a:" << a << endl;
  return floor(max(zero, interval - C_i) / T_i) * C_i + C_i + a;
}

ulong RTA_GFP_NG::interference_nc(const Task& task_k, const Task& task_i,
                                  ulong interval) {
  return min(interval - task_k.get_wcet() + 1,
             max((ulong)0, workload_nc(task_i, interval)));
}

ulong RTA_GFP_NG::interference_ci(const Task& task_k, const Task& task_i,
                                  ulong interval) {
  // cout << "interval:" << interval << " wci:" << workload_ci(task_i, interval) << endl;
  return min(interval - task_k.get_wcet() + 1,
             max((ulong)0, workload_ci(task_i, interval)));
}

ulong RTA_GFP_NG::total_interference(const Task& task_k, ulong interval) {
  // Task& task_k = tasks.get_task_by_id(t_id);
  ulong I_nc_sum = 0, I_ci_sum = 0;
  uint k = task_k.get_priority();
  uint p_num = processors.get_processor_num();
  uint ci_num;
  vector<Carry_in> carry_in;

  foreach_higher_priority_task(tasks.get_tasks(), task_k, task_i) {
    Carry_in temp;
    uint64_t ICI = interference_ci(task_k, *task_i, interval);
    uint64_t INC = interference_nc(task_k, *task_i, interval);
    // cout << "ICI:" << ICI << " INC:" << INC << endl;
    temp.priority = task_i->get_priority();
    temp.gap = ICI - INC;

    // cout << "temp priority:" << temp.priority << " temp gap:" << temp.gap << endl;
    carry_in.push_back(temp);
  }

  sort(carry_in.begin(), carry_in.end(), interference_gap_decrease);

  // foreach(carry_in, ci) {
    // cout << "priority:" << ci->priority << " gap:" << ci->gap << endl;
  // }

  if (0 == k) {
    ci_num = 0;
  } else if (k < p_num) {
    ci_num = k - 1;
  } else {
    ci_num = p_num - 1;
  }

  // cout << ci_num << endl;
  for (uint i = 0; i < ci_num; i++) {
    uint priority = carry_in[i].priority;

    Task& task = tasks.get_task_by_priority(priority);
    task.set_carry_in();
  }

  foreach_higher_priority_task(tasks.get_tasks(), task_k, task_i) {
    if (task_i->is_carry_in()) {
      I_ci_sum += interference_ci(task_k, *task_i, interval);
      // cout<<"carry_in id:"<<task_i.get_id()<<endl;
    } else {
      I_nc_sum += interference_nc(task_k, *task_i, interval);
    }
  }

  for (uint i = 0; i < ci_num; i++) {
    uint priority = carry_in[i].priority;
    Task& task = tasks.get_task_by_priority(priority);
    task.clear_carry_in();
  }
  // cout << "I_nc_sum:" << I_nc_sum << " I_ci_sum:" << I_ci_sum << endl;
  return I_nc_sum + I_ci_sum;
}

ulong RTA_GFP_NG::response_time(const Task& ti) {
  uint t_id = ti.get_id();
  ulong wcet = ti.get_wcet();
  ulong test_end = ti.get_deadline();
  ulong test_start = ti.get_wcet();
  ulong response = test_start;
  ulong demand = 0;
  ulong omega = 0;
  uint p_num = processors.get_processor_num();
  // cout<<"id:"<<t_id<<endl;
  // tasks.display();
  bool test = (wcet == test_end);

  // if (test) {
  // foreach(tasks.get_tasks(), task) {
  //   cout << "Task " << task->get_id() << ":" << endl;
  //   cout << "WCET:" << task->get_wcet() << " Deadline:" << task->get_deadline() << " Period:" << task->get_period() << " Gap:" << task->get_deadline() - task->get_wcet() << endl;
  //   cout << "-----------------------" << endl;
  // }
  // }

  while (response <= test_end) {
    omega = total_interference(ti, response);

    // if (test) {
    //   cout << "task:" << t_id << "total interference:" << omega << endl;
    // }

    // cout << "omega:" << omega << endl;
    demand = floor(omega / p_num) + wcet;
    // cout << response << " " << demand << endl;

    // assert(response <= demand);

    if (response >= demand) {
      // if (test) {
      //   cout << "task:" << t_id << "response:" << response << endl;
      // }

      return demand;
    } else {
      response = demand;
    }
  }
  /*
  if(test)
  {
          cout<<"task:"<<t_id<<"response:"<<response<<endl;
  }
  */
  return test_end + 100;
}

bool RTA_GFP_NG::is_schedulable() {
  ulong response_bound;

  // for (uint i = 0; i < tasks.get_taskset_size(); i ++)
  foreach(tasks.get_tasks(), ti) {
    // Task& ti = tasks.get_task_by_id(i);
    ti->set_response_time(ti->get_wcet());
    // ulong original_bound = ti->get_response_time();
    response_bound = response_time((*ti));
    if (response_bound > ti->get_deadline()) {
      // cout<<"RTA failed on task:"<<ti->get_id()<<endl;
      return false;
    }
    ti->set_response_time(response_bound);
  }
  return true;
}
