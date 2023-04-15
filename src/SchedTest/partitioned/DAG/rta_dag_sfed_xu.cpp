// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <rta_dag_sfed_xu.h>
#include <iteration-helper.h>
#include <lp.h>
#include <math-helper.h>
#include <processors.h>
#include <resources.h>
#include <solution.h>
#include <tasks.h>
#include <iostream>
#include <sstream>

using std::max;
using std::min;
using std::ostringstream;

RTA_DAG_SFED_XU::RTA_DAG_SFED_XU()
    : PartitionedSched(false, RTA, FIX_PRIORITY, FMLP, "", "") {}

RTA_DAG_SFED_XU::RTA_DAG_SFED_XU(DAG_TaskSet tasks,
                                         ProcessorSet processors,
                                         ResourceSet resources)
    : PartitionedSched(false, RTA, FIX_PRIORITY, FMLP, "", "") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;

  this->resources.update(&(this->tasks));
  this->processors.update(&(this->tasks), &(this->resources));

  this->tasks.RM_Order();
  this->processors.init();
}

RTA_DAG_SFED_XU::~RTA_DAG_SFED_XU() {}

bool RTA_DAG_SFED_XU::alloc_schedulable() {}

// Algorithm 2 SF[x+1]
bool RTA_DAG_SFED_XU::is_schedulable() {
  uint32_t p_num = processors.get_processor_num();
  TaskSet l_tasks;
  uint32_t allocated = 0;
  foreach(tasks.get_tasks(), ti) {
    if (1 < ti->get_utilization()) {  // heavy tasks
      uint64_t C_i = ti->get_wcet();
      uint64_t L_i = ti->get_critical_path_length();
      uint64_t D_i = ti->get_deadline();
      uint64_t T_i = ti->get_period();
      uint32_t initial_m_i = (C_i - L_i)/(D_i - L_i);
      double container_load = C_i - L_i;
      container_load /= D_i - L_i;
      container_load -= initial_m_i;
      ti->set_dcores(initial_m_i);
      allocated += initial_m_i;
      uint64_t workload = container_load * T_i;
      l_tasks.add_task(workload, T_i, D_i);
    } else {  // light tasks
      uint64_t C_i = ti->get_wcet();
      uint64_t L_i = ti->get_critical_path_length();
      uint64_t D_i = ti->get_deadline();
      uint64_t T_i = ti->get_period();
      l_tasks.add_task(C_i, T_i, D_i);
    }
  }
  bool update = false;

  if (allocated > p_num) {
    return false;
  } else {
    p_num -= allocated;
    if (0 == p_num)
      return false;
    ProcessorSet r_proc = ProcessorSet(p_num);
    return BinPacking_WF(l_tasks, r_proc);
  }
}


bool RTA_DAG_SFED_XU::BinPacking_WF(TaskSet tasks, ProcessorSet processors) {
  tasks.sort_by_density();
  processors.update(&tasks, NULL);
  foreach(tasks.get_tasks(), ti) {
    processors.sort_by_task_utilization(INCREASE);
    Processor& processor = processors.get_processors()[0];
    if (processor.get_utilization() + ti->get_utilization() <= 1) {
      ti->set_partition(processor.get_processor_id());
      if (!processor.add_task(ti->get_id()))
        return false;
    } else {
      return false;
    }
  }
  return true;
}

