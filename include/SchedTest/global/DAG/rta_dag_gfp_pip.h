// Copyright [2018] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_GLOBAL_DAG_RTA_DAG_GFP_PIP_H_
#define INCLUDE_SCHEDTEST_GLOBAL_DAG_RTA_DAG_GFP_PIP_H_

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

class RTA_DAG_GFP_PIP : public GlobalSched {
 protected:
  DAG_TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  // uint64_t resource_holding(const DAG_Task& ti, const DAG_Task& tx, uint32_t resource_id);
  // uint64_t phi(const DAG_Task& ti, uint32_t resource_id);
  // uint64_t theta(const DAG_Task& ti, uint32_t resource_id);
  // uint64_t request_bound(const DAG_Task& ti, uint32_t resource_id);
  uint64_t intra_blocking(const DAG_Task& ti, uint32_t resource_id,
                          uint32_t request_num);
  uint64_t inter_blocking(const DAG_Task& ti, uint32_t resource_id,
                          uint32_t request_num);
  uint64_t intra_interference(const DAG_Task& ti);
  uint64_t inter_interference(const DAG_Task& ti, uint64_t interval);

  uint32_t instance_number(const DAG_Task& ti,
                          uint64_t interval);
  uint64_t total_workload(const DAG_Task& ti,
                          uint64_t interval);
  uint64_t CS_workload(const DAG_Task& ti, uint32_t resource_id,
                       uint64_t interval);  // Critical-Section

  uint64_t response_time(const DAG_Task& ti);

 public:
  RTA_DAG_GFP_PIP();
  RTA_DAG_GFP_PIP(DAG_TaskSet tasks, ProcessorSet processors,
                   ResourceSet resources);
  ~RTA_DAG_GFP_PIP();
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_GLOBAL_DAG_RTA_DAG_GFP_PIP_H_
