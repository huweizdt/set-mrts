// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_GLOBAL_RTA_GFP_NATIVE_H_
#define INCLUDE_SCHEDTEST_GLOBAL_RTA_GFP_NATIVE_H_

/**
* native RTA for global scheduling with constraint deadline task system
* RTSS 2009 Nan Guan et al. [New Response Time Bounds for Fixed priority
* Multiprocessor Scheudling]
*/

#include <g_sched.h>
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

class RTA_GFP_native : public GlobalSched {
 protected:
  TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  ulong workload(const Task& task, ulong interval);
  ulong response_time(const Task& ti);

 public:
  RTA_GFP_native();
  RTA_GFP_native(TaskSet tasks, ProcessorSet processors, ResourceSet resources);
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_GLOBAL_RTA_GFP_NATIVE_H_
