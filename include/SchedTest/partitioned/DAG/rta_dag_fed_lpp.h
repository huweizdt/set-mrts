// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_DAG_FED_LPP_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_DAG_FED_LPP_H_

/**
 * 2019 DAC Scheduling and Analysis of Parallel Real-Time Tasks with Semaphores
 * 
 */

#include <p_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>
#include <set>

class RTA_DAG_FED_LPP : public PartitionedSched {
 protected:
  DAG_TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  // Lemma 5 intra-task S-Idle time
  uint64_t intra_task_blocking(const DAG_Task& ti, uint32_t res_id, uint request_num = 1);

  uint32_t maximum_request_number(const DAG_Task& ti, const DAG_Task& tj, uint32_t res_id);

  // Lemma 7 request number of inter-task S-Idle time
  uint32_t inter_task_request_number(const DAG_Task& ti, const DAG_Task& tj,
                                     uint32_t res_id, uint request_num = 1);

  // Lemma 8 inter-task S-Idle time
  uint64_t inter_task_blocking(const DAG_Task& ti, uint32_t res_id, uint request_num = 1);

  // Lemma 9 total blocking time
  uint64_t total_blocking_time(const DAG_Task& ti);

  // Theorem 1
  uint64_t get_response_time(const DAG_Task& ti);

  bool alloc_schedulable();
 public:
  RTA_DAG_FED_LPP();
  RTA_DAG_FED_LPP(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~RTA_DAG_FED_LPP();
  // Algorithm 1 Scheduling Test
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_DAG_FED_LPP_H_
