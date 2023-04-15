// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_GLOBAL_DAG_CAB_DAG_GEDF_LI_H_
#define INCLUDE_SCHEDTEST_GLOBAL_DAG_CAB_DAG_GEDF_LI_H_

/*
**
**
**
*/

#include <g_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>
#include <set>
#include <vector>

class CAB_DAG_GEDF_LI : public GlobalSched {
 protected:
  DAG_TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

 public:
  CAB_DAG_GEDF_LI();
  CAB_DAG_GEDF_LI(DAG_TaskSet tasks, ProcessorSet processors,
                   ResourceSet resources);
  ~CAB_DAG_GEDF_LI();
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_GLOBAL_DAG_CAB_DAG_GEDF_LI_H_
