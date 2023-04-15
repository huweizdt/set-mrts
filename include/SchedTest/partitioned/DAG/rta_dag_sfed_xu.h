// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_DAG_SFED_XU_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_DAG_SFED_XU_H_

/**
 * 2017 RTSS semi-federated scheduling
 * 
 */

#include <p_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>
#include <set>

class RTA_DAG_SFED_XU : public PartitionedSched {
 protected:
  DAG_TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  bool BinPacking_WF(TaskSet tasks, ProcessorSet processors);

  bool alloc_schedulable();
 public:
  RTA_DAG_SFED_XU();
  RTA_DAG_SFED_XU(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~RTA_DAG_SFED_XU();
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_DAG_SFED_XU_H_
