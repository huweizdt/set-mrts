// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <lp.h>
#include <math-helper.h>
#include <processors.h>
#include <resources.h>
#include <rta_dag_gfp_mel.h>
#include <solution.h>
#include <tasks.h>
#include <iostream>
#include <sstream>
#include <exception>

using std::min;
using std::exception;
using std::ostringstream;

RTA_DAG_GFP_MEL::RTA_DAG_GFP_MEL()
    : GlobalSched(false, RTA, FIX_PRIORITY, FMLP, "", "") {}

RTA_DAG_GFP_MEL::RTA_DAG_GFP_MEL(DAG_TaskSet tasks, ProcessorSet processors,
                                   ResourceSet resources)
    : GlobalSched(false, RTA, FIX_PRIORITY, FMLP, "", "") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;

  this->resources.update(&(this->tasks));
  this->processors.update(&(this->tasks), &(this->resources));

  this->tasks.init();
  this->tasks.RM_Order();
  this->processors.init();
}

RTA_DAG_GFP_MEL::~RTA_DAG_GFP_MEL() {}

uint32_t RTA_DAG_GFP_MEL::instance_number(const DAG_Task& ti,
                                            uint64_t interval) {
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  uint32_t MPD = min(ti.get_parallel_degree(),
                     ti.get_dcores());  // Minimum Parallel Degree
  return ceiling(interval + r - e/MPD, p);
}

uint64_t RTA_DAG_GFP_MEL::total_workload(const DAG_Task& ti,
                                           uint64_t interval) {
  uint32_t p_num = processors.get_processor_num();
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  uint64_t workload = ceiling(interval + r - e/p_num, p) * e;
  return workload;
}


uint64_t RTA_DAG_GFP_MEL::intra_interference(const DAG_Task& ti) {
  return ti.get_wcet() - ti.get_critical_path_length();
}

uint64_t RTA_DAG_GFP_MEL::inter_interference(const DAG_Task& ti,
                                               uint64_t interval) {
  uint64_t inter_interf = 0;
  try {
    foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
      inter_interf += total_workload((*tx), interval);
    }
  } catch (exception& e) {
    cout << e.what() << endl;
  }
  return inter_interf;
}

uint64_t RTA_DAG_GFP_MEL::response_time(const DAG_Task& ti) {
  uint32_t t_id = ti.get_id();
  uint64_t CPL = ti.get_critical_path_length();
  uint64_t intra_interf = intra_interference(ti);
  uint p_num = processors.get_processor_num();

  uint64_t test_end = ti.get_deadline();
  uint64_t test_start = CPL + intra_interf / p_num;

  uint64_t response = test_start;
  uint64_t demand = 0;

  while (response <= test_end) {
    uint64_t inter_interf = inter_interference(ti, response);
    demand = test_start + inter_interf / p_num;

    if (response == demand) {
      return response;
    } else {
      response = demand;
    }
  }
  return test_end + 100;
}

bool RTA_DAG_GFP_MEL::is_schedulable() {
  uint64_t response_bound;

  foreach(tasks.get_tasks(), ti) {
    response_bound = response_time((*ti));
    if (response_bound > ti->get_deadline()) {
      return false;
    }
    ti->set_response_time(response_bound);
  }
  return true;
}
