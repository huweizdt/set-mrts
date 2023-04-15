// Copyright [2018] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <cab_dag_fed_spin_fifo.h>
#include <iteration-helper.h>
#include <lp.h>
#include <math-helper.h>
#include <processors.h>
#include <resources.h>
#include <solution.h>
#include <tasks.h>
#include <iostream>
#include <fstream>
#include <sstream>

using std::max;
using std::min;
using std::ostringstream;
using std::ofstream;

CAB_DAG_FED_SPIN_FIFO::CAB_DAG_FED_SPIN_FIFO()
    : PartitionedSched(false, CAB, FIX_PRIORITY, SPIN, "", "") {}

CAB_DAG_FED_SPIN_FIFO::CAB_DAG_FED_SPIN_FIFO(DAG_TaskSet tasks,
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

CAB_DAG_FED_SPIN_FIFO::~CAB_DAG_FED_SPIN_FIFO() {}

bool CAB_DAG_FED_SPIN_FIFO::alloc_schedulable() {}

uint64_t CAB_DAG_FED_SPIN_FIFO::intra_wb(const DAG_Task& ti,
                                         uint32_t resource_id) {
  int32_t n_i = ti.get_dcores();
  const Request& request_q = ti.get_request_by_id(resource_id);
  int32_t N_i_q = request_q.get_num_requests();
  uint64_t L_i_q = request_q.get_max_length();

  uint32_t m = min(N_i_q, n_i);

  return ((m * (m - 1) / 2) + (n_i - 1) * max((N_i_q - n_i), 0)) * L_i_q;
}

uint64_t CAB_DAG_FED_SPIN_FIFO::intra_cpb(const DAG_Task& ti,
                                          uint32_t resource_id,
                                          uint32_t request_num) {
  uint32_t n_i = ti.get_dcores();
  const Request& request_q = ti.get_request_by_id(resource_id);
  uint32_t N_i_q = request_q.get_num_requests();
  uint64_t L_i_q = request_q.get_max_length();
  return min((n_i - 1) * request_num, N_i_q - request_num) * L_i_q;
}

uint64_t CAB_DAG_FED_SPIN_FIFO::inter_wb_fifo(const DAG_Task& ti,
                                              const DAG_Task& tj,
                                              uint32_t resource_id) {
  uint32_t n_i = ti.get_dcores();
  uint32_t n_j = tj.get_dcores();
  const Request& request_i_q = ti.get_request_by_id(resource_id);
  uint32_t N_i_q = request_i_q.get_num_requests();
  const Request& request_j_q = tj.get_request_by_id(resource_id);
  uint32_t N_j_q = request_j_q.get_num_requests();
  uint32_t L_j_q = request_j_q.get_max_length();
  uint64_t d_i = ti.get_deadline();

  return min((uint64_t)(N_i_q * n_j), instance_number(tj, d_i) * N_j_q * n_i) *
         L_j_q;
}

uint64_t CAB_DAG_FED_SPIN_FIFO::inter_cpb_fifo(const DAG_Task& ti,
                                               uint32_t resource_id,
                                               uint32_t request_num) {
  uint64_t sum = 0;
  uint64_t d_i = ti.get_deadline();
  foreach_task_except(tasks.get_tasks(), ti, tx) {
    if (!tx->is_request_exist(resource_id)) continue;

    uint32_t n_x = tx->get_dcores();
    const Request& request_x_q = tx->get_request_by_id(resource_id);
    uint32_t N_x_q = request_x_q.get_num_requests();
    uint32_t L_x_q = request_x_q.get_max_length();
    sum += min((uint64_t)(n_x * request_num),
               instance_number((*tx), d_i) * N_x_q) *
           L_x_q;
  }
  return sum;
}

// uint64_t CAB_DAG_FED_SPIN_FIFO::total_wb(const DAG_Task& ti) {
//   uint64_t total_b = 0;
//   foreach(ti.get_requests(), request) {
//     uint32_t q = request->get_resource_id();
//     total_b += intra_wb(ti, q);
//     foreach_task_except(tasks.get_tasks(), ti, tx) {
//       if (tx->is_request_exist(q)) total_b += inter_wb_fifo(ti, (*tx), q);
//     }
//   }
//   return total_b;
// }


uint64_t CAB_DAG_FED_SPIN_FIFO::total_wb(const DAG_Task& ti) {
  uint64_t intra_b = 0;
  foreach(ti.get_requests(), request) {
    uint32_t q = request->get_resource_id();
    intra_b += intra_wb(ti, q);
  }

  uint64_t inter_b = 0;
  foreach(ti.get_requests(), request) {
    uint32_t q = request->get_resource_id();
    foreach_task_except(tasks.get_tasks(), ti, tx) {
      if (tx->is_request_exist(q)) inter_b += inter_wb_fifo(ti, (*tx), q);
    }
  }
  return intra_b+inter_b;
}

uint64_t CAB_DAG_FED_SPIN_FIFO::total_cpb(const DAG_Task& ti) {
  uint64_t total_cpb = 0;
  foreach(ti.get_requests(), request) {
    uint32_t q = request->get_resource_id();
    uint32_t N_i_q = request->get_num_requests();
    uint64_t max_temp = 0;
    for (uint i = 1; i <= N_i_q; i++) {
      uint64_t temp = intra_cpb(ti, q, i) + inter_cpb_fifo(ti, q, i);
      if (temp > max_temp) max_temp = temp;
    }
    total_cpb += max_temp;
  }
  return total_cpb;
}

uint64_t CAB_DAG_FED_SPIN_FIFO::instance_number(const DAG_Task& ti,
                                                uint64_t interval) {
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  return ceiling(interval + r, p);
}

bool CAB_DAG_FED_SPIN_FIFO::is_schedulable() {
  uint32_t p_num = processors.get_processor_num();
  foreach(tasks.get_tasks(), task) {
    uint64_t C = task->get_wcet();
    uint64_t L = task->get_critical_path_length();
    uint64_t D = task->get_deadline();
    if (D == L) {
      // uint64_t b_sum = 0;
      // foreach(tasks.get_tasks(), ti) {
      //   b_sum += total_wb((*ti));
      // }
      // ofstream spin("spin.log", ofstream::app);
      // spin << b_sum/ tasks.get_taskset_size() << "\n";
      // spin.flush();
      // spin.close();
      return false;
    }
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
      if ((L + B_L) >= D) {
        return false;
      }
      uint32_t n_minute = ceiling((C + B_C - L - B_L), (D - L - B_L));
      if (n_minute != n) {
        update = true;
      }
      sum += n_minute;
    }
    if (sum > p_num) {
      return false;
    }
    if (!update) {
      return true;
    }
    foreach(tasks.get_tasks(), ti) {
      uint32_t n = ti->get_dcores();
      uint64_t B_C = total_wb((*ti));
      uint64_t B_L = total_cpb((*ti));
      uint64_t C = ti->get_wcet();
      uint64_t L = ti->get_critical_path_length();
      uint64_t D = ti->get_deadline();
      uint32_t n_minute = ceiling((C + B_C - L - B_L), (D - L - B_L));
      if (n_minute > n) {
        ti->set_dcores(n_minute);
      }
    }
  }
}
