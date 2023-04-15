// Copyright [2018] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_PFP_DAG_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_PFP_DAG_H_

#include <p_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>

class RTA_PFP_DAG : public PartitionedSched {
 protected:
  DAG_TaskSet tasks;
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
  bool alloc_schedulable();
  bool alloc_schedulable(Task* ti);

  ulong interf_intra(DAG_Task &ti, uint n_id);
  ulong interf_inter(DAG_Task &ti, uint n_id);
  ulong response_time_node(DAG_Task &ti, uint n_id);
  ulong response_time_task(DAG_Task &ti);

 public:
  RTA_PFP_DAG();
  RTA_PFP_DAG(DAG_TaskSet tasks, ProcessorSet processors,
              ResourceSet resources);
  ~RTA_PFP_DAG();
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_PFP_DAG_H_
