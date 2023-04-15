// Copyright [2018] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <lp.h>
#include <math-helper.h>
#include <processors.h>
#include <resources.h>
#include <rta_dag_cgfp_pip.h>
#include <solution.h>
#include <tasks.h>
#include <iostream>
#include <sstream>
#include <exception>

using std::min;
using std::exception;
using std::ostringstream;

RTA_DAG_CGFP_PIP::RTA_DAG_CGFP_PIP()
    : GlobalSched(false, RTA, FIX_PRIORITY, PIP, "", "") {}

RTA_DAG_CGFP_PIP::RTA_DAG_CGFP_PIP(DAG_TaskSet tasks, ProcessorSet processors,
                                   ResourceSet resources)
    : GlobalSched(false, RTA, FIX_PRIORITY, PIP, "", "") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;

  this->resources.update(&(this->tasks));
  this->processors.update(&(this->tasks), &(this->resources));

  this->tasks.init();
  this->tasks.RM_Order();
  this->processors.init();
}

RTA_DAG_CGFP_PIP::~RTA_DAG_CGFP_PIP() {}

uint64_t RTA_DAG_CGFP_PIP::resource_holding(const DAG_Task& ti,
                                           const DAG_Task& tx,
                                           uint32_t resource_id) {
  uint p_num = processors.get_processor_num();
  if (tx.is_request_exist(resource_id)) {
    uint32_t y = min(ti.get_priority(), tx.get_priority());
    const DAG_Task& ty = tasks.get_tasks()[y];

    uint64_t parallel_workload = 0;
    for (uint i = 0; i < ty.get_vnode_num(); i++) {
      uint64_t temp = 0;
      VNode v_i = ty.get_vnodes()[i];
      for (uint j = 0; j < ty.get_vnode_num(); j++) {
        VNode v_j = ty.get_vnodes()[j];
        if (j < i) {
          if (!ty.is_connected(v_j, v_i)) temp += v_j.wcet;
        }
      }
      if (temp > parallel_workload) parallel_workload = temp;
    }

    uint64_t L_x_q = tx.get_request_by_id(resource_id).get_max_length();
    uint64_t test_start = L_x_q;
    uint64_t test_end = tx.get_deadline();
    uint64_t holding_time = test_start;

    while (holding_time < test_end) {
      uint64_t demand = test_start;
      uint64_t interf_h = 0;
      foreach_higher_priority_task(tasks.get_tasks(), ty, th) {
        interf_h += total_workload((*th), holding_time);
      }

      uint interf_l = 0;
      foreach_lower_priority_task(tasks.get_tasks(), ty, tl) {
        foreach(tl->get_requests(), request_u) {
          if (resource_id == request_u->get_resource_id())
            continue;
          const Resource& resource =
              resources.get_resource_by_id(request_u->get_resource_id());
          if (ty.get_priority() > resource.get_ceiling()) {
            interf_l += instance_number((*tl), holding_time) *
                        request_u->get_num_requests() *
                        request_u->get_max_length();
          }
        }
      }

      demand += (interf_h + interf_l + parallel_workload) / p_num;
      assert(holding_time <= demand);

      if (holding_time == demand) {
        return holding_time;
      } else {
        holding_time = demand;
      }
    }
  return test_end + 100;
  } else {
    return 0;
  }
}

uint64_t RTA_DAG_CGFP_PIP::phi(const DAG_Task& ti, uint32_t resource_id) {
  uint64_t H_i_l_q = 0;
  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    uint64_t temp = resource_holding(ti, (*tl), resource_id);
    if (H_i_l_q < temp)
      H_i_l_q = temp;
  }

  uint64_t test_start = H_i_l_q, test_end = ti.get_deadline();
  uint64_t phi_i_q = test_start;

  while (phi_i_q < test_end) {
    uint64_t demand = test_start;
    uint64_t H_i_h_q = 0;
    foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
      if (!th->is_request_exist(resource_id))
        continue;
      const Request& request = th->get_request_by_id(resource_id);
      H_i_h_q += instance_number((*th), phi_i_q) * request.get_num_requests() *
                 resource_holding(ti, (*th), resource_id);
    }

    demand += H_i_h_q;

    if (phi_i_q == demand) {
      return phi_i_q;
    } else {
      phi_i_q = demand;
    }
  }
  return test_end + 100;
}

uint64_t RTA_DAG_CGFP_PIP::theta(const DAG_Task& ti, uint32_t resource_id) {
  uint p_num = processors.get_processor_num();
  uint64_t parallel_workload = 0;
  for (uint i = 0; i < ti.get_vnode_num(); i++) {
    uint64_t temp = 0;
    VNode v_i = ti.get_vnodes()[i];
    for (uint j = 0; j < ti.get_vnode_num(); j++) {
      VNode v_j = ti.get_vnodes()[j];
      if (j < i) {
        if (!ti.is_connected(v_j, v_i)) temp += v_j.wcet;
      }
    }
    if (temp > parallel_workload)
      parallel_workload = temp;
  }
  uint64_t L_l_q = 0;
  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    if (!tl->is_request_exist(resource_id))
      continue;
    uint64_t temp = tl->get_request_by_id(resource_id).get_max_length();
    if (L_l_q < temp)
      L_l_q = temp;
  }
  uint64_t test_start = L_l_q, test_end = ti.get_deadline();
  uint64_t theta_i_q = test_start;

  while (theta_i_q < test_end) {
    uint64_t demand = test_start;
    uint64_t L_h_q = 0;
    foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
      if (!th->is_request_exist(resource_id))
        continue;
      const Request& request = th->get_request_by_id(resource_id);
      L_h_q += instance_number((*th), theta_i_q) * request.get_num_requests() *
               request.get_max_length();
    }

    uint64_t interf_h = 0;
    foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
      interf_h += total_workload((*th), theta_i_q);
    }

    uint interf_l = 0;
    foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
      foreach(tl->get_requests(), request_u) {
        if (resource_id == request_u->get_resource_id())
          continue;
        const Resource& resource =
            resources.get_resource_by_id(request_u->get_resource_id());
        if (ti.get_priority() > resource.get_ceiling()) {
          interf_l = instance_number((*tl), theta_i_q) *
                     request_u->get_num_requests() *
                     request_u->get_max_length();
        }
      }
    }

    demand += L_h_q + (interf_h + interf_l + parallel_workload) / p_num;

    if (theta_i_q == demand) {
      return theta_i_q;
    } else {
      theta_i_q = demand;
    }
  }
  return test_end + 100;
}
uint64_t RTA_DAG_CGFP_PIP::request_bound(const DAG_Task& ti,
                                        uint32_t resource_id) {
  uint64_t phi_i_q = phi(ti, resource_id);
  uint64_t theta_i_q = theta(ti, resource_id);
  return min(phi_i_q, theta_i_q);
}

uint32_t RTA_DAG_CGFP_PIP::instance_number(const DAG_Task& ti,
                                            uint64_t interval) {
  // uint64_t cpl = ti.get_critical_path_length();
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  uint32_t MPD = min(ti.get_parallel_degree(),
                     ti.get_dcores());  // Minimum Parallel Degree
  return ceiling(interval + r - e/MPD, p);
}

uint64_t RTA_DAG_CGFP_PIP::total_workload(const DAG_Task& ti,
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

uint64_t RTA_DAG_CGFP_PIP::CS_workload(const DAG_Task& ti,
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

uint64_t RTA_DAG_CGFP_PIP::intra_blocking(const DAG_Task& ti,
                                          uint32_t resource_id,
                                          uint32_t request_num) {
  const Request& request = ti.get_request_by_id(resource_id);
  uint32_t N_i_q = request.get_num_requests();
  uint64_t L_i_q = request.get_max_length();
  uint32_t X_i_q = request_num;
  uint32_t Y_i_q = request.get_num_requests() - X_i_q;
  uint32_t MPD = ti.get_dcores();  // Minimum Parallel Degree
  return min((MPD - 1) * X_i_q, Y_i_q) * L_i_q;
}

uint64_t RTA_DAG_CGFP_PIP::inter_blocking(const DAG_Task& ti,
                                          uint32_t resource_id,
                                          uint32_t request_num) {
  uint64_t L_l_q = 0;
  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    if (!tl->is_request_exist(resource_id))
      continue;
    uint64_t temp = tl->get_request_by_id(resource_id).get_max_length();
    if (L_l_q < temp)
      L_l_q = temp;
  }

  uint64_t inter_b_l = L_l_q * request_num;
  uint64_t inter_h = 0;
  foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
    if (!th->is_request_exist(resource_id))
      continue;
    const Request& request = th->get_request_by_id(resource_id);
    uint32_t N_h_q = request.get_num_requests();
    uint64_t L_h_q = request.get_max_length();
    uint64_t request_b = request_bound(ti, resource_id);
    uint32_t N_h = instance_number((*th), request_b);

    inter_h += N_h * N_h_q * L_h_q;
  }

  return (L_l_q + inter_h) * request_num;
}

uint64_t RTA_DAG_CGFP_PIP::intra_interference(const DAG_Task& ti) {
  return ti.get_wcet() - ti.get_critical_path_length();
}

uint64_t RTA_DAG_CGFP_PIP::inter_interference(const DAG_Task& ti,
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

uint64_t RTA_DAG_CGFP_PIP::response_time(const DAG_Task& ti) {
  uint32_t t_id = ti.get_id();
  uint64_t CPL = ti.get_critical_path_length();
  uint64_t intra_interf = intra_interference(ti);
  uint32_t parallel_degree = ti.get_dcores();
  uint64_t total_blocking = 0;
  uint p_num = processors.get_processor_num();
  uint32_t MPD = min(p_num, min(ti.get_parallel_degree(),
                                ti.get_dcores()));  // Minimum Parallel Degree

  foreach(ti.get_requests(), request) {
    uint32_t r_id = request->get_resource_id();
    uint64_t blocking = 0;
    uint32_t N_i_q = request->get_num_requests();
    for (uint32_t x = 1; x <= N_i_q; x++) {
      uint64_t temp = intra_blocking(ti, r_id, x) + inter_blocking(ti, r_id, x);
      if (temp > blocking)
        blocking = temp;
    }
    total_blocking += blocking;
  }

  uint64_t test_end = ti.get_deadline();
  uint64_t test_start = CPL + total_blocking + intra_interf / ti.get_dcores();
  uint64_t response = test_start;
  uint64_t demand = 0;

  while (response <= test_end) {
    uint64_t inter_interf = inter_interference(ti, response);
    demand = test_start + inter_interf;

    if (response == demand) {
      return response;
    } else {
      response = demand;
    }
  }
return test_end + 100;
}

int RTA_DAG_CGFP_PIP::w_decrease(Interf t1, Interf t2) {
  return ceiling(t1.workload, t1.parallel_degree) >
         ceiling(t2.workload, t2.parallel_degree);
}

uint64_t RTA_DAG_CGFP_PIP::interf_cal(vector<Interf> Q, const DAG_Task& ti,
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

bool RTA_DAG_CGFP_PIP::is_schedulable() {
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
