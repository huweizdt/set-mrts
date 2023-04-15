// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <rta_dag_fed_omip.h>
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

RTA_DAG_FED_OMIP::RTA_DAG_FED_OMIP()
    : PartitionedSched(false, RTA, FIX_PRIORITY, OTHER, "", "") {}

RTA_DAG_FED_OMIP::RTA_DAG_FED_OMIP(DAG_TaskSet tasks,
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

RTA_DAG_FED_OMIP::~RTA_DAG_FED_OMIP() {}

bool RTA_DAG_FED_OMIP::alloc_schedulable() {}

// Algorithm 1 Scheduling Test
bool RTA_DAG_FED_OMIP::is_schedulable() {
  uint32_t p_num = processors.get_processor_num();
  foreach(tasks.get_tasks(), ti) {
    uint64_t C_i = ti->get_wcet();
    uint64_t L_i = ti->get_critical_path_length();
    uint64_t D_i = ti->get_deadline();
    uint32_t initial_m_i = ceiling(C_i - L_i, D_i - L_i);
    ti->set_dcores(initial_m_i);
  }

  bool success = false;

  while (!success) {
    success = true;
    foreach(tasks.get_tasks(), ti) {
      uint64_t D_i = ti->get_deadline();
      uint64_t response_time = get_response_time((*ti));
      if (response_time > D_i) {
        ti->set_dcores(1 + ti->get_dcores());
        success = false;
      }
    }

    uint32_t sum_core = 0;
    foreach(tasks.get_tasks(), ti) {
      sum_core += ti->get_dcores();
    }
    if (sum_core > p_num)
      return false;
    if (success)
      return true;
  }
}

// Lemma 5 intra-task S-Idle time
uint64_t RTA_DAG_FED_OMIP::intra_task_blocking(const DAG_Task& ti,
                                                  uint32_t res_id, uint32_t request_num) {
  uint32_t m_i, N_i_q, alpha;
  uint64_t L_i_q;
  if (!ti.is_request_exist(res_id))
    return 0;
  m_i = ti.get_dcores();
  N_i_q = ti.get_request_by_id(res_id).get_num_requests();
  L_i_q = ti.get_request_by_id(res_id).get_max_length();
  alpha = min(m_i, N_i_q);
  if (0 == request_num) {
    return 0;
  } else {
    return L_i_q * (N_i_q-request_num) * (m_i-1);
  }
}

// Lemma 7 request number of inter-task S-Idle time
uint32_t RTA_DAG_FED_OMIP::maximum_request_number(const DAG_Task& ti,
                                                        const DAG_Task& tj,
                                                        uint32_t res_id) {
  if (!tj.is_request_exist(res_id))
    return 0;
  uint64_t d_i = ti.get_deadline();
  uint64_t d_j = tj.get_deadline();
  uint64_t p_j = tj.get_period();
  return ceiling(d_i + d_j, p_j) * tj.get_request_by_id(res_id).get_num_requests();
}

// Lemma 8 inter-task S-Idle time
uint64_t RTA_DAG_FED_OMIP::inter_task_blocking(const DAG_Task& ti, uint32_t res_id, uint32_t request_num) {
  uint32_t N_i_q, m_i;
  uint64_t L_i_q, sum = 0;
  if (!ti.is_request_exist(res_id))
    return 0;
  m_i = ti.get_dcores();
  N_i_q = ti.get_request_by_id(res_id).get_num_requests();
  L_i_q = ti.get_request_by_id(res_id).get_max_length();

  foreach_task_except(tasks.get_tasks(), ti, tj) {
    uint32_t m_j, mrn, delta;
    uint64_t L_j_q;
    if (!tj->is_request_exist(res_id))
      continue;
    m_j = tj->get_dcores();
    mrn = maximum_request_number(ti, (*tj), res_id);
    delta = min((uint32_t)1, request_num) * min(mrn, N_i_q);
    L_j_q = tj->get_request_by_id(res_id).get_max_length();
    sum += L_j_q * (mrn + (m_i-1)*delta);
  }
  return sum;
}

// Lemma 9 total blocking time
uint64_t RTA_DAG_FED_OMIP::total_blocking_time(const DAG_Task& ti) {
  uint64_t sum = 0;
  foreach(ti.get_requests(), r_i_q) {
    uint res_id = r_i_q->get_resource_id();
    uint64_t max = 0;
    for (uint request_num = 0; request_num <= r_i_q->get_num_requests(); request_num++) {
      uint64_t temp = intra_task_blocking(ti, res_id, request_num) + inter_task_blocking(ti, res_id, request_num);
      if (max < temp)
        max = temp;
    }
    sum += max;
  }
  return sum;
}

uint64_t RTA_DAG_FED_OMIP::get_response_time(const DAG_Task& ti) {
  uint32_t m_i = ti.get_dcores();
  uint64_t response_time =
      (ti.get_wcet() + (m_i - 1) * ti.get_critical_path_length() +
       total_blocking_time(ti)) /
      m_i;
  return response_time;
}
