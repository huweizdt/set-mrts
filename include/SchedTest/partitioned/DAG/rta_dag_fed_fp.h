// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_DAG_FED_FP_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_DAG_FED_FP_H_

/**
 * 2014 RTSS federated scheduling [FP]
 * 
 */

#include <p_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>
#include <set>

class RTA_DAG_FED_FP : public PartitionedSched {
 protected:
  DAG_TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  bool alloc_schedulable();
 public:
  RTA_DAG_FED_FP();
  RTA_DAG_FED_FP(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~RTA_DAG_FED_FP();
  bool is_schedulable();
};

class RTA_DAG_FED_FP_HEAVY : public RTA_DAG_FED_FP {
 public:
  RTA_DAG_FED_FP_HEAVY();
  RTA_DAG_FED_FP_HEAVY(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~RTA_DAG_FED_FP_HEAVY();
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_DAG_FED_FP_H_
