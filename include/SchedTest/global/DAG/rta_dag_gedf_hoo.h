// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_GLOBAL_DAG_RTA_DAG_GEDF_HOO_H_
#define INCLUDE_SCHEDTEST_GLOBAL_DAG_RTA_DAG_GEDF_HOO_H_

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


typedef struct Slack {
  uint32_t t_id;
  uint64_t slack;
} Slack;

class RTA_DAG_GEDF_HOO : public GlobalSched {
 protected:
  DAG_TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  uint64_t intra_interference(const DAG_Task& ti);
  uint64_t inter_interference(const DAG_Task& ti, uint64_t interval, vector<Slack> slacks);

  uint64_t CI_workload(const DAG_Task& tx, uint64_t interval, uint64_t slack = 0);



  uint32_t instance_number(const DAG_Task& ti, uint64_t interval);

  uint64_t total_workload(const DAG_Task& ti,
                          uint64_t interval);

  uint64_t total_workload_2(const DAG_Task& ti,
                          uint64_t interval);

  uint64_t response_time(const DAG_Task& ti, uint64_t slack = 0);

 public:
  RTA_DAG_GEDF_HOO();
  RTA_DAG_GEDF_HOO(DAG_TaskSet tasks, ProcessorSet processors,
                   ResourceSet resources);
  ~RTA_DAG_GEDF_HOO();
  bool is_schedulable();
};


class RTA_DAG_GEDF_HOO_v2 : public RTA_DAG_GEDF_HOO {
 public:
  RTA_DAG_GEDF_HOO_v2();
  RTA_DAG_GEDF_HOO_v2(DAG_TaskSet tasks, ProcessorSet processors,
                   ResourceSet resources);
  ~RTA_DAG_GEDF_HOO_v2();
  bool is_schedulable();
};




#endif  // INCLUDE_SCHEDTEST_GLOBAL_DAG_RTA_DAG_GEDF_HOO_H_
