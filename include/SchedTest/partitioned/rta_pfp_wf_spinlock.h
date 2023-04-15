// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_RTA_PFP_WF_SPINLOCK_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_RTA_PFP_WF_SPINLOCK_H_

/*
** RTA for partitioned fix priority scheduling using spinlock protocol with
*worst-fit allocation
**
** Awieder and Brandenburg's RTA with spin locks (2013)
*/

#include <p_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>

/*
class Task;
class TaskSet;
class Request;
class ProcessorSet;
class ResourceSet;
*/

class RTA_PFP_WF_spinlock : public PartitionedSched {
 protected:
  TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  ulong interference(const Task& task, ulong interval);
  void calculate_spin();
  void calculate_local_blocking();
  ulong response_time(const Task& ti);
  bool alloc_schedulable();

 public:
  RTA_PFP_WF_spinlock();
  RTA_PFP_WF_spinlock(TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_RTA_PFP_WF_SPINLOCK_H_
