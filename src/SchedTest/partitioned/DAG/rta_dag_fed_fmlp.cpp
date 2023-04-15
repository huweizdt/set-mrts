// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <rta_dag_fed_fmlp.h>
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

RTA_DAG_FED_FMLP::RTA_DAG_FED_FMLP()
    : PartitionedSched(false, RTA, FIX_PRIORITY, FMLP, "", "") {}

RTA_DAG_FED_FMLP::RTA_DAG_FED_FMLP(DAG_TaskSet tasks,
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

RTA_DAG_FED_FMLP::~RTA_DAG_FED_FMLP() {}

bool RTA_DAG_FED_FMLP::alloc_schedulable() {}

// Algorithm 1 Scheduling Test
bool RTA_DAG_FED_FMLP::is_schedulable() {
  uint32_t p_num = processors.get_processor_num();
  foreach(tasks.get_tasks(), ti) {
    uint64_t C_i = ti->get_wcet();
    uint64_t L_i = ti->get_critical_path_length();
    uint64_t D_i = ti->get_deadline();
    uint32_t initial_m_i = ceiling(C_i - L_i, D_i - L_i);
    ti->set_dcores(initial_m_i);
  }
  bool update = false;

  while (true) {
    update = false;
    foreach(tasks.get_tasks(), ti) {
      uint64_t D_i = ti->get_deadline();
      uint64_t response_time = get_response_time((*ti));
      if (response_time > D_i) {
        ti->set_dcores(1 + ti->get_dcores());
        update = true;
      }
    }

    uint32_t sum_core = 0;
    foreach(tasks.get_tasks(), ti) {
      sum_core += ti->get_dcores();
    }
    if (sum_core > p_num)
      return false;
    if (!update)
      return true;
  }
}

uint32_t RTA_DAG_FED_FMLP::instance_number(const DAG_Task& ti,
                                            uint64_t interval) {
  // uint64_t cpl = ti.get_critical_path_length();
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  uint32_t MPD = min(ti.get_parallel_degree(),
                     ti.get_dcores());  // Minimum Parallel Degree
  return ceiling(interval + r - e/MPD, p);
}

uint64_t RTA_DAG_FED_FMLP::inter_task_blocking(const DAG_Task& ti,
                                               uint32_t res_id,
                                               uint32_t request_num,
                                               uint64_t interval) {
  const Request& request = ti.get_request_by_id(res_id);
  uint32_t X_i_q = request_num;
  uint64_t sum = 0;
  foreach_task_except(tasks.get_tasks(), ti, tx) {
    if (tx->is_request_exist(res_id)) {
      const Request& request_x = tx->get_request_by_id(res_id);
      uint32_t N_x_q = request_x.get_num_requests();
      uint64_t L_x_q = request_x.get_max_length();
      uint64_t temp = min(tx->get_parallel_degree(), N_x_q);
      uint64_t ceiling_n = instance_number((*tx), interval) * N_x_q;
      sum += min(temp * X_i_q, ceiling_n) * L_x_q;
    }
  }
  return sum;
}

uint64_t RTA_DAG_FED_FMLP::intra_task_blocking(const DAG_Task& ti,
                                               uint32_t res_id,
                                               uint32_t request_num) {
  const Request& request = ti.get_request_by_id(res_id);
  uint64_t L_i_q = request.get_max_length();
  uint32_t X_i_q = request_num;
  uint32_t Y_i_q = request.get_num_requests() - X_i_q;
  uint32_t MPD = ti.get_parallel_degree();
  return min((MPD - 1) * X_i_q, Y_i_q) * L_i_q;
}

uint64_t RTA_DAG_FED_FMLP::indirect_blocking(const DAG_Task& ti, uint64_t interval) {
  uint64_t sum = 0;
  foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
    uint64_t sub_sum = 0;
    uint32_t n_h = th->get_dcores();
    foreach(th->get_requests(), r_h_q) {
      uint32_t r_id = r_h_q->get_resource_id();
      if (ti.is_request_exist(r_id))
        continue;
      sub_sum += r_h_q->get_max_length() * r_h_q->get_num_requests();
    }
    sum += sub_sum/n_h;
  }
  return sum;
}

uint64_t RTA_DAG_FED_FMLP::total_blocking_time(const DAG_Task& ti,
                                               uint64_t interval) {
  uint64_t total_blocking = 0;
  foreach(ti.get_requests(), request) {
    uint32_t r_id = request->get_resource_id();
    uint64_t blocking = 0;
    uint32_t N_i_q = request->get_num_requests();
    for (uint32_t x = 1; x <= N_i_q; x++) {
      uint64_t temp = intra_task_blocking(ti, r_id, x) +
                      inter_task_blocking(ti, r_id, x, interval);
      if (temp > blocking)
        blocking = temp;
    }
    total_blocking += blocking;
  }
  total_blocking += indirect_blocking(ti, interval);
  return total_blocking;
}

uint64_t RTA_DAG_FED_FMLP::get_response_time(const DAG_Task& ti) {
  uint64_t response_time = 0;
  uint32_t m_i = ti.get_dcores();
  response_time +=
      (ti.get_wcet() + (m_i - 1) * ti.get_critical_path_length()) / m_i;
  response_time += total_blocking_time(ti, response_time);
  return response_time;
}
