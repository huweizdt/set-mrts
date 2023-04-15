// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_GLOBAL_DAG_RTA_DAG_CGFP_H_
#define INCLUDE_SCHEDTEST_GLOBAL_DAG_RTA_DAG_CGFP_H_

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

class RTA_DAG_CGFP : public GlobalSched {
 protected:
  DAG_TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  uint32_t instance_number(const DAG_Task& ti, uint64_t interval);
  uint64_t total_workload(const DAG_Task& ti,
                          uint64_t interval);
  uint64_t response_time(const DAG_Task& ti);
 public:
  RTA_DAG_CGFP();
  RTA_DAG_CGFP(DAG_TaskSet tasks, ProcessorSet processors,
                   ResourceSet resources);
  ~RTA_DAG_CGFP();
  bool is_schedulable();
};

class RTA_DAG_CGFP_v2 : public RTA_DAG_CGFP {
 protected:
  uint64_t intra_interference(const DAG_Task& ti);
  uint64_t inter_interference(const DAG_Task& ti, uint64_t interval);

  uint64_t total_workload(const DAG_Task& ti,
                          uint64_t interval);

  uint64_t response_time(const DAG_Task& ti);
  static int w_decrease(Interf t1, Interf t2);
  uint64_t interf_cal(vector<Interf> Q, const DAG_Task &ti, uint p_num);

 public:
  RTA_DAG_CGFP_v2();
  RTA_DAG_CGFP_v2(DAG_TaskSet tasks, ProcessorSet processors,
                   ResourceSet resources);
  ~RTA_DAG_CGFP_v2();
  bool is_schedulable();
};

class RTA_DAG_CGFP_v3 : public RTA_DAG_CGFP_v2 {
 protected:
  uint64_t inter_interference(const DAG_Task& ti, uint64_t interval);
  uint64_t response_time(const DAG_Task& ti);
  uint64_t interf_cal(vector<Interf> Q, const DAG_Task &ti, uint p_num);
 public:
  RTA_DAG_CGFP_v3();
  RTA_DAG_CGFP_v3(DAG_TaskSet tasks, ProcessorSet processors,
                   ResourceSet resources);
  ~RTA_DAG_CGFP_v3();
  bool is_schedulable();
};



#endif  // INCLUDE_SCHEDTEST_GLOBAL_DAG_RTA_DAG_CGFP_H_
