#ifndef INCLUDE_SCHEDTEST_PARTITIONED_RTA_PSO_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_RTA_PSO_H_

#include <p_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>


class RTA_PSO  {

 protected:
  BPTaskSet taskset;
  ProcessorSet processorset;
  ResourceSet resourceset;

  ulong interference(const BPTask& task, ulong interval);
  ulong response_time(BPTaskSet tasks,const BPTask& ti);
  bool alloc_schedulable();

 public:
  double fitness(int* schedule);
  RTA_PSO(BPTaskSet taskset, ProcessorSet processorset, ResourceSet resourceset);
  static bool is_schedulable(int* schedule) ;
  void add_sort(int** solution_arry,int* new_solution,int num_tasks,int arrySize);
  int** PSO(int num_partical,int num_epoch);
  void partition(int* solution_arry);
  void jitter_fit(int t);
  ulong max_jitter(int t);
  ulong RTA_PSO::jitter_total(int t);
  BPTaskSet  choose_solution(int** solution_arry,int arrySize);
  
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_RTA_PFP_ROP_FAST_FIRST_H_
