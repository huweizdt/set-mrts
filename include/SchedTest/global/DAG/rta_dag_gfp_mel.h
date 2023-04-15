// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_GLOBAL_DAG_RTA_DAG_GFP_MEL_H_
#define INCLUDE_SCHEDTEST_GLOBAL_DAG_RTA_DAG_GFP_MEL_H_

/*
**
**
**
*/

#include <g_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>
#include <set>
#include <vector>

class RTA_DAG_GFP_MEL : public GlobalSched {
 protected:
  DAG_TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  uint64_t intra_interference(const DAG_Task& ti);
  uint64_t inter_interference(const DAG_Task& ti, uint64_t interval);

  uint32_t instance_number(const DAG_Task& ti, uint64_t interval);

  uint64_t total_workload(const DAG_Task& ti,
                          uint64_t interval);

  uint64_t response_time(const DAG_Task& ti);

 public:
  RTA_DAG_GFP_MEL();
  RTA_DAG_GFP_MEL(DAG_TaskSet tasks, ProcessorSet processors,
                   ResourceSet resources);
  ~RTA_DAG_GFP_MEL();
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_GLOBAL_DAG_RTA_DAG_GFP_MEL_H_
