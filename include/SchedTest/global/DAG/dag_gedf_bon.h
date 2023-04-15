// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_GLOBAL_DAG_DAG_GEDF_BON_H_
#define INCLUDE_SCHEDTEST_GLOBAL_DAG_DAG_GEDF_BON_H_

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

typedef struct Work {
  ulong time;
  ulong workload;
  bool operator<(const Work &a) const {
    if (time == a.time)
      return workload < a.workload;
    else
      return time < a.time;
  }
} Work;

typedef struct {
  ulong r_t;
  ulong f_t;
} Exec_Range;

template <typename Type>
Type set_member(set<Type> s, int index) {
  typename std::set<Type>::iterator it = s.begin();
  for (int i = 0; i < index; i++) it++;
  return *(it);
}

class DAG_GEDF_BON : public GlobalSched {
 protected:
  DAG_TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;
  double e;

  set<Work> precise_workload(const DAG_Task& dag_task, ulong T);
  ulong approximate_workload(const DAG_Task& dag_task, ulong T);
  double approximation(const DAG_TaskSet& dag_taskset, double e);

 public:
  DAG_GEDF_BON();
  DAG_GEDF_BON(DAG_TaskSet tasks, ProcessorSet processors,
               ResourceSet resources, double e = 0.1);
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_GLOBAL_DAG_DAG_GEDF_BON_H_
