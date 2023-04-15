// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_RTA_PFP_WF_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_RTA_PFP_WF_H_

/*
** RTA for partitioned scheduling with worst-fit allocation
**
** Audsly et al.'s standard RTA (1993)
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

class RTA_PFP_WF : public PartitionedSched {
 protected:
  TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  ulong interference(const Task& task, ulong interval);
  ulong response_time(const Task& ti);
  bool alloc_schedulable();

 public:
  RTA_PFP_WF();
  RTA_PFP_WF(TaskSet tasks, ProcessorSet processors, ResourceSet resources);
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_RTA_PFP_WF_H_
