// Copyright [2018] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_DAG_CAB_DAG_FED_SPIN_PRIO_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_DAG_CAB_DAG_FED_SPIN_PRIO_H_

/**
 * 2018 Blocking Anaysis for Spin Locks in Real-Time Parallel Tasks
 * 
 */

#include <p_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>
#include <set>

class CAB_DAG_FED_SPIN_PRIO : public PartitionedSched {
 protected:
  DAG_TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  // Lemma 3
  uint64_t intra_wb(const DAG_Task& ti, uint32_t resource_id);

  // Lemma 4
  uint64_t intra_cpb(const DAG_Task& ti, uint32_t resource_id,
                          uint32_t request_num);

  // higher Pg 6, Col 1, Line 23
  uint64_t higher(const DAG_Task& ti, uint32_t resource_id, uint64_t interval);

  // lower Pg 6, Col 1, Line 8
  uint64_t lower(const DAG_Task& ti, uint32_t resource_id);

  // equal Pg 6, Col 1, Line 13
  uint64_t equal(const DAG_Task& ti, uint32_t resource_id);

  // dpr Pg 6, Col 1, Line 3
  uint64_t dpr(const DAG_Task& ti, uint32_t resource_id);

  // Lemma 7
  uint64_t inter_wb_lower(const DAG_Task& ti, uint32_t resource_id);

  // Lemma 8
  uint64_t inter_wb_higher(const DAG_Task& ti, uint32_t resource_id);

  // Lemma 9
  uint64_t inter_cpb_lower(const DAG_Task& ti, uint32_t resource_id,
                          uint32_t request_num);

  // Lemma 10
  uint64_t inter_cpb_higher(const DAG_Task& ti, uint32_t resource_id,
                          uint32_t request_num);

  // Theorem 13
  uint64_t total_wb(const DAG_Task& ti);

  // Theorem 14
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
  CAB_DAG_FED_SPIN_PRIO();
  CAB_DAG_FED_SPIN_PRIO(DAG_TaskSet tasks, ProcessorSet processors,
                   ResourceSet resources);
  ~CAB_DAG_FED_SPIN_PRIO();
  // Algorithm 1 Scheduling Test
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_DAG_CAB_DAG_FED_SPIN_PRIO_H_
