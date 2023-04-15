// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <lp.h>
#include <math-helper.h>
#include <processors.h>
#include <resources.h>
#include <rta_dag_cgfp.h>
#include <solution.h>
#include <tasks.h>
#include <iostream>
#include <sstream>
#include <exception>

using std::min;
using std::exception;
using std::ostringstream;

RTA_DAG_CGFP::RTA_DAG_CGFP()
    : GlobalSched(false, RTA, FIX_PRIORITY, NONE, "", "") {}

RTA_DAG_CGFP::RTA_DAG_CGFP(DAG_TaskSet tasks, ProcessorSet processors,
                                   ResourceSet resources)
    : GlobalSched(false, RTA, FIX_PRIORITY, NONE, "", "") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;

  this->resources.update(&(this->tasks));
  this->processors.update(&(this->tasks), &(this->resources));

  this->tasks.init();
  this->tasks.RM_Order();
  this->processors.init();
}

RTA_DAG_CGFP::~RTA_DAG_CGFP() {}

uint32_t RTA_DAG_CGFP::instance_number(const DAG_Task& ti,
                                                uint64_t interval) {
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  return ceiling(interval + r, p);
}

uint64_t RTA_DAG_CGFP::total_workload(const DAG_Task& ti,
                                           uint64_t interval) {
  // uint64_t cpl = ti.get_critical_path_length();
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  uint32_t m_i = ti.get_dcores();
  uint64_t extended_interval = interval + r - e/m_i;
  uint64_t workload = floor(extended_interval/p) * e  + min(e, (extended_interval % p) * m_i);
  return workload;
}

uint64_t RTA_DAG_CGFP::response_time(const DAG_Task& ti) {
  uint32_t t_id = ti.get_id();
  uint64_t CPL = ti.get_critical_path_length();
  uint64_t intra_interf = ti.get_wcet() - CPL;
  uint32_t parallel_degree = ti.get_dcores();
  uint p_num = processors.get_processor_num();
  uint64_t test_end = ti.get_deadline();
  uint64_t test_start = CPL + intra_interf / parallel_degree;

  uint64_t response = test_start;
  uint64_t demand = 0;

  while (response <= test_end) {
    uint64_t inter_interf = 0;
    uint32_t higher_cpd = 0;
    foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
      inter_interf += total_workload((*tx), response);
      higher_cpd += tx->get_dcores();
    }
    if (p_num >= (higher_cpd + ti.get_dcores()))
      inter_interf = 0;
    demand = test_start + inter_interf/p_num;
    if (response >= demand) {
      return demand;
    } else {
      response = demand;
    }
  }
  return test_end + 100;
}


bool RTA_DAG_CGFP::is_schedulable() {
  uint p_num = processors.get_processor_num();
  uint64_t response_bound;

  foreach(tasks.get_tasks(), ti) {
    if (ti->get_critical_path_length() > ti->get_deadline())
      return false;
    bool schedulable = false;
    uint64_t C_i = ti->get_wcet();
    uint64_t L_i = ti->get_critical_path_length();
    uint64_t D_i = ti->get_deadline();
    uint64_t T_i = ti->get_period();
    uint32_t initial;
    if (C_i == L_i) {
      initial = 1;
    } else {
      initial= ceiling(C_i - L_i, D_i - L_i);
    }
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

//////////////////////////////////////////

RTA_DAG_CGFP_v2::RTA_DAG_CGFP_v2()
    : RTA_DAG_CGFP() {}

RTA_DAG_CGFP_v2::RTA_DAG_CGFP_v2(DAG_TaskSet tasks, ProcessorSet processors,
                                   ResourceSet resources)
    : RTA_DAG_CGFP(tasks, processors, resources) {
}

RTA_DAG_CGFP_v2::~RTA_DAG_CGFP_v2() {}

uint64_t RTA_DAG_CGFP_v2::total_workload(const DAG_Task& ti,
                                           uint64_t interval) {
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  uint32_t m_i = ti.get_dcores();
  uint64_t extended_interval = interval + r - e/m_i;
  uint64_t workload = floor(extended_interval/p) * e  + min(e, (extended_interval % p) * m_i);
  return workload;
}

uint64_t RTA_DAG_CGFP_v2::intra_interference(const DAG_Task& ti) {
  return ti.get_wcet() - ti.get_critical_path_length();
}

uint64_t RTA_DAG_CGFP_v2::inter_interference(const DAG_Task& ti,
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
  uint64_t inter_interf = interf_cal(Q, ti, p_num);
  return inter_interf;
}

uint64_t RTA_DAG_CGFP_v2::response_time(const DAG_Task& ti) {
  uint32_t t_id = ti.get_id();
  uint64_t CPL = ti.get_critical_path_length();
  uint64_t intra_interf = intra_interference(ti);
  uint32_t parallel_degree = ti.get_dcores();
  uint p_num = processors.get_processor_num();

  uint64_t test_end = ti.get_deadline();
  uint64_t test_start = CPL + intra_interf / parallel_degree;

  uint64_t response = test_start;
  uint64_t demand = 0;

  while (response <= test_end) {
    uint64_t inter_interf = inter_interference(ti, response);
    demand = test_start + inter_interf;

    if (response >= demand) {
      return demand;
    } else {
      response = demand;
    }
  }
  return test_end + 100;
}

int RTA_DAG_CGFP_v2::w_decrease(Interf t1, Interf t2) {
  return ceiling(t1.workload, t1.parallel_degree) >
         ceiling(t2.workload, t2.parallel_degree);
}

uint64_t RTA_DAG_CGFP_v2::interf_cal(vector<Interf> Q, const DAG_Task& ti,
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
      }
    } else {
      return ceiling(I, p_num_left);
    }
  }

  return ceiling(I, p_num_left);
}

bool RTA_DAG_CGFP_v2::is_schedulable() {
  uint p_num = processors.get_processor_num();
  uint64_t response_bound;

  foreach(tasks.get_tasks(), ti) {
    if (ti->get_critical_path_length() > ti->get_deadline())
      return false;
    bool schedulable = false;
    uint64_t C_i = ti->get_wcet();
    uint64_t L_i = ti->get_critical_path_length();
    uint64_t D_i = ti->get_deadline();
    uint64_t T_i = ti->get_period();
    uint32_t initial;
    if (C_i == L_i) {
      initial = 1;
    } else {
      initial= ceiling(C_i - L_i, D_i - L_i);
    }
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

//////////////////////////////////////////

RTA_DAG_CGFP_v3::RTA_DAG_CGFP_v3()
    : RTA_DAG_CGFP_v2() {}

RTA_DAG_CGFP_v3::RTA_DAG_CGFP_v3(DAG_TaskSet tasks, ProcessorSet processors,
                                   ResourceSet resources)
    : RTA_DAG_CGFP_v2(tasks, processors, resources) {
}

RTA_DAG_CGFP_v3::~RTA_DAG_CGFP_v3() {}

uint64_t RTA_DAG_CGFP_v3::inter_interference(const DAG_Task& ti,
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
  uint64_t inter_interf = interf_cal(Q, ti, p_num);
  return inter_interf;
}

uint64_t RTA_DAG_CGFP_v3::response_time(const DAG_Task& ti) {
  uint32_t t_id = ti.get_id();
  uint64_t CPL = ti.get_critical_path_length();
  uint64_t intra_interf = intra_interference(ti);
  uint32_t parallel_degree = ti.get_dcores();
  uint p_num = processors.get_processor_num();

  uint64_t test_end = ti.get_deadline();
  uint64_t test_start = CPL + intra_interf / parallel_degree;

  uint64_t response = test_start;
  uint64_t demand = 0;

  while (response <= test_end) {
    uint64_t inter_interf = inter_interference(ti, response);
    demand = test_start + inter_interf;

    if (response >= demand) {
      return demand;
    } else {
      response = demand;
    }
  }
  return test_end + 100;
}

// DYNAMIC PROGRAMMING
uint64_t RTA_DAG_CGFP_v3::interf_cal(vector<Interf> Q, const DAG_Task& ti,
                                       uint p_num) {
  // cout << "CGFP-DP" << endl;
  uint32_t capacity = p_num - ti.get_dcores();
  uint64_t I = 0;
  typedef struct {
    uint64_t workload;
    uint32_t width;
    vector<uint32_t> t_ids;
  } info;
  vector<info> infos;
  foreach(Q, object) {
    I += object->workload;
    cout << object->t_id << " " << object->workload << " " << object->parallel_degree << endl;
  }
  if (0 == capacity)
    return ceiling(I, p_num);

  for (uint i = 0; i <= capacity; i++) {
    info temp;
    temp.workload = 0;
    temp.width = 0;
    infos.push_back(temp);
  }

  for (uint i = 1; i <= capacity; i++) {
    double ratio = I/p_num;
    uint64_t workload = 0;
    uint32_t width = 0;
    vector<uint32_t> t_ids;
    foreach(Q, q) {
      if (q->parallel_degree <= i) {
        bool exist = false;
        foreach(infos[i - q->parallel_degree].t_ids, t_id) {
          if (q->t_id == (*t_id))
            exist = true;
        }
        if (!exist) {
          double temp = I - q->workload - infos[i - q->parallel_degree].workload;
          temp /= (p_num - q->parallel_degree - infos[i - q->parallel_degree].width);

          if (ratio >= temp) {
            workload = q->workload + infos[i - q->parallel_degree].workload;
            width = q->parallel_degree + infos[i - q->parallel_degree].width;
            t_ids = infos[i - q->parallel_degree].t_ids;
            t_ids.push_back(q->t_id);
            ratio = temp;
          } 
        }
      }
    }

    double a = I - infos[i-1].workload;
    a /= (p_num - infos[i - 1].width);
    
    double b = I - workload;
    b /= (p_num - width);

    if (a >= b) {
      infos[i].workload = workload;
      infos[i].width = width;
      infos[i].t_ids = t_ids;
    } else {
      infos[i].workload = infos[i - 1].workload;
      infos[i].width = infos[i - 1].width;
      infos[i].t_ids = infos[i - 1].t_ids;
    }
  }
  cout << "222" << endl;

  for (uint i = 0; i <= capacity; i++) {
    cout << infos[i].workload << " " << infos[i].width << endl;
  }

  cout << "333" << endl;
  cout << I << " " << infos[capacity].workload << " " << p_num << " "<< infos[capacity].width << endl;
  return ceiling(I - infos[capacity].workload, p_num - infos[capacity].width);


  // uint32_t p_num_left = p_num;
  // sort(Q.begin(), Q.end(), w_decrease);
  // uint64_t I = 0;
  // foreach(Q, object) {
  //   I += object->workload;
  // }

  // foreach(Q, object) {
  //   if (ti.get_id() == object->t_id)
  //     continue;
  //   uint64_t temp_1 = ceiling(object->workload, object->parallel_degree);
  //   uint64_t temp_2 = ceiling(I, p_num_left);
  //   if (temp_1 > temp_2) {
  //     if ((p_num_left - object->parallel_degree) >= ti.get_dcores()) {
  //       I -= object->workload;
  //       p_num_left -= object->parallel_degree;
  //     }
  //   } else {
  //     return ceiling(I, p_num_left);
  //   }
  // }

  // return ceiling(I, p_num_left);
}


bool RTA_DAG_CGFP_v3::is_schedulable() {
  uint p_num = processors.get_processor_num();
  uint64_t response_bound;

  foreach(tasks.get_tasks(), ti) {
    if (ti->get_critical_path_length() > ti->get_deadline())
      return false;
    bool schedulable = false;
    uint64_t C_i = ti->get_wcet();
    uint64_t L_i = ti->get_critical_path_length();
    uint64_t D_i = ti->get_deadline();
    uint64_t T_i = ti->get_period();
    uint32_t initial;
    if (C_i == L_i) {
      initial = 1;
    } else {
      initial= ceiling(C_i - L_i, D_i - L_i);
    }
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


// bool RTA_DAG_CGFP_v3::is_schedulable() {
//   foreach(tasks.get_tasks(), ti) {
//     uint64_t C_i = ti->get_wcet();
//     uint64_t L_i = ti->get_critical_path_length();
//     uint64_t D_i = ti->get_deadline();
//     uint64_t T_i = ti->get_period();
//     uint32_t m_i;
//     if (C_i == L_i) {
//       m_i = 1;
//     } else {
//       m_i= ceiling(C_i - L_i, D_i - L_i);
//     }
//     ti->set_dcores(m_i);
//   }
//   foreach(tasks.get_tasks(), ti) {
//     uint64_t response_bound;
//     response_bound = response_time((*ti));
//     if (response_bound <= ti->get_deadline()) {
//       ti->set_response_time(response_bound);
//     } else {
//     return false;
//     }
//   }
//   return true;
// }
