// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_GLOBAL_RTA_GFP_BC_H_
#define INCLUDE_SCHEDTEST_GLOBAL_RTA_GFP_BC_H_

/**
 * Bertogna&Cirinei's RTA for global scheduling
 * RTSS 2009 Nan Guan [New Response Time Bounds for Fixed priority Multiprocess
 * or Scheudling]
 */

#include <g_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>

class RTA_GFP_BC : public GlobalSched {
 protected:
  TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  ulong workload(const Task& task, ulong interval);
  ulong response_time(const Task& ti);
  ulong interference(const Task& tk, const Task& ti, ulong interval);

 public:
  RTA_GFP_BC();
  RTA_GFP_BC(TaskSet tasks, ProcessorSet processors, ResourceSet resources);
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_GLOBAL_RTA_GFP_BC_H_
