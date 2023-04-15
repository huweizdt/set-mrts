// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_RTA_PDC_RO_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_RTA_PDC_RO_H_

#include <p_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>

class RTA_PDC_RO : public PartitionedSched {
 protected:
  TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  ulong ro_workload(const Task& ti, ulong interval);
  ulong ro_agent_workload(const Task& ti, uint resource_id, ulong interval);
  ulong ro_get_bloking(const Task& ti);
  ulong ro_get_interference_R(const Task& ti, ulong interval);
  ulong ro_get_interference_AC(const Task& ti, ulong interval);
  ulong ro_get_interference_UC(const Task& ti, ulong interval);
  ulong ro_get_interference(const Task& ti, ulong interval);
  ulong response_time(const Task& ti);
  bool worst_fit_for_resources(uint active_processor_num);
  bool is_first_fit_for_tasks_schedulable(uint start_processor);
  bool alloc_schedulable();

 public:
  RTA_PDC_RO();
  RTA_PDC_RO(TaskSet tasks, ProcessorSet processors, ResourceSet resources);
  ~RTA_PDC_RO();
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_RTA_PDC_RO_H_
