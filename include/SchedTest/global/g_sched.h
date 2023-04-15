// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_GLOBAL_G_SCHED_H_
#define INCLUDE_SCHEDTEST_GLOBAL_G_SCHED_H_

#include <sched_test_base.h>
#include <types.h>
#include <string>

class Task;
class TaskSet;
class Request;
class ProcessorSet;
class ResourceSet;

class GlobalSched : public SchedTestBase {
 public:
  // enum
  // {
  //         UNTEST,
  //         TEST
  // };

  GlobalSched(bool LinearProgramming, uint TestMethod, uint PriorityAssignment,
              uint LockingProtocol = NONE, string name = "",
              string remark = "");

  virtual ~GlobalSched();
};

#endif  // INCLUDE_SCHEDTEST_GLOBAL_G_SCHED_H_
