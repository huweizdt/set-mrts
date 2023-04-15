// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_DAG_FED_LI_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_DAG_FED_LI_H_

/**
 * 2014 RTSS federated scheduling [EDF]
 * 
 */

#include <p_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>
#include <set>

class RTA_DAG_FED_LI : public PartitionedSched {
 protected:
  DAG_TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  bool BinPacking_WF(TaskSet tasks, ProcessorSet processors);

  bool alloc_schedulable();
 public:
  RTA_DAG_FED_LI();
  RTA_DAG_FED_LI(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~RTA_DAG_FED_LI();
  bool is_schedulable();
};

class RTA_DAG_FED_LI_HEAVY : public RTA_DAG_FED_LI {
 public:
  RTA_DAG_FED_LI_HEAVY();
  RTA_DAG_FED_LI_HEAVY(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~RTA_DAG_FED_LI_HEAVY();
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_DAG_FED_LI_H_
