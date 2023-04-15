// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_RTA_PFP_RO_FEASIBLE_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_RTA_PFP_RO_FEASIBLE_H_

#include <p_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>

class RTA_PFP_RO_FEASIBLE : public PartitionedSched {
 protected:
  TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  bool condition_1();
  bool condition_2();
  bool condition_3();
  ulong DBF_R(const Task& ti, uint r_id, ulong interval);
  bool alloc_schedulable();

 public:
  RTA_PFP_RO_FEASIBLE();
  RTA_PFP_RO_FEASIBLE(TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~RTA_PFP_RO_FEASIBLE();
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_RTA_PFP_RO_FEASIBLE_H_
