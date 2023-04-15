// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_RTA_PFP_WF_SEMAPHORE_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_RTA_PFP_WF_SEMAPHORE_H_

/*
** RTA for partitioned fix priority scheduling using spinlock protocol with
*worst-fit allocation
**
** RTA with self-suspension (to appear in RTS journal)
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

class RTA_PFP_WF_semaphore : public PartitionedSched {
 protected:
  TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  ulong transitive_preemption(const Task& ti, uint r_id);
  ulong DLB(const Task& task_i);
  ulong DGB_l(const Task& task_i);
  ulong DGB_h(const Task& task_i);
  ulong MLI(const Task& task_i);
  void calculate_total_blocking(Task* task_i);
  ulong interference(const Task& task, ulong interval);
  ulong response_time(const Task& ti);
  bool alloc_schedulable();

 public:
  RTA_PFP_WF_semaphore();
  RTA_PFP_WF_semaphore(TaskSet tasks, ProcessorSet processors,
                       ResourceSet resources);
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_RTA_PFP_WF_SEMAPHORE_H_
