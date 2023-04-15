// Copyright [2018] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <lp.h>
#include <math-helper.h>
#include <processors.h>
#include <resources.h>
#include <rta_dag_gfp_spin_fifo.h>
#include <solution.h>
#include <tasks.h>
#include <iostream>
#include <sstream>

using std::exception;
using std::max;
using std::min;
using std::ostringstream;

RTA_DAG_GFP_SPIN_FIFO::RTA_DAG_GFP_SPIN_FIFO()
    : GlobalSched(false, RTA, FIX_PRIORITY, SPIN, "", "") {}

RTA_DAG_GFP_SPIN_FIFO::RTA_DAG_GFP_SPIN_FIFO(DAG_TaskSet tasks,
                                             ProcessorSet processors,
                                             ResourceSet resources)
    : GlobalSched(false, RTA, FIX_PRIORITY, SPIN, "", "") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;

  this->resources.update(&(this->tasks));
  this->processors.update(&(this->tasks), &(this->resources));

  this->tasks.RM_Order();
  this->processors.init();
}

RTA_DAG_GFP_SPIN_FIFO::~RTA_DAG_GFP_SPIN_FIFO() {}

uint64_t RTA_DAG_GFP_SPIN_FIFO::intra_wb(const DAG_Task& ti,
                                         uint32_t resource_id) {
  // uint32_t n_i = ti.get_dcores();
  int32_t n_i = min(ti.get_parallel_degree(), processors.get_processor_num());
  const Request& request_q = ti.get_request_by_id(resource_id);
  int32_t N_i_q = request_q.get_num_requests();
  int64_t L_i_q = request_q.get_max_length();

  int32_t m = min(N_i_q, n_i);

  return ((m * (m - 1) / 2) + (n_i - 1) * max((N_i_q - n_i), 0)) * L_i_q;
}

uint64_t RTA_DAG_GFP_SPIN_FIFO::intra_cpb(const DAG_Task& ti,
                                          uint32_t resource_id,
                                          uint32_t request_num) {
  uint32_t n_i = min(ti.get_parallel_degree(), processors.get_processor_num());
  const Request& request_q = ti.get_request_by_id(resource_id);
  uint32_t N_i_q = request_q.get_num_requests();
  uint64_t L_i_q = request_q.get_max_length();
  return min((n_i - 1) * request_num, N_i_q - request_num) * L_i_q;
}

uint64_t RTA_DAG_GFP_SPIN_FIFO::inter_wb_fifo(const DAG_Task& ti,
                                              const DAG_Task& tj,
                                              uint32_t resource_id) {
  uint32_t n_i = min(ti.get_parallel_degree(), processors.get_processor_num());
  uint32_t n_j = min(tj.get_parallel_degree(), processors.get_processor_num());
  const Request& request_i_q = ti.get_request_by_id(resource_id);
  uint32_t N_i_q = request_i_q.get_num_requests();
  const Request& request_j_q = tj.get_request_by_id(resource_id);
  uint32_t N_j_q = request_j_q.get_num_requests();
  uint32_t L_j_q = request_j_q.get_max_length();
  uint64_t d_i = ti.get_deadline();

  return min((uint64_t)(N_i_q * n_j), instance_number(tj, d_i) * N_j_q * n_i) *
         L_j_q;
}

uint64_t RTA_DAG_GFP_SPIN_FIFO::inter_cpb_fifo(const DAG_Task& ti,
                                               uint32_t resource_id,
                                               uint32_t request_num) {
  uint64_t sum = 0;
  foreach_task_except(tasks.get_tasks(), ti, tx) {
    if (!tx->is_request_exist(resource_id)) continue;

    uint32_t n_x =
        min(tx->get_parallel_degree(), processors.get_processor_num());
    const Request& request_x_q = tx->get_request_by_id(resource_id);
    uint32_t N_x_q = request_x_q.get_num_requests();
    uint32_t L_x_q = request_x_q.get_max_length();
    uint64_t d_i = ti.get_deadline();
    sum += min((uint64_t)(n_x * request_num),
               instance_number((*tx), d_i) * N_x_q) *
           L_x_q;
  }
  return sum;
}

uint64_t RTA_DAG_GFP_SPIN_FIFO::total_wb(const DAG_Task& ti) {
  uint64_t total_b = 0;
  foreach(ti.get_requests(), request) {
    uint32_t q = request->get_resource_id();
    total_b += intra_wb(ti, q);
    foreach_task_except(tasks.get_tasks(), ti, tx) {
      if (tx->is_request_exist(q)) total_b += inter_wb_fifo(ti, (*tx), q);
    }
  }
  return total_b;
}

uint64_t RTA_DAG_GFP_SPIN_FIFO::total_cpb(const DAG_Task& ti) {
  uint64_t total_cpb = 0;
  foreach(ti.get_requests(), request) {
    uint32_t q = request->get_resource_id();
    uint32_t N_i_q = request->get_num_requests();
    uint64_t max_temp = 0;
    for (uint i = 1; i <= N_i_q; i++) {
      uint64_t temp = intra_cpb(ti, q, i) + inter_cpb_fifo(ti, q, i);
      // cout << "intra_cpb:" << intra_cpb(ti, q, i)
      //      << " inter_cpb:" << inter_cpb_fifo(ti, q, i) << endl;
      if (temp > max_temp) max_temp = temp;
    }
    total_cpb += max_temp;
  }
  return total_cpb;
}

uint64_t RTA_DAG_GFP_SPIN_FIFO::intra_interference(const DAG_Task& ti) {
  return ti.get_wcet() - ti.get_critical_path_length();
}

uint64_t RTA_DAG_GFP_SPIN_FIFO::inter_interference(const DAG_Task& ti,
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
      inter_interf += instance_number((*th), interval) *
                      total_wb((*th));
    }
  } catch (exception& e) {
    // cout << e.what() << endl;
  }

  // foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
  //   foreach(tl->get_requests(), l_u) {
  //     uint32_t r_id = l_u->get_resource_id();
  //     Resource& resource = resources.get_resource_by_id(r_id);
  //     if (resource.get_ceiling() < ti.get_priority()) {
  //       // cout << "CS workload of task " << tl->get_id() << ": " <<
  //       CS_workload((*tl), r_id, interval); inter_interf +=
  //       CS_workload((*tl), r_id, interval);
  //     }
  //   }
  // }
  return inter_interf;
}

uint64_t RTA_DAG_GFP_SPIN_FIFO::instance_number(const DAG_Task& ti,
                                                uint64_t interval) {
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  uint32_t MPD = min(processors.get_processor_num(),
                     ti.get_parallel_degree());  // Minimum Parallel Degree
  return ceiling(interval + r - e / MPD, p);
}

uint64_t RTA_DAG_GFP_SPIN_FIFO::total_workload(const DAG_Task& ti,
                                               uint64_t interval) {
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  uint32_t MPD = min(processors.get_processor_num(),
                     ti.get_parallel_degree());  // Minimum Parallel Degree
  return ceiling(interval + r - e / MPD, p) * e;
}

uint64_t RTA_DAG_GFP_SPIN_FIFO::CS_workload(const DAG_Task& ti,
                                            uint32_t resource_id,
                                            uint64_t interval) {
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  const Request& request = ti.get_request_by_id(resource_id);
  uint64_t agent_length = request.get_num_requests() * request.get_max_length();
  return ceiling((interval + r - agent_length), p) * agent_length;
}

uint64_t RTA_DAG_GFP_SPIN_FIFO::response_time(const DAG_Task& ti) {
  uint32_t t_id = ti.get_id();
  uint64_t CPL = ti.get_critical_path_length();
  uint64_t intra_interf = intra_interference(ti);
  uint64_t total_w_blocking = total_wb(ti);
  uint64_t total_cp_blocking = total_cpb(ti);
  // uint64_t intra_w_blocking = 0;
  // uint64_t intra_cp_blocking = 0;
  uint p_num = processors.get_processor_num();

  // cout << "total_w_blocking: " << total_w_blocking << " total_cp_blocking: " << total_cp_blocking << endl;

  uint64_t test_end = ti.get_deadline();
  uint64_t test_start =
      CPL + total_cp_blocking + (intra_interf + total_w_blocking - total_cp_blocking) / p_num;
  uint64_t response = test_start;
  uint64_t demand = 0;

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

bool RTA_DAG_GFP_SPIN_FIFO::is_schedulable() {
  uint64_t response_bound;

  foreach(tasks.get_tasks(), ti) {
    // cout << "Task:" << ti->get_id() << " Priority:" << ti->get_priority() <<
    // " Deadline:" << ti->get_deadline() << endl;
    response_bound = response_time((*ti));
    // cout << "response time:" << response_bound << endl;
    // cout << "total wb:" << total_wb((*ti)) << endl;
    // cout << "total cpb:" << total_cpb((*ti)) << endl;
    if (response_bound > ti->get_deadline()) {
      return false;
    }
    ti->set_response_time(response_bound);
  }
  // foreach(tasks.get_tasks(), task) {
  //   cout << "Task " << task->get_id() << ": wb " << total_wb((*task))
  //         << " cpb " << total_cpb((*task)) << endl;
  // }
  return true;
}
