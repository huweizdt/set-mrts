// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <iteration-helper.h>
#include <math-helper.h>
#include <nc_lp_ee_vpr.h>

NC_LP_EE_VPR::NC_LP_EE_VPR() : GlobalSched(false, NC, EDF, NONE, "", "vpr") {}

NC_LP_EE_VPR::NC_LP_EE_VPR(TaskSet tasks, ProcessorSet processors,
                           ResourceSet resources)
    : GlobalSched(false, NC, EDF, NONE, "", "vpr") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;

  this->resources.update(&(this->tasks));
  this->processors.update(&(this->tasks), &(this->resources));

  this->tasks.RM_Order();
  this->processors.init();
}

bool NC_LP_EE_VPR::is_schedulable() {
  // Non-Critical-Section Condition
  if (!NCS_condition()) {
    // cout<<"NCS"<<endl;
    return false;
  }

  // Critical-Section Condition
  foreach(resources.get_resources(), resource) {
    uint r_id = resource->get_resource_id();

    if (!condition_1(r_id)) {
      cout << "CS1" << endl;
      // return false;
    }

    if (!condition_2(r_id)) {
      // cout<<"CS2"<<endl;
      return false;
    }
  }
  return true;
}

bool NC_LP_EE_VPR::condition_1(uint r_id) {
  double sum = 0;
  foreach(tasks.get_tasks(), ti) {
    if ((!ti->is_request_exist(r_id))) continue;

    double temp = 2 * ti->get_wcet_critical_sections();
    temp /= ti->get_period();

    sum += temp;
  }

  if (sum <= 1)
    return true;
  else
    return false;
}

bool NC_LP_EE_VPR::condition_2(uint r_id) {
  foreach(tasks.get_tasks(), ti) {
    if ((!ti->is_request_exist(r_id))) continue;

    ulong interval = ti->get_period() / 2;

    ulong dbf = DBF_R(r_id, interval);
    ulong bt = blocking_time(r_id, interval);
    // cout<<"interval:"<<interval<<" dbf:"<<dbf<<" bt:"<<bt<<endl;
    if ((dbf + bt) > interval) return false;
  }
  return true;
}

set<ulong> NC_LP_EE_VPR::get_time_point() {
  set<ulong> time_points;
  ulong max_period = 0;
  foreach(tasks.get_tasks(), task) {
    ulong period = task->get_period();
    if (max_period < period) max_period = period;
  }

  foreach(tasks.get_tasks(), task) {
    ulong period = task->get_period();
    for (uint i = 0;; i++) {
      ulong time_point = period / 2 + period * i;
      if (time_point <= (max_period / 2))
        time_points.insert(time_point);
      else
        break;
    }
  }
  return time_points;
}

ulong NC_LP_EE_VPR::NCS_condition() {
  uint p_num = processors.get_processor_num();
  set<ulong> time_points = get_time_point();
  foreach(time_points, t) {
    ulong sum = 0;
    foreach(tasks.get_tasks(), task) {
      ulong period = task->get_period();
      ulong wcet_nc = task->get_wcet_non_critical_sections();
      sum += (((*t) + (period / 2)) / period) * (2 * wcet_nc);
    }

    if (p_num * (*t) < sum) return false;
  }

  return true;
}

ulong NC_LP_EE_VPR::DBF_R(uint r_id, ulong interval) {
  ulong sum = 0;

  foreach(tasks.get_tasks(), ti) {
    if ((!ti->is_request_exist(r_id))) continue;

    ulong p_i = ti->get_period();
    ulong wcet_i_c = ti->get_wcet_critical_sections();
    sum += ((interval + (p_i / 2)) / p_i) * (2 * wcet_i_c);
  }
  return sum;
}

ulong NC_LP_EE_VPR::blocking_time(uint r_id, ulong interval) {
  ulong max = 0;

  foreach(tasks.get_tasks(), ti) {
    uint i = ti->get_id();
    if ((!ti->is_request_exist(r_id)) || (ti->get_period() <= interval))
      continue;

    ulong wcet_i_c = ti->get_wcet_critical_sections();
    if (max < 2 * wcet_i_c) max = 2 * wcet_i_c;
  }
  return max;
}
