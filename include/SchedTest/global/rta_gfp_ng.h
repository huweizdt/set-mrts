// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_GLOBAL_RTA_GFP_NG_H_
#define INCLUDE_SCHEDTEST_GLOBAL_RTA_GFP_NG_H_

/**
* Nan Guan's Method RTA for global scheduling with constraint deadline task
* system
*
* RTSS 2009 Nan Guan [New Response Time Bounds for Fixed priority
* Multiprocessor Scheudling]
*/

#include <g_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>

class RTA_GFP_NG : public GlobalSched {
 protected:
  TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  typedef struct {
    uint priority;
    ulong gap;
  } Carry_in;

  static int interference_gap_decrease(Carry_in t1, Carry_in t2);
  ulong workload_nc(const Task& task, ulong interval);
  ulong workload_ci(const Task& task, ulong interval);
  ulong interference_nc(const Task& task_k, const Task& task_i, ulong interval);
  ulong interference_ci(const Task& task_k, const Task& task_i, ulong interval);
  ulong total_interference(const Task& task_k, ulong interval);
  ulong response_time(const Task& ti);

 public:
  RTA_GFP_NG();
  RTA_GFP_NG(TaskSet tasks, ProcessorSet processors, ResourceSet resources);
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_GLOBAL_RTA_GFP_NG_H_
