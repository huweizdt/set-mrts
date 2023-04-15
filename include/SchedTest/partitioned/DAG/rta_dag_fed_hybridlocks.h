// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_DAG_FED_HYBRIDLOCKS_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_DAG_FED_HYBRIDLOCKS_H_

/**
 * 
 * 
 */

#include <p_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>
#include <set>

class RTA_DAG_FED_HYBRIDLOCKS : public PartitionedSched {
 protected:
  DAG_TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;
  uint p_num_lu;

  // Lemma 1
  uint64_t blocking_spin(const DAG_Task& ti, uint32_t res_id);
  uint64_t blocking_spin_total(const DAG_Task& ti);

  // Lemma 2
  uint64_t blocking_susp(const DAG_Task& ti, uint32_t res_id, int32_t Y);

  // Lemma 3
  uint64_t blocking_global(const DAG_Task& ti);

  // Lemma 4
  uint64_t blocking_local(const DAG_Task& ti);

  uint64_t interference(const DAG_Task& ti, uint64_t interval);

  // Theorem 1
  uint64_t get_response_time_HUT(const DAG_Task& ti);

  // Theorem 2
  uint64_t get_response_time_LUT(const DAG_Task& ti);

  bool alloc_schedulable();
 public:
  RTA_DAG_FED_HYBRIDLOCKS();
  RTA_DAG_FED_HYBRIDLOCKS(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~RTA_DAG_FED_HYBRIDLOCKS();
  bool is_schedulable();
};

class RTA_DAG_FED_HYBRIDLOCKS_FF : public RTA_DAG_FED_HYBRIDLOCKS {
 public:
  RTA_DAG_FED_HYBRIDLOCKS_FF();
  RTA_DAG_FED_HYBRIDLOCKS_FF(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~RTA_DAG_FED_HYBRIDLOCKS_FF();
  bool is_schedulable();
};


class RTA_DAG_FED_HYBRIDLOCKS_HEAVY : public RTA_DAG_FED_HYBRIDLOCKS {
 public:
  RTA_DAG_FED_HYBRIDLOCKS_HEAVY();
  RTA_DAG_FED_HYBRIDLOCKS_HEAVY(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~RTA_DAG_FED_HYBRIDLOCKS_HEAVY();
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_DAG_FED_HYBRIDLOCKS_H_
