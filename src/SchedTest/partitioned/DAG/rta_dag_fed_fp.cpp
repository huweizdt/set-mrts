// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <rta_dag_fed_fp.h>
#include <rta_pfp_wf.h>
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

RTA_DAG_FED_FP::RTA_DAG_FED_FP()
    : PartitionedSched(false, RTA, FIX_PRIORITY, FMLP, "", "") {}

RTA_DAG_FED_FP::RTA_DAG_FED_FP(DAG_TaskSet tasks,
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

RTA_DAG_FED_FP::~RTA_DAG_FED_FP() {}

bool RTA_DAG_FED_FP::alloc_schedulable() {}

bool RTA_DAG_FED_FP::is_schedulable() {
  uint32_t p_num = processors.get_processor_num();
  TaskSet l_tasks;
  uint32_t allocated = 0;
  foreach(tasks.get_tasks(), ti) {
    if (ti->get_critical_path_length() > ti->get_deadline())
      return false;
    if (1 < ti->get_utilization()) {  // heavy tasks
      uint64_t C_i = ti->get_wcet();
      uint64_t L_i = ti->get_critical_path_length();
      uint64_t D_i = ti->get_deadline();
      uint64_t T_i = ti->get_period();
      uint32_t initial_m_i = ceiling(C_i - L_i, D_i - L_i);
      ti->set_dcores(initial_m_i);
      allocated += initial_m_i;
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
    if (0 == p_num && 0 != l_tasks.get_taskset_size())
      return false;
    ProcessorSet r_proc = ProcessorSet(p_num);
    ResourceSet resources = ResourceSet();
    // cout << "p_num:" << p_num << endl;
    l_tasks.init();
    l_tasks.RM_Order();
    r_proc.init();
    resources.init();
    RTA_PFP_WF test = RTA_PFP_WF(l_tasks, r_proc, resources);
    return test.is_schedulable();
  }
}


///////////////////////////////////////


RTA_DAG_FED_FP_HEAVY::RTA_DAG_FED_FP_HEAVY()
    : RTA_DAG_FED_FP() {}

RTA_DAG_FED_FP_HEAVY::RTA_DAG_FED_FP_HEAVY(DAG_TaskSet tasks,
                                         ProcessorSet processors,
                                         ResourceSet resources)
    : RTA_DAG_FED_FP(tasks, processors, resources) {
}

RTA_DAG_FED_FP_HEAVY::~RTA_DAG_FED_FP_HEAVY() {}

bool RTA_DAG_FED_FP_HEAVY::is_schedulable() {
  uint32_t p_num = processors.get_processor_num();
  uint32_t allocated = 0;
  foreach(tasks.get_tasks(), ti) {
    if (ti->get_critical_path_length() > ti->get_deadline())
      return false;
    uint64_t C_i = ti->get_wcet();
    uint64_t L_i = ti->get_critical_path_length();
    uint64_t D_i = ti->get_deadline();
    uint64_t T_i = ti->get_period();
    uint32_t initial_m_i = ceiling(C_i - L_i, D_i - L_i);
    ti->set_dcores(initial_m_i);
    allocated += initial_m_i;
  }
  bool update = false;

  if (allocated > p_num) {
    return false;
  } else {
    return true;
  }
}
