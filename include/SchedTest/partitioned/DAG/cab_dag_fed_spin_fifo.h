// Copyright [2018] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_DAG_CAB_DAG_FED_SPIN_FIFO_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_DAG_CAB_DAG_FED_SPIN_FIFO_H_

/**
 * 2018 Blocking Anaysis for Spin Locks in Real-Time Parallel Tasks
 * 
 */

#include <p_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>
#include <set>

class CAB_DAG_FED_SPIN_FIFO : public PartitionedSched {
 protected:
  DAG_TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  // Lemma 3
  uint64_t intra_wb(const DAG_Task& ti, uint32_t resource_id);

  // Lemma 4
  uint64_t intra_cpb(const DAG_Task& ti, uint32_t resource_id,
                          uint32_t request_num);

  // Lemma 5
  uint64_t inter_wb_fifo(const DAG_Task& ti, const DAG_Task& tj,
                         uint32_t resource_id);

  // Lemma 6
  uint64_t inter_cpb_fifo(const DAG_Task& ti, uint32_t resource_id,
                          uint32_t request_num);

  // Theorem 11
  uint64_t total_wb(const DAG_Task& ti);

  // Theorem 12
  uint64_t total_cpb(const DAG_Task& ti);

  // uint64_t intra_interference(const DAG_Task& ti);
  // uint64_t inter_interference(const DAG_Task& ti, uint64_t interval);

  // njobs Pg 3, Col 1, Line 21
  uint64_t instance_number(const DAG_Task& ti,
                          uint64_t interval);

  bool alloc_schedulable();
  // uint64_t total_workload(const DAG_Task& ti,
  //                         uint64_t interval);
  // uint64_t CS_workload(const DAG_Task& ti, uint32_t resource_id,
  //                      uint64_t interval);  // Critical-Section

 public:
  CAB_DAG_FED_SPIN_FIFO();
  CAB_DAG_FED_SPIN_FIFO(DAG_TaskSet tasks, ProcessorSet processors,
                   ResourceSet resources);
  ~CAB_DAG_FED_SPIN_FIFO();
  // Algorithm 1 Scheduling Test
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_DAG_CAB_DAG_FED_SPIN_FIFO_H_
