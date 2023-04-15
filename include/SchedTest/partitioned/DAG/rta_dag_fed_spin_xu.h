// Copyright [2020] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_DAG_FED_SPIN_FIFO_XU_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_DAG_FED_SPIN_FIFO_XU_H_

/**
 * 
 * 
 */

#include <p_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>
#include <set>

class RTA_DAG_FED_SPIN_FIFO_XU : public PartitionedSched {
 protected:
  DAG_TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  // Lemma 6 intra-task I^I time
  uint64_t intra_task_blocking(const DAG_Task& ti, uint32_t res_id, uint32_t request_num);

  // 
  uint32_t maximum_request_number(const DAG_Task& ti, const DAG_Task& tj,
                                     uint32_t res_id);

  // Lemma 7 inter-task I^O time
  uint64_t inter_task_blocking(const DAG_Task& ti, uint32_t res_id, uint32_t request_num);

  // Lemma 8 total blocking time
  uint64_t total_blocking_time(const DAG_Task& ti);

  // Theorem 2
  uint64_t get_response_time(const DAG_Task& ti);

  bool alloc_schedulable();
 public:
  RTA_DAG_FED_SPIN_FIFO_XU();
  RTA_DAG_FED_SPIN_FIFO_XU(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~RTA_DAG_FED_SPIN_FIFO_XU();
  // Algorithm 1 Scheduling Test
  bool is_schedulable();
};


class RTA_DAG_FED_SPIN_PRIO_XU : public PartitionedSched {
 protected:
  DAG_TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  uint64_t intra_task_blocking(const DAG_Task& ti, uint32_t res_id, uint32_t request_num);

  uint32_t maximum_request_number(const DAG_Task& ti, const DAG_Task& tj,
                                     uint32_t res_id);
                                     
  uint64_t higher(const DAG_Task& ti, uint32_t res_id, uint64_t interval);
                                     
  uint64_t lower(const DAG_Task& ti, uint32_t res_id);
                                     
  uint64_t equal(const DAG_Task& ti, uint32_t res_id);

  uint64_t dpr(const DAG_Task& ti, uint32_t res_id);

  uint64_t inter_task_blocking(const DAG_Task& ti, uint32_t res_id, uint32_t request_num);

  uint64_t total_blocking_time(const DAG_Task& ti);

  uint64_t get_response_time(const DAG_Task& ti);

  bool alloc_schedulable();
 public:
  RTA_DAG_FED_SPIN_PRIO_XU();
  RTA_DAG_FED_SPIN_PRIO_XU(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~RTA_DAG_FED_SPIN_PRIO_XU();
  
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_DAG_FED_SPIN_FIFO_XU_H_
