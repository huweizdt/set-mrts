// Copyright [2018] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <lp.h>
#include <math-helper.h>
#include <processors.h>
#include <resources.h>
#include <rta_dag_cgfp_fmlp.h>
#include <solution.h>
#include <tasks.h>
#include <iostream>
#include <sstream>
#include <exception>

using std::min;
using std::exception;
using std::ostringstream;

RTA_DAG_CGFP_FMLP::RTA_DAG_CGFP_FMLP()
    : GlobalSched(false, RTA, FIX_PRIORITY, FMLP, "", "") {}

RTA_DAG_CGFP_FMLP::RTA_DAG_CGFP_FMLP(DAG_TaskSet tasks, ProcessorSet processors,
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

RTA_DAG_CGFP_FMLP::~RTA_DAG_CGFP_FMLP() {}

uint32_t RTA_DAG_CGFP_FMLP::instance_number(const DAG_Task& ti,
                                            uint64_t interval) {
  // uint64_t cpl = ti.get_critical_path_length();
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  uint32_t MPD = min(ti.get_parallel_degree(),
                     ti.get_dcores());  // Minimum Parallel Degree
  return ceiling(interval + r - e/MPD, p);
}

uint64_t RTA_DAG_CGFP_FMLP::total_workload(const DAG_Task& ti,
                                           uint64_t interval) {
  // uint64_t cpl = ti.get_critical_path_length();
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  uint32_t MPD = min(ti.get_parallel_degree(),
                     ti.get_dcores());  // Minimum Parallel Degree
  uint64_t workload = ceiling(interval + r - e/MPD, p) * e;
  return workload;
}

uint64_t RTA_DAG_CGFP_FMLP::CS_workload(const DAG_Task& ti,
                                        uint32_t resource_id,
                                        uint64_t interval) {
  // uint64_t cpl = ti.get_critical_path_length();
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  const Request& request = ti.get_request_by_id(resource_id);
  uint64_t agent_length = request.get_num_requests() * request.get_max_length();
  uint32_t MPD = min(ti.get_parallel_degree(),
                     ti.get_dcores());  // Minimum Parallel Degree
  return ceiling((interval + r - agent_length), p) * agent_length;
}

uint64_t RTA_DAG_CGFP_FMLP::intra_blocking(const DAG_Task& ti,
                                          uint32_t resource_id,
                                          uint32_t request_num) {
  const Request& request = ti.get_request_by_id(resource_id);
  uint32_t N_i_q = request.get_num_requests();
  uint64_t L_i_q = request.get_max_length();
  uint32_t X_i_q = request_num;
  uint32_t Y_i_q = request.get_num_requests() - X_i_q;
  // uint32_t MPD = min(ti.get_parallel_degree(),
  //                    ti.get_dcores());  // Minimum Parallel Degree
  uint32_t MPD = ti.get_parallel_degree();
  return min((MPD - 1) * X_i_q, Y_i_q) * L_i_q;
}

uint64_t RTA_DAG_CGFP_FMLP::inter_blocking(uint64_t interval,
                                          const DAG_Task& ti,
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
      uint32_t MPD = min(processors.get_processor_num(),
                         min(ti.get_parallel_degree(),
                             ti.get_dcores()));  // Minimum Parallel Degree
      // sum += min(MPD, N_x_q) * X_i_q * L_x_q;
      // uint64_t temp = min(MPD, N_x_q);
      uint64_t temp = min(tx->get_parallel_degree(), N_x_q);
      uint64_t ceiling_n = instance_number((*tx), interval) * N_x_q;
      sum += min(temp * X_i_q, ceiling_n) * L_x_q;

      // cout << min(MPD, N_x_q) * X_i_q * L_x_q << " "
      //      << min(temp * X_i_q, ceiling_n) * L_x_q << endl;
    }
  }
  return sum;
}

uint64_t RTA_DAG_CGFP_FMLP::intra_interference(const DAG_Task& ti) {
  return ti.get_wcet() - ti.get_critical_path_length();
}

uint64_t RTA_DAG_CGFP_FMLP::inter_interference(const DAG_Task& ti,
                                               uint64_t interval) {
  vector<Interf> Q;
  uint p_num = processors.get_processor_num();
  try {
    foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
      uint64_t tw = total_workload((*th), interval);
      Interf temp;
      temp.t_id = th->get_id();
      temp.workload = tw;
      temp.parallel_degree = th->get_dcores();
      Q.push_back(temp);
    }
  } catch (exception& e) {
    cout << e.what() << endl;
  }

  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    foreach(tl->get_requests(), l_u) {
      uint32_t r_id = l_u->get_resource_id();
      Resource& resource = resources.get_resource_by_id(r_id);
      if (resource.get_ceiling() < ti.get_priority()) {
        uint64_t csw = CS_workload((*tl), r_id, interval);
        Interf temp;
        temp.t_id = tl->get_id();
        temp.workload = csw;
        temp.parallel_degree = 1;
        Q.push_back(temp);
      }
    }
  }
  uint64_t inter_interf = interf_cal(Q, ti, p_num);

  return inter_interf;
}

uint64_t RTA_DAG_CGFP_FMLP::response_time(const DAG_Task& ti) {
  uint32_t t_id = ti.get_id();
  uint64_t CPL = ti.get_critical_path_length();
  uint64_t intra_interf = intra_interference(ti);
  uint32_t parallel_degree = ti.get_dcores();
  uint64_t total_blocking = 0;
  uint p_num = processors.get_processor_num();
  uint32_t MPD = min(p_num, min(ti.get_parallel_degree(),
                                ti.get_dcores()));  // Minimum Parallel Degree

  uint64_t test_end = ti.get_deadline();
  uint64_t test_start = CPL + intra_interf / ti.get_dcores();

  uint64_t response = test_start;
  uint64_t demand = 0;

  while (response <= test_end) {
    uint64_t inter_interf = inter_interference(ti, response);

    total_blocking = 0;
    foreach(ti.get_requests(), request) {
      uint32_t r_id = request->get_resource_id();
      uint64_t blocking = 0;
      uint32_t N_i_q = request->get_num_requests();
      for (uint32_t x = 1; x <= N_i_q; x++) {
        uint64_t temp =
            intra_blocking(ti, r_id, x) + inter_blocking(response, ti, r_id, x);
        if (temp > blocking)
          blocking = temp;
      }
      total_blocking += blocking;
    }
    demand = test_start + inter_interf + total_blocking;
    // demand = test_start + inter_interf;

    if (response == demand) {
      return response;
    } else {
      response = demand;
    }
  }
  return test_end + 100;
}

int RTA_DAG_CGFP_FMLP::w_decrease(Interf t1, Interf t2) {
  return ceiling(t1.workload, t1.parallel_degree) >
         ceiling(t2.workload, t2.parallel_degree);
}

uint64_t RTA_DAG_CGFP_FMLP::interf_cal(vector<Interf> Q, const DAG_Task& ti,
                                       uint p_num) {
  uint32_t p_num_left = p_num;
  sort(Q.begin(), Q.end(), w_decrease);
  uint64_t I = 0;
  foreach(Q, object) {
    I += object->workload;
  }

  foreach(Q, object) {
    if (ti.get_id() == object->t_id)
      continue;
    uint64_t temp_1 = ceiling(object->workload, object->parallel_degree);
    uint64_t temp_2 = ceiling(I, p_num_left);
    if (temp_1 > temp_2) {
      if ((p_num_left - object->parallel_degree) >= ti.get_dcores()) {
        I -= object->workload;
        p_num_left -= object->parallel_degree;
      } else {
        I -= (object->workload / object->parallel_degree) *
             (p_num_left - ti.get_dcores());
        p_num_left = ti.get_dcores();
        break;
      }
    } else {
      return ceiling(I, p_num_left);
    }
  }

  return ceiling(I, p_num_left);
}

bool RTA_DAG_CGFP_FMLP::is_schedulable() {
  uint p_num = processors.get_processor_num();
  uint64_t response_bound;

  foreach(tasks.get_tasks(), ti) {
    bool schedulable = false;
    uint32_t initial = ceiling(ti->get_wcet(), ti->get_response_time());
    for (uint n_minute = initial; n_minute <= p_num; n_minute++) {
      ti->set_dcores(n_minute);
      response_bound = response_time((*ti));
      if (response_bound <= ti->get_deadline()) {
        ti->set_response_time(response_bound);
        schedulable = true;
        break;
      }
    }
    if (!schedulable) {
      return false;
    }
  }
  return true;
}

// ------------------------------V2-----------------------------------

RTA_DAG_CGFP_FMLP_v2::RTA_DAG_CGFP_FMLP_v2() : RTA_DAG_CGFP_FMLP() {}

RTA_DAG_CGFP_FMLP_v2::RTA_DAG_CGFP_FMLP_v2(DAG_TaskSet tasks,
                                           ProcessorSet processors,
                                           ResourceSet resources)
    : RTA_DAG_CGFP_FMLP(tasks, processors, resources) {}

RTA_DAG_CGFP_FMLP_v2::~RTA_DAG_CGFP_FMLP_v2() {}

uint64_t RTA_DAG_CGFP_FMLP_v2::inter_blocking(uint64_t interval,
                                              const DAG_Task& ti,
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
      uint32_t MPD = min(processors.get_processor_num(),
                         min(ti.get_parallel_degree(),
                             ti.get_dcores()));  // Minimum Parallel Degree
      // sum += min(MPD, N_x_q) * X_i_q * L_x_q;
      // uint64_t temp = min(MPD, N_x_q);
      uint64_t temp = min(ti.get_parallel_degree(), N_x_q);
      uint64_t ceiling_n = instance_number((*tx), interval) * N_x_q;
      sum += min(temp * X_i_q, ceiling_n) * L_x_q;

      // cout << min(MPD, N_x_q) * X_i_q * L_x_q << " "
      //      << min(temp * X_i_q, ceiling_n) * L_x_q << endl;
    }
  }
  return sum;
}

uint64_t RTA_DAG_CGFP_FMLP_v2::inter_interference(const DAG_Task& ti,
                                                  uint64_t interval) {
  vector<Interf> Q;
  uint p_num = processors.get_processor_num();
  try {
    foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
      uint64_t tw = total_workload((*th), interval);
      Interf temp;
      temp.t_id = th->get_id();
      temp.workload = tw;
      temp.parallel_degree = th->get_dcores();
      Q.push_back(temp);
    }
  } catch (exception& e) {
    cout << e.what() << endl;
  }

  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    foreach(tl->get_requests(), l_u) {
      uint32_t r_id = l_u->get_resource_id();
      Resource& resource = resources.get_resource_by_id(r_id);
      if (resource.get_ceiling() < ti.get_priority()) {
        uint64_t csw = CS_workload((*tl), r_id, interval);
        Interf temp;
        temp.t_id = tl->get_id();
        temp.workload = csw;
        temp.parallel_degree = 1;
        Q.push_back(temp);
      }
    }
  }

  uint64_t inter_interf = interf_cal(Q, ti, p_num);
  return inter_interf;
}


uint64_t RTA_DAG_CGFP_FMLP_v2::response_time(const DAG_Task& ti) {
  uint32_t t_id = ti.get_id();
  uint64_t CPL = ti.get_critical_path_length();
  uint64_t intra_interf = intra_interference(ti);
  uint32_t parallel_degree = ti.get_dcores();
  uint64_t total_blocking = 0;
  uint p_num = processors.get_processor_num();
  uint32_t MPD = min(p_num, min(ti.get_parallel_degree(),
                                ti.get_dcores()));  // Minimum Parallel Degree
  // foreach(ti.get_requests(), request) {
  //   uint32_t r_id = request->get_resource_id();
  //   uint64_t blocking = 0;
  //   uint32_t N_i_q = request->get_num_requests();
  //   for (uint32_t x = 1; x <= N_i_q; x++) {
  //     uint64_t temp =
  //         intra_blocking(ti, r_id, x) + inter_blocking(ti, r_id, x);
  //     if (temp > blocking)
  //       blocking = temp;
  //   }
  //   total_blocking += blocking;
  // }

  uint64_t test_end = ti.get_deadline();
  uint64_t test_start = CPL + intra_interf / ti.get_dcores();

  uint64_t response = test_start;
  uint64_t demand = 0;

  while (response <= test_end) {
    uint64_t inter_interf = inter_interference(ti, response);

    total_blocking = 0;
    foreach(ti.get_requests(), request) {
      uint32_t r_id = request->get_resource_id();
      uint64_t blocking = 0;
      uint32_t N_i_q = request->get_num_requests();
      for (uint32_t x = 1; x <= N_i_q; x++) {
        uint64_t temp =
            intra_blocking(ti, r_id, x) + inter_blocking(response, ti, r_id, x);
        if (temp > blocking)
          blocking = temp;
      }
      total_blocking += blocking;
    }
    demand = test_start + inter_interf + total_blocking;
    // demand = test_start + inter_interf;

    if (response == demand) {
      return response;
    } else {
      response = demand;
    }
  }
  return test_end + 100;
}

uint64_t RTA_DAG_CGFP_FMLP_v2::interf_cal(vector<Interf> Q, const DAG_Task& ti,
                                          uint p_num) {
  uint32_t p_num_left = p_num;
  sort(Q.begin(), Q.end(), w_decrease);
  uint64_t I = 0;
  foreach(Q, object) {
    I += object->workload;
  }

  foreach(Q, object) {
    if (ti.get_id() == object->t_id) continue;
    uint64_t temp_1 = ceiling(object->workload, object->parallel_degree);
    uint64_t temp_2 = ceiling(I, p_num_left);
    if ((temp_1 >= temp_2) &&
        ((p_num_left - object->parallel_degree) >= ti.get_dcores())) {
      I -= object->workload;
      p_num_left -= object->parallel_degree;
    } else {
      return ceiling(I, p_num_left);
    }
  }

  return ceiling(I, p_num_left);
}

bool RTA_DAG_CGFP_FMLP_v2::is_schedulable() {
  uint p_num = processors.get_processor_num();
  uint64_t response_bound;

  foreach(tasks.get_tasks(), ti) {
    bool schedulable = false;
    uint32_t initial = ceiling(ti->get_wcet(), ti->get_response_time());
    for (uint n_minute = initial; n_minute <= p_num; n_minute++) {
      ti->set_dcores(n_minute);
      response_bound = response_time((*ti));

      if (response_bound <= ti->get_deadline()) {
        ti->set_response_time(response_bound);
        schedulable = true;
        break;
      }
    }
    if (!schedulable) {
      return false;
    }
  }
  return true;
}
