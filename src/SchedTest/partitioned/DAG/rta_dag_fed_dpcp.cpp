// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <rta_dag_fed_dpcp.h>
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

RTA_DAG_FED_DPCP::RTA_DAG_FED_DPCP()
    : PartitionedSched(false, RTA, FIX_PRIORITY, DPCP, "", "") {}

RTA_DAG_FED_DPCP::RTA_DAG_FED_DPCP(DAG_TaskSet tasks,
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

  // foreach(this->tasks.get_tasks(), ti) {
  //   cout << "Task" << ti->get_id() << ":" <<endl;
  //   foreach(ti->get_requests(), request) {
  //     cout << " r" << request->get_resource_id() << endl;
  //   }
  // }

  // foreach(this->resources.get_resources(), resource) {
  //   cout << "Resource" << resource->get_resource_id() << ":" <<endl;
  //   foreach(resource->get_taskqueue(), t_id) {
  //     cout << " t" << (*t_id) << endl;
  //   }
  // }
}

RTA_DAG_FED_DPCP::~RTA_DAG_FED_DPCP() {}

bool RTA_DAG_FED_DPCP::is_schedulable() {
  if (0 == tasks.get_taskset_size())
    return true;
  uint32_t p_num = processors.get_processor_num();
  uint32_t sum = 0;
  foreach(tasks.get_tasks(), ti) {
    uint64_t C_i = ti->get_wcet();
    uint64_t L_i = ti->get_critical_path_length();
    uint64_t D_i = ti->get_deadline();
    uint64_t T_i = ti->get_period();
    uint32_t initial_m_i = ceiling(C_i - L_i, D_i - L_i);
    ti->set_dcores(initial_m_i);    
    sum += initial_m_i;
  }
  if (p_num < sum) 
    return false;

  bool update = false;
  // cout << "1" << endl;
  while (true) {
  // cout << "2" << endl;
    uint32_t p_id = 0;
    foreach(tasks.get_tasks(), ti) {
      uint32_t m_i = ti->get_dcores();
      Cluster cluster;
      for(uint32_t i = 0; i < m_i; i++) {
        cluster.push_back(p_id++);
      }
      ti->set_cluster(cluster);
    }
  // cout << "3" << endl;

    // foreach(tasks.get_tasks(), ti) {
    //   Cluster cluster = ti->get_cluster();
    //   foreach(cluster, c) {
    //     cout << (*c) << " ";
    //   }
    //   cout << endl;
    // }

    // foreach(resources.get_resources(), resource) {
    //   resource->display_task_queue();
    // }

  // cout << "4" << endl;
    update = false;
    if (!WFD_Resources())
      return false;

  // cout << "5" << endl;
    tasks.update_requests(resources);

    // foreach(resources.get_resources(), resource) {
    //   uint32_t r_id = resource->get_resource_id();
    //   if(is_local_resource(r_id))
    //     cout << "local resource." << endl;
    //   else
    //     cout << "global resource." << endl;
    //   cout << "Ceiling: " << get_ceiling(r_id) << endl;
    //   // cout << "Resource " << resource->get_resource_id() << " location:" << resource->get_locality() << endl;
    //   resource->display_task_queue();
    // }

  // cout << "6" << endl;

    foreach(tasks.get_tasks(), ti) {
  // cout << "7" << endl;
      uint64_t D_i = ti->get_deadline();
      uint64_t response_time = get_response_time((*ti));
      if (response_time > D_i) {
        ti->set_dcores(1 + ti->get_dcores());
        update = true;
      }
  
  // cout << "8" << endl;
    }

  // cout << "9" << endl;
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

uint32_t RTA_DAG_FED_DPCP::instance_number(const DAG_Task& ti, uint64_t interval) {
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  return ceiling(interval + r, p);
}


uint64_t RTA_DAG_FED_DPCP::gamma(const DAG_Task& ti, uint32_t r_id, uint64_t interval) {
  Resource &r_q = resources.get_resource_by_id(r_id);
  uint64_t sum = 0;
  foreach_higher_priority_task(tasks.get_tasks(), ti, tj) {
    foreach(tj->get_requests(), r_j_u) {
      Resource &r_u = resources.get_resource_by_id(r_j_u->get_resource_id());
      if (!is_local_resource(r_u.get_resource_id()) && r_u.get_locality() == r_q.get_locality()) {
        sum += instance_number((*tj), interval) * r_j_u->get_num_requests() * r_j_u->get_max_length();
      }
    }
  }
  return sum;
}

uint64_t RTA_DAG_FED_DPCP::resource_holding(const DAG_Task& ti, Path path, uint32_t r_id) {
  if (!ti.is_request_exist(r_id))
    return 0;
  uint64_t test_start = 0;
  uint64_t test_end = ti.get_deadline();
  const Request &r_i_q = ti.get_request_by_id(r_id);
  test_start += r_i_q.get_max_length();
  uint64_t beta = 0;
  foreach(ti.get_requests(), r_i_u) {
    if (r_i_q.get_locality() != r_i_u->get_locality())
      continue;
    uint32_t N_i_u = r_i_u->get_num_requests();
    uint64_t L_i_u = r_i_u->get_max_length();
    uint32_t num = 0;
    foreach(path.requests, r_u) {
      if (r_u->get_resource_id() != r_i_u->get_resource_id())
        continue;
      else {
        num = r_u->get_num_requests();
        break; 
      }
    }
    test_start += (N_i_u - num) * L_i_u;
  }

  foreach_lower_priority_task(tasks.get_tasks(), ti, tj) {
    foreach(tj->get_requests(), r_j_u) {
      if (r_j_u->get_locality() == r_i_q.get_locality() && get_ceiling(r_j_u->get_resource_id()) >= ti.get_priority()) {
        uint64_t L_j_u = r_j_u->get_max_length();
        if (L_j_u > beta)
          beta = L_j_u;
      }
    }
  }
  test_start += beta;

  uint64_t holding_time = test_start;

  while(holding_time <= test_end) {
    uint64_t temp = test_start + gamma(ti, r_id, holding_time);

    if (temp > holding_time)
      holding_time = temp;
    else
      break;
  }

  if (holding_time > test_end)
    return test_end + 100;
  else
    return holding_time;
}

uint64_t RTA_DAG_FED_DPCP::inter_blocking(const DAG_Task &ti, Path path, uint64_t interval) {
  uint64_t sum = 0;
  foreach(processors.get_processors(), p_k) {
    uint32_t p_id = p_k->get_processor_id();
    uint64_t epsilon = 0, zeta = 0;
    foreach(ti.get_requests(), r_i_q) {
      if (is_local_resource(r_i_q->get_resource_id())||r_i_q->get_locality() != p_id)
        continue;
      uint32_t r_id = r_i_q->get_resource_id();
      uint64_t beta = 0;
      foreach_lower_priority_task(tasks.get_tasks(), ti, tj) {
        foreach(tj->get_requests(), r_j_u) {
          if (r_j_u->get_locality() == r_i_q->get_locality() && get_ceiling(r_j_u->get_resource_id()) >= ti.get_priority()) {
            uint64_t L_j_u = r_j_u->get_max_length();
            if (L_j_u > beta)
              beta = L_j_u;
          }
        }
      }
      uint64_t blocking = beta + gamma(ti, r_id, resource_holding(ti, path, r_id));
      foreach(path.requests, request) {
        if (r_id == request->get_resource_id())
          epsilon += blocking * request->get_num_requests();
      }
      
    }

    foreach_task_except(tasks.get_tasks(), ti, tj) {
      foreach(tj->get_requests(), r_j_q) {
        uint32_t r_id = r_j_q->get_resource_id();
        if (is_local_resource(r_id)||r_j_q->get_locality() != p_id)
          continue;
        zeta = instance_number((*tj), interval) * r_j_q->get_num_requests() * r_j_q->get_max_length();
      }
    }
    sum += min(epsilon, zeta);
  }
  return sum;
}

uint64_t RTA_DAG_FED_DPCP::intra_blocking(const DAG_Task &ti, Path path) {
  uint64_t sum = 0;
  uint64_t local_blocking = 0;
  uint64_t global_blocking = 0;
  foreach(path.requests, r_i_q) {
    uint32_t r_id = r_i_q->get_resource_id();
    if (is_local_resource(r_id)) {
      local_blocking += (ti.get_request_by_id(r_id).get_num_requests() - r_i_q->get_num_requests()) * r_i_q->get_max_length();
    }
  }

  foreach(processors.get_processors(), p_k) {
    uint32_t p_id = p_k->get_processor_id();
    foreach(path.requests, r_i_q) {
      uint32_t r_id = r_i_q->get_resource_id();
      if ((!is_local_resource(r_id)) && p_id == r_i_q->get_locality()) {
        global_blocking += (ti.get_request_by_id(r_id).get_num_requests() - r_i_q->get_num_requests()) * r_i_q->get_max_length();
      }
    }
  }
  return local_blocking + global_blocking;
}

uint64_t RTA_DAG_FED_DPCP::inter_interf(const DAG_Task &ti, Path path, uint64_t interval) {
  uint64_t I_inter = 0;
  foreach(path.requests, r_i_q) {
    uint32_t r_id = r_i_q->get_resource_id();
    if ((!is_local_resource(r_id)) && ti.is_in_cluster(r_i_q->get_locality()))
      I_inter += (ti.get_request_by_id(r_id).get_num_requests() - r_i_q->get_num_requests()) * r_i_q->get_max_length();
  }
  // cout << "I'A:" << I_inter << endl;
  // uint64_t temp = I_inter;
  foreach_task_except(tasks.get_tasks(), ti, tj) {
    foreach(tj->get_requests(), r_j_q) {
      uint32_t r_id = r_j_q->get_resource_id();
      if ((!is_local_resource(r_id)) && ti.is_in_cluster(r_j_q->get_locality()))
        I_inter += instance_number((*tj), interval) * r_j_q->get_num_requests() * r_j_q->get_max_length();
    }
  }
  // temp = I_inter - temp;
  // cout << "IA:" << I_inter << endl;

  return I_inter;
}

uint64_t RTA_DAG_FED_DPCP::intra_interf(const DAG_Task &ti, Path path) {
  uint64_t I_intra = ti.get_wcet_non_critical_sections() - path.wcet_non_critical_section;
  foreach(path.requests, r_i_q) {
    uint32_t r_id = r_i_q->get_resource_id();
    if (is_local_resource(r_id))
      I_intra += (ti.get_request_by_id(r_id).get_num_requests() - r_i_q->get_num_requests()) * r_i_q->get_max_length();
  }
  return I_intra;
}

bool RTA_DAG_FED_DPCP::is_local_resource(uint r_id) {
  Resource &resource = resources.get_resource_by_id(r_id);
  if (1 >= resource.get_taskqueue().size()) {
    return true;
  } else {
    // uint32_t partition = tasks.get_task_by_id(resource.get_taskqueue()[0]).get_partition();
    // foreach(resource.get_taskqueue(), t_id) {
    //   DAG_Task &task = tasks.get_task_by_id((*t_id));
    //   if (partition != task.get_partition())
    //     return false;
    // }
    return false;
  }
  // return true;
}

uint32_t RTA_DAG_FED_DPCP::get_ceiling(uint r_id) {
  Resource &resource = resources.get_resource_by_id(r_id);
  return resource.get_ceiling();
}

bool RTA_DAG_FED_DPCP::WFD_Resources() {
// cout << "1111" << endl;
  // foreach(resources.get_resources(), resource) {
  //   cout << "Resource " << resource->get_resource_id() << " ";
  //   foreach(resource->get_taskqueue(), t_id) {
  //     cout << " T" << (*t_id);
  //   }
  //   cout << endl << " utilization:" << resource->get_utilization() << endl;

  // }
  resources.sort_by_utilization();
  // foreach(resources.get_resources(), resource) {
  //   cout << "Resource " << resource->get_resource_id() << " ";
  //   foreach(resource->get_taskqueue(), t_id) {
  //     cout << " T" << (*t_id);
  //   }
  //   cout << endl << " utilization:" << resource->get_utilization() << endl;

  // }
// cout << "2222" << endl;
  vector<double> U_clusters;
  foreach(tasks.get_tasks(), ti) {
    U_clusters.push_back(ti->get_utilization());
  }
// cout << "3333" << endl;
  foreach(resources.get_resources(), resource) {
// cout << "4444" << endl;
    uint32_t r_id = resource->get_resource_id();
    if (is_local_resource(r_id)) {
      resource->set_locality(MAX_INT);
      continue;
    }
// cout << "5555" << endl;
    uint32_t cluster_id = 0;
    double max = 0;
    for(uint32_t i = 0; i < tasks.get_taskset_size(); i++) {
      // cout << "Task " << tasks.get_tasks()[i].get_id() << endl;
      // cout << "m_i:" << tasks.get_tasks()[i].get_dcores() << endl;
      // cout << "U_clusters[i]:" << U_clusters[i] << endl;
      double gap = tasks.get_tasks()[i].get_dcores() - U_clusters[i];
      // cout << "gap:" << gap << endl;
      if (max < gap) {
        max = gap;
        cluster_id = i;
      }
    }

    // foreach(tasks.get_tasks(), ti) {
    //   Cluster cluster = ti->get_cluster();
    //   foreach(cluster, c) {
    //     cout << (*c) << " ";
    //   }
    //   cout << endl;
    // }
// cout << "6666" << endl;
//     cout << "cluster_id:" << cluster_id << endl;
    // cout << "U_clusters[cluster_id]:" << U_clusters[cluster_id] << endl;
    // cout << "resource_id:" << resource->get_resource_id() << endl;
    // cout << "utilization:" << resource->get_utilization() << endl;
    // cout << "m_i:" << tasks.get_tasks()[cluster_id].get_dcores() << endl;
    if (U_clusters[cluster_id] + resource->get_utilization() > tasks.get_tasks()[cluster_id].get_dcores())
      return false;
    else {
      double min = processors.get_processor_num();
      DAG_Task &task = tasks.get_tasks()[cluster_id];
      uint p_id = task.get_cluster()[0];
      // foreach(task.get_cluster(), c) {
      //   cout << (*c) << " ";
      // }
      // cout << endl;
      // cout << "p_id:" << p_id << endl;

      Cluster temp = task.get_cluster();
      foreach(temp, p_k) {
        // cout << "p_c_id:" << (*p_k) << endl;
        Processor& processor = processors.get_processors()[(*p_k)];
        double p_u = processor.get_resource_utilization();
        // cout << "Processor " << (*p_k) << " utilization:" << p_u << endl;
        if (p_u < min) {
          min = p_u;
          p_id = processor.get_processor_id();
        }
      }


      // cout << "new p_id:" << p_id << endl;
      processors.get_processors()[p_id].add_resource(r_id);
      U_clusters[cluster_id] += resource->get_utilization();
      resource->set_locality(p_id);
    }
// cout << "7777" << endl;
  }
  return true;
}

// Theorem 1
uint64_t RTA_DAG_FED_DPCP::get_response_time(const DAG_Task& ti) {
  Paths paths = ti.get_paths();
  uint32_t m_i = ti.get_dcores();
  uint64_t wcrt = 0;
  uint64_t test_end = ti.get_deadline();
  // cout << "number of paths: " << paths.size() << endl;
  uint32_t idx = 0;
  foreach(paths, path) {

    // cout << "<-------- path " << idx++ << " -------->" << endl;
    //   cout << "  subjobs: ";
    //   uint64_t verified_len = 0;
    //   foreach(path->vnodes, vnode) {
    //     if (0 == vnode->follow_ups.size()) {
    //       cout << "v" << vnode->job_id << endl;
    //       verified_len += vnode->wcet;          
    //     } else {
    //      cout << "v" << vnode->job_id << "->";
    //       verified_len += vnode->wcet;   
    //     }
    //   }
    //   cout << "Verfied length:" << verified_len << endl;
    //   cout << "  wcet:" << path->wcet << " wcet_ncs:" << path->wcet_non_critical_section << " wcet-cs:" << path->wcet_critical_section << endl;
    //   foreach(path->requests, request) {
    //     cout << "  Request:" << request->get_resource_id() << " num:" << request->get_num_requests() << " len:" << request->get_max_length() << endl; 
    //   }
    //   if (verified_len != path->wcet)
    //     cout << "!!!!!!!!!!!!!!!" << endl;

    // if (path->wcet == ti.get_critical_path_length())
    //   cout << "<=======================critical path=============================>" << endl;
    
    if (path->wcet < ti.get_critical_path_length() && 0 == path->wcet_critical_section)
      continue;
    uint64_t test_start = path->wcet;
    uint64_t response_time = path->wcet;
    while (response_time < test_end) {
      uint64_t intra_b = intra_blocking(ti, (*path));
      uint64_t intra_i = intra_interf(ti, (*path));
      uint64_t inter_b = inter_blocking(ti, (*path), response_time);
      uint64_t inter_i = inter_interf(ti, (*path), response_time);
      uint demand = test_start + intra_b + inter_b + (intra_i+inter_i)/m_i;
      // cout << "path length:" << test_start << endl;
      // cout << "off path length:" << ti.get_wcet() - path->wcet << endl;
      // cout << "NCS path length:" << path->wcet_non_critical_section << endl;
      // cout << "NCS off path length:" << ti.get_wcet() - path->wcet_non_critical_section << endl;
      // cout << "CS path length:" << path->wcet_critical_section << endl;
      // cout << "CS off path length:" << ti.get_wcet_critical_sections() - path->wcet_critical_section << endl;
      // cout << "intra_i + length:" << intra_i + test_start << " wcet:" << ti.get_wcet() << endl;
      // cout << "intra_b:" << intra_b << endl;
      // cout << "inter_b:" << inter_b << endl;
      // cout << "intra_i:" << intra_i << endl;
      // cout << "inter_i:" << inter_i << endl;
      // cout << "demand:" << demand << endl;
      if (response_time < demand)
        response_time = demand;
      else 
        break;
    }

    if(response_time > test_end)
      return test_end + 100;
    else 
      if(wcrt < response_time)
        wcrt = response_time;
  }
  // cout << "WCRT:" << wcrt << " deadline:" << test_end << endl;
  return wcrt;
}

bool RTA_DAG_FED_DPCP::alloc_schedulable() {
  
}

/////////////////////////////////////


RTA_DAG_FED_DPCP_v2::RTA_DAG_FED_DPCP_v2()
    : RTA_DAG_FED_DPCP() {}

RTA_DAG_FED_DPCP_v2::RTA_DAG_FED_DPCP_v2(DAG_TaskSet tasks,
                                         ProcessorSet processors,
                                         ResourceSet resources)
    : RTA_DAG_FED_DPCP(tasks, processors, resources) {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;

  this->resources.update(&(this->tasks));
  this->processors.update(&(this->tasks), &(this->resources));

  this->tasks.RM_Order();
  this->processors.init();
}

RTA_DAG_FED_DPCP_v2::~RTA_DAG_FED_DPCP_v2() {}

bool RTA_DAG_FED_DPCP_v2::is_schedulable() {
  if (0 == tasks.get_taskset_size())
    return true;
  uint32_t p_num = processors.get_processor_num();
  uint32_t sum = 0;
  foreach(tasks.get_tasks(), ti) {
    uint64_t C_i = ti->get_wcet();
    uint64_t L_i = ti->get_critical_path_length();
    uint64_t D_i = ti->get_deadline();
    uint64_t T_i = ti->get_period();
    uint32_t initial_m_i = ceiling(C_i - L_i, D_i - L_i);
    ti->set_dcores(initial_m_i);    
    sum += initial_m_i;
  }
  if (p_num < sum) 
    return false;

  bool update = false;
  while (true) {
    uint32_t p_id = 0;
    foreach(tasks.get_tasks(), ti) {
      uint32_t m_i = ti->get_dcores();
      Cluster cluster;
      for(uint32_t i = 0; i < m_i; i++) {
        cluster.push_back(p_id++);
      }
      ti->set_cluster(cluster);
    }

    update = false;
    if (!WFD_Resources())
      return false;

    tasks.update_requests(resources);

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

uint64_t RTA_DAG_FED_DPCP_v2::inter_blocking(const DAG_Task &ti, uint64_t interval) {
  uint64_t sum = 0;
  foreach(processors.get_processors(), p_k) {
    uint32_t p_id = p_k->get_processor_id();
    uint64_t zeta = 0;

    foreach_task_except(tasks.get_tasks(), ti, tj) {
      foreach(tj->get_requests(), r_j_q) {
        uint32_t r_id = r_j_q->get_resource_id();
        if (is_local_resource(r_id)||r_j_q->get_locality() != p_id)
          continue;
        zeta = instance_number((*tj), interval) * r_j_q->get_num_requests() * r_j_q->get_max_length();
      }
    }
    sum += zeta;
  }
  return sum;
}

uint64_t RTA_DAG_FED_DPCP_v2::intra_blocking(const DAG_Task &ti) {
  uint64_t sum = 0;
  uint64_t local_blocking = 0;
  uint64_t global_blocking = 0;
  foreach(ti.get_requests(), r_i_q) {
    uint32_t r_id = r_i_q->get_resource_id();
    if (is_local_resource(r_id)) {
      local_blocking += (r_i_q->get_num_requests() - 1) * r_i_q->get_max_length();
    }
  }

  foreach(processors.get_processors(), p_k) {
    uint32_t p_id = p_k->get_processor_id();
    foreach(ti.get_requests(), r_i_q) {
      uint32_t r_id = r_i_q->get_resource_id();
      if ((!is_local_resource(r_id)) && p_id == r_i_q->get_locality()) {
        global_blocking += (r_i_q->get_num_requests() - 1) * r_i_q->get_max_length();
      }
    }
  }
  return local_blocking + global_blocking;
}

uint64_t RTA_DAG_FED_DPCP_v2::inter_interf(const DAG_Task &ti, uint64_t interval) {
  uint64_t I_inter = 0;
  foreach(ti.get_requests(), r_i_q) {
    uint32_t r_id = r_i_q->get_resource_id();
    if ((!is_local_resource(r_id)) && ti.is_in_cluster(r_i_q->get_locality()))
      I_inter += (r_i_q->get_num_requests() - 1) * r_i_q->get_max_length();
  }
  // cout << "I'A:" << I_inter << endl;
  // uint64_t temp = I_inter;
  foreach_task_except(tasks.get_tasks(), ti, tj) {
    foreach(tj->get_requests(), r_j_q) {
      uint32_t r_id = r_j_q->get_resource_id();
      if ((!is_local_resource(r_id)) && ti.is_in_cluster(r_j_q->get_locality()))
        I_inter += instance_number((*tj), interval) * r_j_q->get_num_requests() * r_j_q->get_max_length();
    }
  }
  // temp = I_inter - temp;
  // cout << "IA:" << I_inter << endl;

  return I_inter;
}

uint64_t RTA_DAG_FED_DPCP_v2::intra_interf(const DAG_Task &ti) {
  uint64_t I_intra = ti.get_wcet() - ti.get_critical_path_length();
  foreach(ti.get_requests(), r_i_q) {
    uint32_t r_id = r_i_q->get_resource_id();
    if (is_local_resource(r_id))
      I_intra += (r_i_q->get_num_requests() - 1) * r_i_q->get_max_length();
  }
  return I_intra;
}

// Theorem 1
uint64_t RTA_DAG_FED_DPCP_v2::get_response_time(const DAG_Task& ti) {
  uint32_t m_i = ti.get_dcores();
  uint64_t test_end = ti.get_deadline();
  // cout << "number of paths: " << paths.size() << endl;
  uint32_t idx = 0;
  uint64_t test_start = ti.get_critical_path_length();
  uint64_t response_time = test_start;
  while (response_time < test_end) {
    uint64_t intra_b = intra_blocking(ti);
    uint64_t intra_i = intra_interf(ti);
    uint64_t inter_b = inter_blocking(ti, response_time);
    uint64_t inter_i = inter_interf(ti, response_time);
    uint demand = test_start + intra_b + inter_b + (intra_i+inter_i)/m_i;
    // cout << "path length:" << test_start << endl;
    // cout << "intra_b:" << intra_b << endl;
    // cout << "inter_b:" << inter_b << endl;
    // cout << "intra_i:" << intra_i << endl;
    // cout << "inter_i:" << inter_i << endl;
    // cout << "demand:" << demand << endl;
    if (response_time < demand)
      response_time = demand;
    else 
      break;
  }

  if (test_end < response_time)
    return test_end + 100;
  else
    return response_time;
}