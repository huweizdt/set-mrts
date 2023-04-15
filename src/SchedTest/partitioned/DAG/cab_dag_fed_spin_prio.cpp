// Copyright [2018] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <cab_dag_fed_spin_prio.h>
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

CAB_DAG_FED_SPIN_PRIO::CAB_DAG_FED_SPIN_PRIO()
    : PartitionedSched(false, CAB, FIX_PRIORITY, SPIN, "", "") {}

CAB_DAG_FED_SPIN_PRIO::CAB_DAG_FED_SPIN_PRIO(DAG_TaskSet tasks,
                                             ProcessorSet processors,
                                             ResourceSet resources)
    : PartitionedSched(false, CAB, FIX_PRIORITY, SPIN, "", "") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;

  this->resources.update(&(this->tasks));
  this->processors.update(&(this->tasks), &(this->resources));

  this->tasks.RM_Order();
  this->processors.init();
}

CAB_DAG_FED_SPIN_PRIO::~CAB_DAG_FED_SPIN_PRIO() {}

bool CAB_DAG_FED_SPIN_PRIO::alloc_schedulable() {}

uint64_t CAB_DAG_FED_SPIN_PRIO::intra_wb(const DAG_Task& ti,
                                         uint32_t resource_id) {
  int32_t n_i = ti.get_dcores();
  const Request& request_q = ti.get_request_by_id(resource_id);
  int32_t N_i_q = request_q.get_num_requests();
  uint64_t L_i_q = request_q.get_max_length();

  uint32_t m = min(N_i_q, n_i);

  return ((m * (m - 1) / 2) + (n_i - 1) * max((N_i_q - n_i), 0)) * L_i_q;
}

uint64_t CAB_DAG_FED_SPIN_PRIO::intra_cpb(const DAG_Task& ti,
                                          uint32_t resource_id,
                                          uint32_t request_num) {
  uint32_t n_i = ti.get_dcores();
  const Request& request_q = ti.get_request_by_id(resource_id);
  uint32_t N_i_q = request_q.get_num_requests();
  int64_t L_i_q = request_q.get_max_length();
  return min((n_i - 1) * request_num, N_i_q - request_num) * L_i_q;
}

uint64_t CAB_DAG_FED_SPIN_PRIO::higher(const DAG_Task& ti, uint32_t resource_id,
                                       uint64_t interval) {
  uint64_t sum = 0;
  foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
    if (th->is_request_exist(resource_id)) {
      const Request& request_q = th->get_request_by_id(resource_id);
      uint32_t n = instance_number((*th), interval);
      uint32_t N_h_q = request_q.get_num_requests();
      uint64_t L_h_q = request_q.get_max_length();
      sum += n * N_h_q * L_h_q;
    }
  }
  return sum;
}

uint64_t CAB_DAG_FED_SPIN_PRIO::lower(const DAG_Task& ti,
                                      uint32_t resource_id) {
  uint64_t max = 0;
  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    if (tl->is_request_exist(resource_id)) {
      const Request& request_q = tl->get_request_by_id(resource_id);
      uint64_t L_l_q = request_q.get_max_length();
      if (L_l_q > max) max = L_l_q;
    }
  }
  return max;
}

uint64_t CAB_DAG_FED_SPIN_PRIO::equal(const DAG_Task& ti,
                                      uint32_t resource_id) {
  if (ti.is_request_exist(resource_id)) {
    const Request& request_q = ti.get_request_by_id(resource_id);
    uint32_t N_i_q = request_q.get_num_requests();
    uint64_t L_i_q = request_q.get_max_length();
    uint32_t n_i = ti.get_dcores();
    uint32_t m_i = min(n_i - 1, N_i_q - 1);
    return m_i * L_i_q;
  } else {
    return 0;
  }
}

uint64_t CAB_DAG_FED_SPIN_PRIO::dpr(const DAG_Task& ti, uint32_t resource_id) {
  uint64_t test_start = lower(ti, resource_id) + equal(ti, resource_id);
  uint64_t test_end = ti.get_deadline();
  uint64_t delay = test_start;

  while (delay <= test_end) {
    uint64_t temp = test_start;

    temp += higher(ti, resource_id, delay);

    if (temp > delay)
      delay = temp;
    else if (temp == delay)
      return delay;
  }
  return test_end + 100;
}

uint64_t CAB_DAG_FED_SPIN_PRIO::inter_wb_lower(const DAG_Task& ti,
                                               uint32_t resource_id) {
  if (ti.is_request_exist(resource_id)) {
    const Request& request_q = ti.get_request_by_id(resource_id);
    uint32_t N_i_q = request_q.get_num_requests();
    uint64_t max = 0;
    foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
      if (tl->is_request_exist(resource_id)) {
        const Request& request_q = tl->get_request_by_id(resource_id);
        uint64_t L_l_q = request_q.get_max_length();
        if (L_l_q > max) max = L_l_q;
      }
    }
    return N_i_q * max;
  } else {
    return 0;
  }
}

uint64_t CAB_DAG_FED_SPIN_PRIO::inter_wb_higher(const DAG_Task& ti,
                                                uint32_t resource_id) {
  uint64_t sum = 0;
  if (ti.is_request_exist(resource_id)) {
    const Request& request_i_q = ti.get_request_by_id(resource_id);
    foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
      if (th->is_request_exist(resource_id)) {
        uint64_t d_i = ti.get_deadline();
        const Request& request_h_q = th->get_request_by_id(resource_id);
        uint32_t N_i_q = request_i_q.get_num_requests();
        uint32_t N_h_q = request_h_q.get_num_requests();
        uint64_t L_h_q = request_h_q.get_max_length();
        uint32_t n_h = instance_number((*th), dpr(ti, resource_id));
        uint32_t n_i = ti.get_dcores();
        sum +=
            min((instance_number((*th), dpr(ti, resource_id)) * N_i_q * N_h_q),
                (instance_number((*th), d_i) * N_h_q * n_i)) *
            L_h_q;
      }
    }
  }
  return sum;
}

uint64_t CAB_DAG_FED_SPIN_PRIO::inter_cpb_lower(const DAG_Task& ti,
                                                uint32_t resource_id,
                                                uint32_t request_num) {
  uint64_t max = 0;
  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    if (tl->is_request_exist(resource_id)) {
      const Request& request_q = tl->get_request_by_id(resource_id);
      uint64_t L_l_q = request_q.get_max_length();
      if (L_l_q > max) max = L_l_q;
    }
  }
  return request_num * max;
}

uint64_t CAB_DAG_FED_SPIN_PRIO::inter_cpb_higher(const DAG_Task& ti,
                                                 uint32_t resource_id,
                                                 uint32_t request_num) {
  uint64_t sum = 0;
  if (ti.is_request_exist(resource_id)) {
    const Request& request_i_q = ti.get_request_by_id(resource_id);
    foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
      if (th->is_request_exist(resource_id)) {
        uint64_t d_i = ti.get_deadline();
        const Request& request_h_q = th->get_request_by_id(resource_id);
        uint32_t N_i_q = request_i_q.get_num_requests();
        uint32_t N_h_q = request_h_q.get_num_requests();
        uint64_t L_h_q = request_h_q.get_max_length();
        uint32_t n_h = instance_number((*th), dpr(ti, resource_id));
        sum += min((instance_number((*th), dpr(ti, resource_id)) * N_h_q *
                    request_num),
                   (instance_number((*th), d_i) * N_h_q)) *
               L_h_q;
      }
    }
  }
  return sum;
}

uint64_t CAB_DAG_FED_SPIN_PRIO::total_wb(const DAG_Task& ti) {
  uint64_t total_b = 0;
  foreach(ti.get_requests(), request) {
    uint32_t q = request->get_resource_id();
    uint64_t wb = intra_wb(ti, q);
    uint64_t wb_h = inter_wb_higher(ti, q);
    uint64_t wb_l = inter_wb_lower(ti, q);
    total_b += intra_wb(ti, q) + inter_wb_higher(ti, q) + inter_wb_lower(ti, q);
  }
  return total_b;
}

uint64_t CAB_DAG_FED_SPIN_PRIO::total_cpb(const DAG_Task& ti) {
  uint64_t total_cpb = 0;
  foreach(ti.get_requests(), request) {
    uint32_t q = request->get_resource_id();
    uint32_t N_i_q = request->get_num_requests();
    uint64_t max_temp = 0;
    for (uint i = 1; i <= N_i_q; i++) {
      uint64_t temp = intra_cpb(ti, q, i) + inter_cpb_higher(ti, q, i) +
                      inter_cpb_lower(ti, q, i);
      if (temp > max_temp) max_temp = temp;
    }
    total_cpb += max_temp;
  }
  return total_cpb;
}

uint64_t CAB_DAG_FED_SPIN_PRIO::instance_number(const DAG_Task& ti,
                                                uint64_t interval) {
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  uint32_t MPD = min(processors.get_processor_num(),
                     ti.get_parallel_degree());  // Minimum Parallel Degree
  return ceiling(interval + r - e / MPD, p);
}

bool CAB_DAG_FED_SPIN_PRIO::is_schedulable() {
  uint32_t p_num = processors.get_processor_num();
  foreach(tasks.get_tasks(), task) {
    uint64_t C = task->get_wcet();
    uint64_t L = task->get_critical_path_length();
    uint64_t D = task->get_deadline();
    if (D == L) return false;
    uint32_t n;
    n = ceiling(C - L, D - L);
    task->set_dcores(n);
  }

  while (true) {
    uint32_t sum = 0;
    bool update = false;
    foreach(tasks.get_tasks(), task) {
      uint32_t n = task->get_dcores();
      uint64_t B_C = total_wb((*task));
      uint64_t B_L = total_cpb((*task));
      uint64_t C = task->get_wcet();
      uint64_t L = task->get_critical_path_length();
      uint64_t D = task->get_deadline();
      if ((L + B_L) >= D) return false;
      uint32_t n_minute = ceiling((C + B_C - L - B_L), (D - L - B_L));
      if (n_minute > n) {
        task->set_dcores(n_minute);
        update = true;
      }
      sum += n_minute;
    }
    if (sum > p_num) return false;
    if (!update) {

      return true;
    }
  }
}
