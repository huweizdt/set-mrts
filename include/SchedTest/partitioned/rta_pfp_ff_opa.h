// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_RTA_PFP_FF_OPA_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_RTA_PFP_FF_OPA_H_

/*
**
**
**
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

class RTA_PFP_FF_OPA : public PartitionedSched {
 protected:
  TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  ulong interference(const Task& task, ulong interval);
  ulong response_time(const Task& ti);
  bool alloc_schedulable();
  bool BinPacking_FF(Task* ti, TaskSet* tasks,
                     ProcessorSet* processors,
                     ResourceSet* resources, uint MODE);
  bool OPA(Task* ti);

 public:
  RTA_PFP_FF_OPA();
  RTA_PFP_FF_OPA(TaskSet tasks, ProcessorSet processors, ResourceSet resources);
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_RTA_PFP_FF_OPA_H_
