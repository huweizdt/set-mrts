// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_RTA_PFP_GS_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_RTA_PFP_GS_H_

#include <p_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>
#include <types.h>
#include <vector>

using std::vector;

typedef struct {
  uint p_id;
  int64_t s;
  uint priority;
} gs_tryAssign;

typedef struct {
  uint t_id;
  uint priority;
} priority_storage;

typedef vector<priority_storage> Priorities;

class RTA_PFP_GS : public PartitionedSched {
 protected:
  TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  ulong pfp_gs_local_blocking(const Task& ti);
  ulong pfp_gs_spin_time(const Task& ti, uint resource_id);
  ulong pfp_gs_remote_blocking(const Task& ti);
  ulong pfp_gs_NP_blocking(const Task& ti);
  ulong response_time(Task* ti);
  bool alloc_schedulable();
  bool alloc_schedulable(Task* ti);
  int64_t pfp_gs_tryAssign(Task* ti, uint p_id);

 public:
  RTA_PFP_GS();
  RTA_PFP_GS(TaskSet tasks, ProcessorSet processors, ResourceSet resources);
  ~RTA_PFP_GS();
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_RTA_PFP_GS_H_
