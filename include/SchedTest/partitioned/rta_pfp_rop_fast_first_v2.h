// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_RTA_PFP_ROP_FAST_FIRST_V2_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_RTA_PFP_ROP_FAST_FIRST_V2_H_

#include <p_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>

static uint ROP_FF_P_2 = 0;

class RTA_PFP_ROP_FAST_FIRST_V2 : public PartitionedSched {
 protected:
  TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  ulong blocking_bound(const Task& ti, uint r_id);
  ulong request_bound(const Task& ti, uint r_id);
  ulong formula_30(const Task& ti, uint p_id, ulong interval);
  ulong angent_exec_bound(const Task& ti, uint p_id, ulong interval);
  ulong NCS_workload(const Task& ti, ulong interval);  // Non-Critical-Section
  ulong CS_workload(const Task& ti, uint resource_id,
                    ulong interval);  // Critical-Section
  ulong response_time_AP(const Task& ti);
  ulong response_time_SP(const Task& ti);
  bool worst_fit_for_resources(uint active_processor_num);
  bool is_first_fit_for_tasks_schedulable(uint start_processor);
  bool is_worst_fit_for_tasks_schedulable();
  bool alloc_schedulable();
  bool alloc_schedulable(Task* ti);

 public:
  RTA_PFP_ROP_FAST_FIRST_V2();
  RTA_PFP_ROP_FAST_FIRST_V2(TaskSet tasks, ProcessorSet processors,
                            ResourceSet resources);
  ~RTA_PFP_ROP_FAST_FIRST_V2();
  double get_total_power();
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_RTA_PFP_ROP_FAST_FIRST_V2_H_
