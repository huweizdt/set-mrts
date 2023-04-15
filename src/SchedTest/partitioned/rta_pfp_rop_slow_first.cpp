// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <math-helper.h>
#include <math.h>
#include <rta_pfp_rop_slow_first.h>

using std::min;

RTA_PFP_ROP_SLOW_FIRST::RTA_PFP_ROP_SLOW_FIRST()
    : PartitionedSched(false, RTA, FIX_PRIORITY, DPCP, "",
                       "Resource-Oriented") {}

RTA_PFP_ROP_SLOW_FIRST::RTA_PFP_ROP_SLOW_FIRST(TaskSet tasks,
                                               ProcessorSet processors,
                                               ResourceSet resources)
    : PartitionedSched(false, RTA, FIX_PRIORITY, DPCP, "",
                       "Resource-Oriented") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;

  this->resources.update(&(this->tasks));
  this->processors.update(&(this->tasks), &(this->resources));

  this->processors.init();
  this->processors.sort_by_speedfactor(INCREASE);
}

RTA_PFP_ROP_SLOW_FIRST::~RTA_PFP_ROP_SLOW_FIRST() {}

double RTA_PFP_ROP_SLOW_FIRST::get_total_power() {
  return processors.get_total_power_heterogeneous();
}

ulong RTA_PFP_ROP_SLOW_FIRST::blocking_bound(const Task& ti, uint r_id) {
  ulong bound = 0;
  const Request& request_q = ti.get_request_by_id(r_id);
  Resource& resource_q = resources.get_resource_by_id(r_id);
  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    foreach(tl->get_requests(), request_v) {
      Resource& resource_v =
          resources.get_resource_by_id(request_v->get_resource_id());
      if (resource_v.get_ceiling() <= resource_q.get_ceiling()) {
        double speedfactor =
            processors.get_processors()[resource_v.get_locality()]
                .get_speedfactor();
        ulong L_l_v = ceiling(request_v->get_max_length(), speedfactor);
        if (L_l_v > bound) bound = L_l_v;
      }
    }
  }
  return bound;
}

ulong RTA_PFP_ROP_SLOW_FIRST::request_bound(const Task& ti, uint r_id) {
  ulong deadline = ti.get_deadline();
  const Request& request_q = ti.get_request_by_id(r_id);
  Resource& resource_q = resources.get_resource_by_id(r_id);
  double speedfactor =
      processors.get_processors()[resource_q.get_locality()].get_speedfactor();
  uint p_id = resource_q.get_locality();
  ulong test_start = ceiling(request_q.get_max_length(), speedfactor) +
                     blocking_bound(ti, r_id);
  ulong bound = test_start;

  while (bound <= deadline) {
    ulong temp = test_start;
    foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
      foreach(th->get_requests(), request_v) {
        Resource& resource_v =
            resources.get_resource_by_id(request_v->get_resource_id());
        if (p_id == resource_v.get_locality()) {
          temp += CS_workload(*th, request_v->get_resource_id(), bound);
        }
      }
    }

    assert(temp >= bound);

    if (temp != bound)
      bound = temp;
    else
      return bound;
  }
  return deadline + 100;
}

ulong RTA_PFP_ROP_SLOW_FIRST::formula_30(const Task& ti, uint p_id,
                                         ulong interval) {
  ulong miu = 0;

  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    Resource& resource = resources.get_resource_by_id(q);
    if (p_id == resource.get_locality()) {
      double speedfactor = processors.get_processors()[resource.get_locality()]
                               .get_speedfactor();
      miu += request->get_num_requests() *
             ceiling(request->get_max_length(), speedfactor);
    }
  }

  foreach_task_except(tasks.get_tasks(), ti, tj) {
    foreach(tj->get_requests(), request_v) {
      uint v = request_v->get_resource_id();
      Resource& resource_v = resources.get_resource_by_id(v);
      if (p_id == resource_v.get_locality()) {
        miu += CS_workload(*tj, v, interval);
      }
    }
  }
  return miu;
}

ulong RTA_PFP_ROP_SLOW_FIRST::angent_exec_bound(const Task& ti, uint p_id,
                                                ulong interval) {
  ulong deadline = ti.get_deadline();
  ulong lambda = 0;
  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    Resource& resource = resources.get_resource_by_id(q);

    if (p_id == resource.get_locality()) {
      uint N_i_q = request->get_num_requests();
      ulong r_b = request_bound(ti, q);
      if (deadline < r_b) {
        return r_b;
      }
      lambda += N_i_q * r_b;
    }
  }

  ulong miu = formula_30(ti, p_id, interval);

  return min(lambda, miu);
}

ulong RTA_PFP_ROP_SLOW_FIRST::NCS_workload(const Task& ti, ulong interval) {
  ulong e = ti.get_wcet_non_critical_sections_heterogeneous();
  ulong p = ti.get_period();
  ulong r = ti.get_response_time();
  return ceiling((interval + r - e), p) * e;
}

ulong RTA_PFP_ROP_SLOW_FIRST::CS_workload(const Task& ti, uint resource_id,
                                          ulong interval) {
  ulong p = ti.get_period();
  ulong r = ti.get_response_time();
  const Request& request = ti.get_request_by_id(resource_id);
  Resource& resource = resources.get_resource_by_id(request.get_resource_id());
  double speedfactor =
      processors.get_processors()[resource.get_locality()].get_speedfactor();
  ulong agent_length = request.get_num_requests() *
                       ceiling(request.get_max_length(), speedfactor);
  return ceiling((interval + r - agent_length), p) * agent_length;
}

ulong RTA_PFP_ROP_SLOW_FIRST::response_time_AP(const Task& ti) {
  uint p_id = ti.get_partition();
  ulong test_bound = ti.get_deadline();
  ulong test_start = ti.get_wcet_non_critical_sections_heterogeneous();
  ulong response_time = test_start;
#if RTA_DEBUG == 1
  cout << "AP "
       << "Task" << ti.get_id() << " priority:" << ti.get_priority()
       << ": partition:" << ti.get_partition() << endl;
  cout << "ncs-wcet:" << ti.get_wcet_non_critical_sections_heterogeneous()
       << " cs-wcet:" << ti.get_wcet_critical_sections_heterogeneous()
       << " wcet:" << ti.get_wcet_heterogeneous()
       << " response time:" << ti.get_response_time()
       << " deadline:" << ti.get_deadline() << " period:" << ti.get_period()
       << " utilization:" << ti.get_utilization() << endl;
  foreach(ti.get_requests(), request) {
    cout << "request" << request->get_resource_id() << ":"
         << " resource_u"
         << resources.get_resource_by_id(request->get_resource_id())
                .get_utilization()
                
         << " num:" << request->get_num_requests()
         << " length:" << request->get_max_length() << " locality:"
         << resources.get_resource_by_id(request->get_resource_id()).get_locality()
         << endl;
  }
#endif
  while (response_time <= test_bound) {
    ulong temp = test_start;
    ulong temp2 = temp;

    foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
#if RTA_DEBUG == 1
      cout << "Th:" << th->get_id() << " partition:" << th->get_partition()
           << " priority:" << th->get_priority() << endl;
#endif
      if (th->get_partition() == ti.get_partition()) {
        temp += NCS_workload(*th, response_time);
      }
    }
#if RTA_DEBUG == 1
    cout << "NCS workload:" << temp - test_start << endl;
    temp2 = temp;
#endif

    for (uint k = 0; k < processors.get_processor_num(); k++) {
      if (p_id == k) continue;
      ulong agent_bound = angent_exec_bound(ti, k, response_time);
      temp += agent_bound;
    }
#if RTA_DEBUG == 1
    cout << "agent exec bound:" << temp - temp2 << endl;

    cout << "response time:" << temp << endl;
#endif

    assert(temp >= response_time);

    if (temp != response_time) {
      response_time = temp;
    } else if (temp == response_time) {
#if RTA_DEBUG == 1
      cout << "==============================" << endl;
#endif
      return response_time;
    }
  }
    // cout<<"AP miss."<<endl;
#if RTA_DEBUG == 1
  cout << "==============================" << endl;
#endif
  return test_bound + 100;
}

ulong RTA_PFP_ROP_SLOW_FIRST::response_time_SP(const Task& ti) {
  uint p_id = ti.get_partition();
  ulong test_bound = ti.get_deadline();
  ulong test_start = ti.get_wcet_non_critical_sections_heterogeneous();
  ulong response_time = test_start;

#if RTA_DEBUG == 1
  cout << "SP "
       << "Task" << ti.get_id() << ": partition:" << ti.get_partition() << endl;
  cout << "ncs-wcet:" << ti.get_wcet_non_critical_sections_heterogeneous()
       << " cs-wcet:" << ti.get_wcet_critical_sections_heterogeneous()
       << " wcet:" << ti.get_wcet_heterogeneous()
       << " response time:" << ti.get_response_time()
       << " deadline:" << ti.get_deadline() << " period:" << ti.get_period()
       << " utilization:" << ti.get_utilization() << endl;
  foreach(ti.get_requests(), request) {
    cout << "request" << request->get_resource_id() << ":"
         << " resource_u"
         << resources.get_resource_by_id(request->get_resource_id())
                .get_utilization()
                
         << " num:" << request->get_num_requests()
         << " length:" << request->get_max_length() << " locality:"
         << resources.get_resource_by_id(request->get_resource_id()).get_locality()
         << endl;
  }
#endif

  while (response_time <= test_bound) {
    ulong temp = test_start;
    ulong temp2 = temp;

    foreach(ti.get_requests(), request) {  // A
      uint q = request->get_resource_id();
      Resource& resource = resources.get_resource_by_id(q);
      if (p_id == resource.get_locality()) {
        double speedfactor =
            processors.get_processors()[resource.get_locality()]
                .get_speedfactor();
        temp += request->get_num_requests() *
                ceiling(request->get_max_length(), speedfactor);
      }
    }

#if RTA_DEBUG == 1
    cout << "A:" << temp - temp2 << endl;
    temp2 = temp;
#endif

    foreach_higher_priority_task(tasks.get_tasks(), ti, th) {  // W
      if (p_id == th->get_partition()) {
        temp += NCS_workload(*th, response_time);
      }
    }

#if RTA_DEBUG == 1
    cout << "NCS workload:" << temp - temp2 << endl;
    temp2 = temp;
#endif

    foreach_task_except(tasks.get_tasks(), ti, tj) {
      foreach(tj->get_requests(), request_v) {
        uint v = request_v->get_resource_id();
        Resource& resource_v = resources.get_resource_by_id(v);
        if (p_id == resource_v.get_locality()) {
          temp += CS_workload(*tj, v, response_time);
        }
      }
    }
#if RTA_DEBUG == 1
    cout << "CS workload:" << temp - temp2 << endl;
    temp2 = temp;
#endif

    for (uint k = 0; k < processors.get_processor_num(); k++) {  // Theata
      if (p_id == k) continue;

      temp += angent_exec_bound(ti, k, response_time);
    }
#if RTA_DEBUG == 1
    cout << "Theata:" << temp - temp2 << endl;
    temp2 = temp;

    cout << "response time:" << temp << endl;
#endif

    assert(temp >= response_time);

    if (temp != response_time) {
      response_time = temp;
    } else if (temp == response_time) {
#if RTA_DEBUG == 1
      cout << "==============================" << endl;
#endif
      return response_time;
    }
  }
    // cout<<"SP miss."<<endl;
#if RTA_DEBUG == 1
  cout << "==============================" << endl;
#endif
  return test_bound + 100;
}

bool RTA_PFP_ROP_SLOW_FIRST::worst_fit_for_resources(
    uint active_processor_num) {
  resources.sort_by_utilization();
  foreach(resources.get_resources(), resource) {
    double r_utilization = 1;
    uint assignment = 0;
    for (uint i = 0; i < active_processor_num; i++) {
      Processor& p_temp = processors.get_processors()[i];
      if (r_utilization > p_temp.get_resource_utilization()) {
        r_utilization = p_temp.get_resource_utilization();
        assignment = i;
      }
    }
    Processor& processor = processors.get_processors()[assignment];
    if (processor.add_resource(resource->get_resource_id())) {
      resource->set_locality(assignment);
    } else {
      return false;
    }
  }
  return true;
}

bool RTA_PFP_ROP_SLOW_FIRST::is_first_fit_for_tasks_schedulable(
    uint start_processor) {
  bool schedulable;
  uint p_num = processors.get_processor_num();
  // tasks.RM_Order();
  foreach(tasks.get_tasks(), ti) {
    uint assignment;
    schedulable = false;
    for (uint i = start_processor; i < start_processor + p_num; i++) {
      assignment = i % p_num;
      Processor& processor = processors.get_processors()[assignment];

      if (processor.add_task(ti->get_id())) {
        ti->set_partition(assignment, processor.get_speedfactor());
        if (alloc_schedulable(&(*ti))) {
          schedulable = true;
          break;
        } else {
          ti->init();
          processor.remove_task(ti->get_id());
        }
      }
    }
    if (!schedulable) {
      return schedulable;
    }
  }
  return schedulable;
}

bool RTA_PFP_ROP_SLOW_FIRST::alloc_schedulable() {
  ulong response_bound = 0;
  foreach(tasks.get_tasks(), ti) {
    uint p_id = ti->get_partition();
    if (MAX_INT == p_id) continue;

    Processor& processor = processors.get_processors()[p_id];
    if (0 == processor.get_resourcequeue().size()) {  // Application Processor
      response_bound = response_time_AP((*ti));
    } else {  // Synchronization Processor
      response_bound = response_time_SP((*ti));
    }
    if (response_bound <= ti->get_deadline()) {
      ti->set_response_time(response_bound);
    } else {
      return false;
    }
  }
  return true;
}

bool RTA_PFP_ROP_SLOW_FIRST::alloc_schedulable(Task* ti) {
  uint p_id = ti->get_partition();
  if (MAX_INT == p_id) return false;

  Processor& processor = processors.get_processors()[p_id];
  ulong response_bound = 0;
  if (0 == processor.get_resourcequeue().size()) {  // Application Processor
    response_bound = response_time_AP(*ti);
    ti->set_response_time(response_bound);
  } else {  // Synchronization Processor
    response_bound = response_time_SP(*ti);
    ti->set_response_time(response_bound);
  }

  if (response_bound <= ti->get_deadline()) {
    return true;
  } else {
    return false;
  }
}

bool RTA_PFP_ROP_SLOW_FIRST::is_schedulable() {
  bool schedulable = false;
  uint p_num = processors.get_processor_num();
  // uint p_num;
  uint r_num = resources.get_resourceset_size();

  uint fast_p_num = 0;

  foreach(processors.get_processors(), processor)
    if (1 > processor->get_speedfactor()) fast_p_num++;

  /*	if(0 == fast_p_num)
                  p_num = processors.get_processor_num();
          else
                  p_num = fast_p_num;*/

  if (0 != resources.get_resourceset_size()) {
    for (uint i = 1; i <= p_num; i++) {
      // initialzation
      tasks.init();
      processors.init();
      resources.init();

      if (!worst_fit_for_resources(i)) continue;

      foreach(resources.get_resources(), resource) {
        cout << "Resource:" << resource->get_resource_id()
             << " locality:" << resource->get_locality() << endl;
        if (p_num < resource->get_locality()) exit(0);
      }

      schedulable = is_first_fit_for_tasks_schedulable(i % p_num);
      if (schedulable) {
        cout << "Power:" << get_total_power() << endl;
        return schedulable;
      }
    }
  } else {
    for (uint i = 0; i < p_num; i++) {
      // initialzation
      tasks.init();
      processors.init();
      resources.init();

      schedulable = is_first_fit_for_tasks_schedulable(i % p_num);
      if (schedulable) {
        cout << "Power:" << get_total_power() << endl;
        return schedulable;
      }
    }
  }
  return schedulable;
}
