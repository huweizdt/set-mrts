// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <rta_dag_fed_spin_xu.h>
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

RTA_DAG_FED_SPIN_FIFO_XU::RTA_DAG_FED_SPIN_FIFO_XU()
    : PartitionedSched(false, RTA, FIX_PRIORITY, OTHER, "", "") {}

RTA_DAG_FED_SPIN_FIFO_XU::RTA_DAG_FED_SPIN_FIFO_XU(DAG_TaskSet tasks,
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

RTA_DAG_FED_SPIN_FIFO_XU::~RTA_DAG_FED_SPIN_FIFO_XU() {}

bool RTA_DAG_FED_SPIN_FIFO_XU::alloc_schedulable() {}

// Algorithm 1 Scheduling Test
bool RTA_DAG_FED_SPIN_FIFO_XU::is_schedulable() {
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

// Lemma 6 intra-task I^I time
uint64_t RTA_DAG_FED_SPIN_FIFO_XU::intra_task_blocking(const DAG_Task& ti,
                                                  uint32_t res_id, uint32_t request_num) {
  uint32_t m_i, N_i_q, delta;
  uint64_t L_i_q;
  if (!ti.is_request_exist(res_id))
    return 0;
  m_i = ti.get_dcores();
  N_i_q = ti.get_request_by_id(res_id).get_num_requests();
  L_i_q = ti.get_request_by_id(res_id).get_max_length();

  delta = min(m_i, N_i_q) * (m_i - (min(m_i, N_i_q)+1)/2);

  if (0 == request_num) {
    return L_i_q * ((N_i_q-request_num) * (m_i-1) - delta);
  } else {
    return L_i_q * (N_i_q-request_num) * (m_i-1);
  }
}

// 
uint32_t RTA_DAG_FED_SPIN_FIFO_XU::maximum_request_number(const DAG_Task& ti,
                                                        const DAG_Task& tj,
                                                        uint32_t res_id) {
  if (!tj.is_request_exist(res_id))
    return 0;
  uint64_t d_i = ti.get_deadline();
  uint64_t d_j = tj.get_deadline();
  uint64_t p_j = tj.get_period();
  return ceiling(d_i + d_j, p_j) * tj.get_request_by_id(res_id).get_num_requests();
}

// Lemma 7 inter-task I^O time
uint64_t RTA_DAG_FED_SPIN_FIFO_XU::inter_task_blocking(const DAG_Task& ti, uint32_t res_id, uint32_t request_num) {
  uint32_t N_i_q, m_i;
  uint64_t L_i_q, sum = 0;
  if (!ti.is_request_exist(res_id))
    return 0;
  m_i = ti.get_dcores();
  N_i_q = ti.get_request_by_id(res_id).get_num_requests();
  L_i_q = ti.get_request_by_id(res_id).get_max_length();

  foreach_task_except(tasks.get_tasks(), ti, tj) {
    uint32_t m_j;
    uint64_t L_j_q;
    if (!tj->is_request_exist(res_id))
      continue;
    m_j = tj->get_dcores();
    L_j_q = tj->get_request_by_id(res_id).get_max_length();
    sum += L_j_q * min((m_i * maximum_request_number(ti, (*tj), res_id)), m_j *(N_i_q + (m_i-1)*request_num));
    // sum += L_j_q *  m_j *(N_i_q + (m_i-1)*request_num);
  }
  // cout << "inter_task_blocking:" << sum << endl;
  // return 0;
  return sum;
}

// Lemma 8 total blocking time
uint64_t RTA_DAG_FED_SPIN_FIFO_XU::total_blocking_time(const DAG_Task& ti) {
  uint64_t sum = 0;
  uint32_t max_delay_request_num;
  foreach(ti.get_requests(), r_i_q) {
    uint res_id = r_i_q->get_resource_id();
    uint64_t max = 0;
    for (uint request_num = 0; request_num <= r_i_q->get_num_requests(); request_num++) {
      uint64_t temp = intra_task_blocking(ti, res_id, request_num) + inter_task_blocking(ti, res_id, request_num);
      // cout << "[SPIN-Xu]x="<< request_num << ":"<< temp << endl;
      if (max < temp) {
        max = temp;
        max_delay_request_num = request_num;
      }
    }
    // cout << "[SPIN-Xu] X_"<<ti.get_id()<<"_"<<res_id<<":"<< max_delay_request_num << endl;
    sum += max;
  }
  return sum;
}

// Theorem 2
uint64_t RTA_DAG_FED_SPIN_FIFO_XU::get_response_time(const DAG_Task& ti) {
  uint32_t m_i = ti.get_dcores();
  uint64_t B_i = total_blocking_time(ti);
  // cout << "[SPIN-Xu] Task[" << ti.get_id() << "] with "<<m_i <<" processors. total blocking:" <<  B_i/m_i << endl;
  uint64_t response_time =
      (ti.get_wcet() + (m_i - 1) * ti.get_critical_path_length() +
       B_i) /
      m_i;
  return response_time;
}






/** Class RTA_DAG_FED_SPIN_PRIO_XU */


RTA_DAG_FED_SPIN_PRIO_XU::RTA_DAG_FED_SPIN_PRIO_XU()
    : PartitionedSched(false, RTA, FIX_PRIORITY, OTHER, "", "") {}

RTA_DAG_FED_SPIN_PRIO_XU::RTA_DAG_FED_SPIN_PRIO_XU(DAG_TaskSet tasks,
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

RTA_DAG_FED_SPIN_PRIO_XU::~RTA_DAG_FED_SPIN_PRIO_XU() {}

bool RTA_DAG_FED_SPIN_PRIO_XU::alloc_schedulable() {}

// Algorithm 1 Scheduling Test
bool RTA_DAG_FED_SPIN_PRIO_XU::is_schedulable() {
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

uint64_t RTA_DAG_FED_SPIN_PRIO_XU::intra_task_blocking(const DAG_Task& ti,
                                                  uint32_t res_id, uint32_t request_num) {
  uint32_t m_i, N_i_q, delta;
  uint64_t L_i_q;
  if (!ti.is_request_exist(res_id))
    return 0;
  m_i = ti.get_dcores();
  N_i_q = ti.get_request_by_id(res_id).get_num_requests();
  L_i_q = ti.get_request_by_id(res_id).get_max_length();

  delta = min(m_i, N_i_q) * (m_i - (min(m_i, N_i_q)+1)/2);

  if (0 == request_num) {
    return L_i_q * ((N_i_q-request_num) * (m_i-1) - delta);
  } else {
    return L_i_q * (N_i_q-request_num) * (m_i-1);
  }
}

uint32_t RTA_DAG_FED_SPIN_PRIO_XU::maximum_request_number(const DAG_Task& ti,
                                                        const DAG_Task& tj,
                                                        uint32_t res_id) {
  if (!tj.is_request_exist(res_id))
    return 0;
  uint64_t d_i = ti.get_deadline();
  uint64_t d_j = tj.get_deadline();
  uint64_t p_j = tj.get_period();
  return ceiling(d_i + d_j, p_j) * tj.get_request_by_id(res_id).get_num_requests();
}

uint64_t RTA_DAG_FED_SPIN_PRIO_XU::higher(const DAG_Task& ti, uint32_t res_id,
                                       uint64_t interval) {
  uint64_t sum = 0;
  foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
    if (th->is_request_exist(res_id)) {
      const Request& request_q = th->get_request_by_id(res_id);
      uint32_t n = th->get_max_job_num(interval);
      uint32_t N_h_q = request_q.get_num_requests();
      uint64_t L_h_q = request_q.get_max_length();
      sum += n * N_h_q * L_h_q;
    }
  }
  return sum;
}

uint64_t RTA_DAG_FED_SPIN_PRIO_XU::lower(const DAG_Task& ti,
                                      uint32_t res_id) {
  uint64_t max = 0;
  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    if (tl->is_request_exist(res_id)) {
      const Request& request_q = tl->get_request_by_id(res_id);
      uint64_t L_l_q = request_q.get_max_length();
      if (L_l_q > max) max = L_l_q;
    }
  }
  return max;
}

uint64_t RTA_DAG_FED_SPIN_PRIO_XU::equal(const DAG_Task& ti,
                                      uint32_t res_id) {
  if (ti.is_request_exist(res_id)) {
    const Request& request_q = ti.get_request_by_id(res_id);
    uint32_t N_i_q = request_q.get_num_requests();
    uint64_t L_i_q = request_q.get_max_length();
    uint32_t n_i = ti.get_dcores();
    uint32_t m_i = min(n_i - 1, N_i_q - 1);
    return m_i * L_i_q;
  } else {
    return 0;
  }
}

uint64_t RTA_DAG_FED_SPIN_PRIO_XU::dpr(const DAG_Task& ti, uint32_t res_id) {
  uint64_t test_start = lower(ti, res_id) + equal(ti, res_id);
  uint64_t test_end = ti.get_deadline();
  uint64_t delay = test_start;

  while (delay <= test_end) {
    uint64_t temp = test_start;

    temp += higher(ti, res_id, delay);

    if (temp > delay)
      delay = temp;
    else if (temp == delay)
      return delay;
  }
  return test_end + 100;
}

uint64_t RTA_DAG_FED_SPIN_PRIO_XU::inter_task_blocking(const DAG_Task& ti, uint32_t res_id, uint32_t request_num) {
  uint32_t N_i_q, m_i;
  uint64_t L_i_q, sum = 0;
  uint64_t max_lprl = 0; // lower-priority request length
  if (!ti.is_request_exist(res_id))
    return 0;
  m_i = ti.get_dcores();
  N_i_q = ti.get_request_by_id(res_id).get_num_requests();
  L_i_q = ti.get_request_by_id(res_id).get_max_length();


  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    if (!tl->is_request_exist(res_id))
      continue;
    uint64_t L_l_q = tl->get_request_by_id(res_id).get_max_length();
    if (max_lprl < L_l_q) {
      max_lprl = L_l_q;
    }
  }
  sum += (N_i_q + (m_i - 1) * request_num) * max_lprl;

  foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
    uint32_t m_h, N_h_q;
    uint64_t L_h_q;
    if (!th->is_request_exist(res_id))
      continue;
    m_h = th->get_dcores();
    L_h_q = th->get_request_by_id(res_id).get_max_length();
    N_h_q = th->get_request_by_id(res_id).get_num_requests();
    uint32_t delta = ceiling(dpr(ti, res_id)+th->get_deadline(), th->get_period());
    sum += L_h_q * min((m_i * maximum_request_number(ti, (*th), res_id)), m_h *(N_i_q + (m_i-1)*request_num)*delta*N_h_q);
  }
  // cout << "inter_task_blocking:" << sum << endl;
  // return 0;
  return sum;
}

uint64_t RTA_DAG_FED_SPIN_PRIO_XU::total_blocking_time(const DAG_Task& ti) {
  uint64_t sum = 0;
  uint32_t max_delay_request_num;
  foreach(ti.get_requests(), r_i_q) {
    uint res_id = r_i_q->get_resource_id();
    uint64_t max = 0;
    for (uint request_num = 0; request_num <= r_i_q->get_num_requests(); request_num++) {
      uint64_t temp = intra_task_blocking(ti, res_id, request_num) + inter_task_blocking(ti, res_id, request_num);
      // cout << "[SPIN-Xu]x="<< request_num << ":"<< temp << endl;
      if (max < temp) {
        max = temp;
        max_delay_request_num = request_num;
      }
    }
    // cout << "[SPIN-Xu] X_"<<ti.get_id()<<"_"<<res_id<<":"<< max_delay_request_num << endl;
    sum += max;
  }
  return sum;
}

uint64_t RTA_DAG_FED_SPIN_PRIO_XU::get_response_time(const DAG_Task& ti) {
  uint32_t m_i = ti.get_dcores();
  uint64_t B_i = total_blocking_time(ti);
  cout << "[SPIN-p-Xu] Task[" << ti.get_id() << "] with "<<m_i <<" processors. total blocking:" <<  B_i/m_i << endl;
  uint64_t response_time =
      (ti.get_wcet() + (m_i - 1) * ti.get_critical_path_length() +
       B_i) /
      m_i;
  return response_time;
}
