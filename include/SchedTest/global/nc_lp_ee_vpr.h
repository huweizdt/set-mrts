// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_GLOBAL_NC_LP_EE_VPR_H_
#define INCLUDE_SCHEDTEST_GLOBAL_NC_LP_EE_VPR_H_

/**
 * 
 */

#include <g_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>
#include <set>

using std::set;

class NC_LP_EE_VPR : public GlobalSched {
 protected:
  TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  set<ulong> get_time_point();
  ulong DBF_R(uint r_id, ulong interval);
  ulong blocking_time(uint r_id, ulong interval);
  ulong NCS_condition();
  bool condition_1(uint r_id);
  bool condition_2(uint r_id);

 public:
  NC_LP_EE_VPR();
  NC_LP_EE_VPR(TaskSet tasks, ProcessorSet processors, ResourceSet resources);
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_GLOBAL_NC_LP_EE_VPR_H_
