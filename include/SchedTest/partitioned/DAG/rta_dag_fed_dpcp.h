// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_DAG_FED_DPCP_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_DAG_FED_DPCP_H_

/**
 * 2019 DAC Scheduling and Analysis of Parallel Real-Time Tasks with Semaphores
 * 
 */

#include <p_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>
#include <set>

typedef vector<Path> Paths;

class RTA_DAG_FED_DPCP : public PartitionedSched {
 protected:
  DAG_TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;
  Paths paths;

  uint32_t instance_number(const DAG_Task& ti, uint64_t interval);

  uint64_t gamma(const DAG_Task& ti, uint32_t r_id, uint64_t interval);

  uint64_t resource_holding(const DAG_Task& ti, Path path, uint32_t r_id);

  uint64_t inter_blocking(const DAG_Task &ti, Path path, uint64_t interval);

  uint64_t intra_blocking(const DAG_Task &ti, Path path);

  uint64_t inter_interf(const DAG_Task &ti, Path path, uint64_t interval);

  uint64_t intra_interf(const DAG_Task &ti, Path path);

  bool is_local_resource(uint r_id);

  uint32_t get_ceiling(uint r_id);

  bool WFD_Resources();

  // Theorem 1
  uint64_t get_response_time(const DAG_Task& ti);

  bool alloc_schedulable();
 public:
  RTA_DAG_FED_DPCP();
  RTA_DAG_FED_DPCP(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~RTA_DAG_FED_DPCP();
  // Algorithm 1 Scheduling Test
  bool is_schedulable();
};


class RTA_DAG_FED_DPCP_v2 : public RTA_DAG_FED_DPCP {
 protected:

  uint64_t inter_blocking(const DAG_Task &ti, uint64_t interval);

  uint64_t intra_blocking(const DAG_Task &ti);

  uint64_t inter_interf(const DAG_Task &ti, uint64_t interval);

  uint64_t intra_interf(const DAG_Task &ti);

  // Theorem 1
  uint64_t get_response_time(const DAG_Task& ti);

 public:
   public:
  RTA_DAG_FED_DPCP_v2();
  RTA_DAG_FED_DPCP_v2(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~RTA_DAG_FED_DPCP_v2();
  // Algorithm 1 Scheduling Test
  bool is_schedulable();
};


#endif  // INCLUDE_SCHEDTEST_PARTITIONED_DAG_RTA_DAG_FED_DPCP_H_
