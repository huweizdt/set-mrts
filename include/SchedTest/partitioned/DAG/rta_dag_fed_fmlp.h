// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_DAG_FED_FMLP_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_DAG_FED_FMLP_H_

/**
 * 2019 DAC Scheduling and Analysis of Parallel Real-Time Tasks with Semaphores
 * 
 */

#include <p_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>
#include <set>

class RTA_DAG_FED_FMLP : public PartitionedSched {
 protected:
  DAG_TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  uint32_t instance_number(const DAG_Task& ti, uint64_t interval);

  uint64_t inter_task_blocking(const DAG_Task& ti, uint32_t res_id,
                               uint32_t request_num, uint64_t interval);

  uint64_t intra_task_blocking(const DAG_Task& ti, uint32_t res_id,
                               uint32_t request_num);

  uint64_t indirect_blocking(const DAG_Task& ti, uint64_t interval);

  uint64_t total_blocking_time(const DAG_Task& ti, uint64_t interval);

  uint64_t get_response_time(const DAG_Task& ti);

  bool alloc_schedulable();
 public:
  RTA_DAG_FED_FMLP();
  RTA_DAG_FED_FMLP(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~RTA_DAG_FED_FMLP();
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_DAG_FED_FMLP_H_
