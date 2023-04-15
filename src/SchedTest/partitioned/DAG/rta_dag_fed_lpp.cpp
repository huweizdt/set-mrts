// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <rta_dag_fed_lpp.h>
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

RTA_DAG_FED_LPP::RTA_DAG_FED_LPP()
    : PartitionedSched(false, RTA, FIX_PRIORITY, FMLP, "", "") {}

RTA_DAG_FED_LPP::RTA_DAG_FED_LPP(DAG_TaskSet tasks,
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

RTA_DAG_FED_LPP::~RTA_DAG_FED_LPP() {}

bool RTA_DAG_FED_LPP::alloc_schedulable() {}

// Algorithm 1 Scheduling Test
bool RTA_DAG_FED_LPP::is_schedulable() {
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

// Lemma 5 intra-task S-Idle time
uint64_t RTA_DAG_FED_LPP::intra_task_blocking(const DAG_Task& ti, uint32_t res_id, uint request_num) {
  if (!ti.is_request_exist(res_id))
    return 0;
  const Request& r_q = ti.get_request_by_id(res_id);
  uint32_t N_i_q = r_q.get_num_requests();
  uint64_t L_i_q = r_q.get_max_length();
  uint32_t m_i = ti.get_dcores();
  return min((uint32_t)1, request_num) * (m_i - 1) * (N_i_q - 1) * L_i_q;
}

uint32_t RTA_DAG_FED_LPP::maximum_request_number(const DAG_Task& ti,
                                                        const DAG_Task& tj,
                                                        uint32_t res_id) {
  if (!tj.is_request_exist(res_id))
    return 0;
  uint64_t d_i = ti.get_deadline();
  uint64_t d_j = tj.get_deadline();
  uint64_t p_j = tj.get_period();
  return ceiling(d_i + d_j, p_j) * tj.get_request_by_id(res_id).get_num_requests();
}

uint32_t RTA_DAG_FED_LPP::inter_task_request_number(const DAG_Task& ti,
                                                        const DAG_Task& tj,
                                                        uint32_t res_id,
                                                        uint32_t request_num) {
  if (ti.is_request_exist(res_id) && tj.is_request_exist(res_id)) {
    uint64_t d_i = ti.get_deadline();
    uint64_t d_j = tj.get_deadline();
    uint64_t p_j = tj.get_period();
    const Request& r_i_q = ti.get_request_by_id(res_id);
    uint32_t N_i_q = r_i_q.get_num_requests();
    const Request& r_j_q = tj.get_request_by_id(res_id);
    uint32_t N_j_q = r_j_q.get_num_requests();
    uint32_t mrn = maximum_request_number(ti, tj, res_id);
    return min((uint32_t)1, request_num) * min(mrn, N_i_q);
  } else {
    return 0;
  }
}

// Lemma 8 inter-task S-Idle time
uint64_t RTA_DAG_FED_LPP::inter_task_blocking(const DAG_Task& ti, uint32_t res_id, uint request_num) {
    uint64_t d_i = ti.get_deadline();
    uint32_t m_i = ti.get_dcores();
    uint64_t sum = 0;
    foreach_task_except(tasks.get_tasks(), ti, tj) {
      uint32_t m_j, mrn;
      uint64_t L_j_q;
      if (!tj->is_request_exist(res_id))
        continue;
      m_j = ti.get_dcores();
      mrn = maximum_request_number(ti, (*tj), res_id);
      L_j_q = tj->get_request_by_id(res_id).get_max_length();
      sum += ((mrn+(m_j-1)*inter_task_request_number(ti, (*tj), res_id, request_num))*L_j_q)/m_j;
    }
    return sum * m_i;
}

// uint64_t RTA_DAG_FED_LPP::inter_task_blocking(const DAG_Task& ti, uint32_t res_id, uint request_num) {
//     uint64_t d_i = ti.get_deadline();
//     uint32_t m_i = ti.get_dcores();
//     uint64_t sum = 0;
//     foreach_task_except(tasks.get_tasks(), ti, tj) {
//       uint64_t d_j = tj->get_deadline();
//       uint64_t p_j = tj->get_period();
//       uint64_t delta_j = 0;
//       uint64_t S_O_J = 0;
//       uint32_t m_j = tj->get_dcores();

//       foreach(tj->get_requests(), r_j_q) {
//         uint32_t N_j_q = r_j_q->get_num_requests();
//         uint64_t L_j_q = r_j_q->get_max_length();
//         uint32_t maximal_request_number = ceiling(d_i + d_j, p_j) * N_j_q;
//         if (!ti.is_request_exist(r_j_q->get_resource_id())) {
//           delta_j += maximal_request_number * L_j_q;
//         } else {
//           S_O_J += (maximal_request_number +
//                     (m_j - 1) * inter_task_request_number(
//                                     ti, (*tj), r_j_q->get_resource_id())) *
//                    L_j_q;
//         }
//       }
//       sum += (delta_j + S_O_J) / m_j;
//     }
//     return sum * m_i;
// }

// Lemma 9 total blocking time
uint64_t RTA_DAG_FED_LPP::total_blocking_time(const DAG_Task& ti) {
  uint32_t m_i = ti.get_dcores();
  uint64_t sum = 0;

  foreach_task_except(tasks.get_tasks(), ti, tj) {
    uint32_t m_j = tj->get_dcores();
    foreach(tj->get_requests(), request) {
      uint32_t q = request->get_resource_id();
      uint64_t L_j_q = request->get_max_length();
      if (ti.is_request_exist(q))
        continue;

      sum += m_i * (maximum_request_number(ti, (*tj), q) * L_j_q)/m_j;
    }
  }

  foreach(ti.get_requests(), request) {
    uint32_t q = request->get_resource_id();
    sum += intra_task_blocking(ti, q) + inter_task_blocking(ti, q);
  }
  return sum;
}

uint64_t RTA_DAG_FED_LPP::get_response_time(const DAG_Task& ti) {
  uint32_t m_i = ti.get_dcores();
  cout << "[LPP] Task[" << ti.get_id() << "] total blocking:" <<  total_blocking_time(ti) << endl;
  uint64_t response_time =
      (ti.get_wcet() + (m_i - 1) * ti.get_critical_path_length() +
       total_blocking_time(ti)) /
      m_i;
  return response_time;
}
