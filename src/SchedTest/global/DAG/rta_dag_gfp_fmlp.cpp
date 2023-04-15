// Copyright [2018] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <lp.h>
#include <math-helper.h>
#include <processors.h>
#include <resources.h>
#include <rta_dag_gfp_fmlp.h>
#include <solution.h>
#include <tasks.h>
#include <iostream>
#include <sstream>
#include <exception>

using std::min;
using std::exception;
using std::ostringstream;

RTA_DAG_GFP_FMLP::RTA_DAG_GFP_FMLP()
    : GlobalSched(false, RTA, FIX_PRIORITY, FMLP, "", "") {}

RTA_DAG_GFP_FMLP::RTA_DAG_GFP_FMLP(DAG_TaskSet tasks, ProcessorSet processors,
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

RTA_DAG_GFP_FMLP::~RTA_DAG_GFP_FMLP() {}

uint32_t RTA_DAG_GFP_FMLP::instance_number(const DAG_Task& ti,
                                                uint64_t interval) {
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  return ceiling(interval + r, p);
}

uint64_t RTA_DAG_GFP_FMLP::total_workload(const DAG_Task& ti,
                                          uint64_t interval) {
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  uint32_t p_num = processors.get_processor_num();
  // uint32_t MPD = min(processors.get_processor_num(),
  //                    ti.get_parallel_degree());  // Minimum Parallel Degree
  // return ceiling(interval + r - e / MPD, p) * e;
  return ceiling(interval + r - e / p_num, p) * e;
}

uint64_t RTA_DAG_GFP_FMLP::CS_workload(const DAG_Task& ti, uint32_t resource_id,
                                    uint64_t interval) {
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  const Request& request = ti.get_request_by_id(resource_id);
  uint64_t agent_length = request.get_num_requests() * request.get_max_length();
  return ceiling((interval + r - agent_length), p) * agent_length;
}

uint64_t RTA_DAG_GFP_FMLP::intra_blocking(const DAG_Task& ti,
                                          uint32_t resource_id,
                                          uint32_t request_num) {
  const Request& request = ti.get_request_by_id(resource_id);
  uint32_t N_i_q = request.get_num_requests();
  uint64_t L_i_q = request.get_max_length();
  uint32_t X_i_q = request_num;
  uint32_t Y_i_q = request.get_num_requests() - X_i_q;
  uint32_t m_i = ti.get_parallel_degree();
  return min((m_i - 1) * X_i_q, Y_i_q) * L_i_q;
}

uint64_t RTA_DAG_GFP_FMLP::inter_blocking(const DAG_Task& ti,
                                          uint32_t resource_id,
                                          uint32_t request_num) {
  const Request& request = ti.get_request_by_id(resource_id);
  uint32_t N_i_q = request.get_num_requests();
  uint64_t L_i_q = request.get_max_length();
  uint32_t X_i_q = request_num;
  uint32_t Y_i_q = request.get_num_requests() - X_i_q;
  uint64_t d_i = ti.get_deadline();
  uint64_t sum = 0;
  foreach_task_except(tasks.get_tasks(), ti, tx) {
    if (tx->is_request_exist(resource_id)) {
      const Request& request_x = tx->get_request_by_id(resource_id);
      uint32_t N_x_q = request_x.get_num_requests();
      uint64_t L_x_q = request_x.get_max_length();
      // uint32_t MPD = min(processors.get_processor_num(),
      //                tx->get_parallel_degree());  // Minimum Parallel Degree
      uint32_t m_x = tx->get_parallel_degree();
      sum +=
          min(min(m_x, N_x_q) * X_i_q, (instance_number((*tx), d_i) * N_x_q))
          * L_x_q;
    }
  }
  return sum;
}

uint64_t RTA_DAG_GFP_FMLP::intra_interference(const DAG_Task& ti) {
  return ti.get_wcet() - ti.get_critical_path_length();
}

uint64_t RTA_DAG_GFP_FMLP::inter_interference(const DAG_Task& ti,
                                              uint64_t interval) {
  // cout << "inter_interf." << endl;
  uint64_t inter_interf = 0;
  try {
    foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
      // cout << "H-Task" << th->get_id() << endl;
      // cout << "Priority:" << th->get_priority() <<endl;
      // cout << "workload of task " << th->get_id() << ": " <<
      // total_workload((*th), interval);
      inter_interf += total_workload((*th), interval);
    }
  } catch (exception& e) {
    // cout << e.what() << endl;
  }

  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    foreach(tl->get_requests(), l_u) {
      uint32_t r_id = l_u->get_resource_id();
      Resource& resource = resources.get_resource_by_id(r_id);
      if (resource.get_ceiling() < ti.get_priority()) {
        // cout << "CS workload of task " << tl->get_id() << ": " <<
        // CS_workload((*tl), r_id, interval);
        inter_interf += CS_workload((*tl), r_id, interval);
      }
    }
  }
  return inter_interf;
}

uint64_t RTA_DAG_GFP_FMLP::response_time(const DAG_Task& ti) {
  uint32_t t_id = ti.get_id();
  // cout << "ID:" << t_id << endl;
  uint64_t CPL = ti.get_critical_path_length();
  // cout << "CPL:" << CPL << endl;
  uint64_t intra_interf = intra_interference(ti);
  // cout << "intra interference:" << intra_interf << endl;
  uint64_t total_blocking = 0;
  uint p_num = processors.get_processor_num();

  foreach(ti.get_requests(), request) {
    uint32_t r_id = request->get_resource_id();
    // cout << "r_id:" << r_id << endl;
    uint64_t blocking = 0;
    uint32_t N_i_q = request->get_num_requests();
    // cout << "N_i_q:" << N_i_q << endl;
    for (uint32_t x = 1; x <= N_i_q; x++) {
      // cout << "X=" << x << endl;
      // cout << "intra_b:" << intra_blocking(ti, r_id, x) << endl;
      // cout << "inter_b:" << inter_blocking(ti, r_id, x) << endl;
      uint64_t temp = intra_blocking(ti, r_id, x) + inter_blocking(ti, r_id, x);
      if (temp > blocking)
        blocking = temp;
    }
    total_blocking += blocking;
  }

  // cout << "total blocking:" << total_blocking << endl;

  uint64_t test_end = ti.get_deadline();
  uint64_t test_start = CPL + total_blocking + intra_interf / p_num;
  uint64_t response = test_start;
  uint64_t demand = 0;

  // cout << "FMLP" << endl;
  // cout << "total blocking:" << total_blocking << endl;

  // foreach(tasks.get_tasks(), task) {
  //   cout << "Task" << task->get_id() << endl;
  //   cout << "Priority:" << task->get_priority() <<endl;
  // }

  // cout << "FMLP" << endl;


  while (response <= test_end) {
    uint64_t inter_interf = inter_interference(ti, response);

    // cout << "inter interference:" << inter_interf << endl;

    demand = test_start + inter_interf / p_num;

    if (response == demand) {
      return response;
    } else {
      response = demand;
    }
  }
return test_end + 100;
}

bool RTA_DAG_GFP_FMLP::is_schedulable() {
  uint64_t response_bound;

  foreach(tasks.get_tasks(), ti) {
    // cout << "Task:" << ti->get_id() << " Priority:" << ti->get_priority() <<
    // " Deadline:" << ti->get_deadline() << endl;
    response_bound = response_time((*ti));
    // cout << "response time:" << response_bound << endl;
    if (response_bound > ti->get_deadline()) {
      return false;
    }
    ti->set_response_time(response_bound);
  }
  return true;
}

