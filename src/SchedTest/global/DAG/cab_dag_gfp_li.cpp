// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <lp.h>
#include <math-helper.h>
#include <processors.h>
#include <resources.h>
#include <cab_dag_gfp_li.h>
#include <solution.h>
#include <tasks.h>
#include <iostream>
#include <sstream>
#include <exception>

using std::min;
using std::exception;
using std::ostringstream;

CAB_DAG_GFP_LI::CAB_DAG_GFP_LI()
    : GlobalSched(false, RTA, FIX_PRIORITY, FMLP, "", "") {}

CAB_DAG_GFP_LI::CAB_DAG_GFP_LI(DAG_TaskSet tasks, ProcessorSet processors,
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

CAB_DAG_GFP_LI::~CAB_DAG_GFP_LI() {}

bool CAB_DAG_GFP_LI::is_schedulable() {
  uint p_num = processors.get_processor_num();
  uint64_t response_bound;

  double cab = (4.0 - (1 / p_num) +
                    sqrt(12.0 - (4.0 / p_num) + (1.0 / (p_num * p_num)))) /
                   2.0;

  // cout << cab << endl;

  if (p_num < (tasks.get_utilization_sum() * cab))
    return false;

  foreach(tasks.get_tasks(), ti) {
    if (ti->get_deadline() < (ti->get_critical_path_length() * cab))
      return false;
  }
  return true;
}
