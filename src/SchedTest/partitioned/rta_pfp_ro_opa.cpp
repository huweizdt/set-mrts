// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <math-helper.h>
#include <math.h>
#include <rta_pfp_ro_opa.h>

using std::min;

RTA_PFP_RO_OPA::RTA_PFP_RO_OPA()
    : PartitionedSched(false, RTA, FIX_PRIORITY, DPCP, "",
                       "Resource-Oriented") {}

RTA_PFP_RO_OPA::RTA_PFP_RO_OPA(TaskSet tasks, ProcessorSet processors,
                               ResourceSet resources)
    : PartitionedSched(false, RTA, FIX_PRIORITY, DPCP, "",
                       "Resource-Oriented") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;

  this->resources.update(&(this->tasks));
  this->processors.update(&(this->tasks), &(this->resources));

  this->tasks.RM_Order();
  this->processors.init();
}

RTA_PFP_RO_OPA::~RTA_PFP_RO_OPA() {}

ulong RTA_PFP_RO_OPA::blocking_bound(const Task& ti, uint r_id) {
  ulong bound = 0;
  const Request& request_q = ti.get_request_by_id(r_id);
  Resource& resource_q = resources.get_resource_by_id(r_id);
  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    foreach(tl->get_requests(), request_v) {
      Resource& resource_v =
          resources.get_resource_by_id(request_v->get_resource_id());
      if (resource_v.get_ceiling() <= resource_q.get_ceiling()) {
        ulong L_l_v = request_v->get_max_length();
        if (L_l_v > bound) bound = L_l_v;
      }
    }
  }
  return bound;
}

ulong RTA_PFP_RO_OPA::request_bound(const Task& ti, uint r_id) {
  ulong deadline = ti.get_deadline();
  const Request& request_q = ti.get_request_by_id(r_id);
  Resource& resource_q = resources.get_resource_by_id(r_id);
  uint p_id = resource_q.get_locality();
  ulong test_start = request_q.get_max_length() + blocking_bound(ti, r_id);
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

    if (temp != bound) {
      bound = temp;
    } else {
      return bound;
    }
  }
  return deadline + 100;
}

ulong RTA_PFP_RO_OPA::formula_30(const Task& ti, uint p_id, ulong interval) {
  ulong miu = 0;

  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    Resource& resource = resources.get_resource_by_id(q);
    if (p_id == resource.get_locality()) {
      miu += request->get_num_requests() * request->get_max_length();
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

ulong RTA_PFP_RO_OPA::angent_exec_bound(const Task& ti, uint p_id,
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

ulong RTA_PFP_RO_OPA::NCS_workload(const Task& ti, ulong interval) {
  ulong e = ti.get_wcet_non_critical_sections();
  ulong p = ti.get_period();
  ulong r = ti.get_response_time();
  return ceiling((interval + r - e), p) * e;
}

ulong RTA_PFP_RO_OPA::CS_workload(const Task& ti, uint resource_id,
                                  ulong interval) {
  ulong p = ti.get_period();
  ulong r = ti.get_response_time();
  const Request& request = ti.get_request_by_id(resource_id);
  ulong agent_length = request.get_num_requests() * request.get_max_length();
  return ceiling((interval + r - agent_length), p) * agent_length;
}

ulong RTA_PFP_RO_OPA::response_time_UA(const Task& ti) {
  ulong response_time = ti.get_wcet_non_critical_sections();

  for (uint k = 0; k < processors.get_processor_num(); k++) {
    response_time += formula_30(ti, k, ti.get_deadline());
  }

  return response_time;
}

ulong RTA_PFP_RO_OPA::response_time_AP(const Task& ti) {
  uint p_id = ti.get_partition();
  ulong test_bound = ti.get_deadline();
  ulong test_start = ti.get_wcet_non_critical_sections();
  ulong response_time = test_start;

#if RTA_DEBUG == 1
  cout << "AP "
       << "Task" << ti.get_id() << " priority:" << ti.get_priority()
       << ": partition:" << ti.get_partition() << endl;
  cout << "ncs-wcet:" << ti.get_wcet_non_critical_sections()
       << " cs-wcet:" << ti.get_wcet_critical_sections()
       << " wcet:" << ti.get_wcet()
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
      ulong agent_bound = angent_exec_bound(ti, k, response_time);
      temp += agent_bound;
    }
#if RTA_DEBUG == 1
    cout << "agent exec bound:" << temp - temp2 << endl;

    cout << "response time:" << temp << endl;
#endif

    assert(temp >= response_time);

    // cout<<"t"<<ti.get_id()<<": wcet:"<<ti.get_wcet()<<"
    // deadline:"<<ti.get_deadline()<<" rt:"<<temp<<endl;

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

ulong RTA_PFP_RO_OPA::response_time_SP(const Task& ti) {
  uint p_id = ti.get_partition();
  ulong test_bound = ti.get_deadline();
  ulong test_start = ti.get_wcet_non_critical_sections();
  ulong response_time = test_start;

#if RTA_DEBUG == 1
  cout << "SP "
       << "Task" << ti.get_id() << ": partition:" << ti.get_partition() << endl;
  cout << "ncs-wcet:" << ti.get_wcet_non_critical_sections()
       << " cs-wcet:" << ti.get_wcet_critical_sections()
       << " wcet:" << ti.get_wcet()
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
        temp += request->get_num_requests() * request->get_max_length();
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

    // cout<<"t"<<ti.get_id()<<": wcet:"<<ti.get_wcet()<<"
    // deadline:"<<ti.get_deadline()<<" rt:"<<temp<<endl;

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

bool RTA_PFP_RO_OPA::worst_fit_for_resources(uint active_processor_num) {
  /*
  foreach(resources.get_resources(), resource)
  {
          cout<<"task size:"<<resource->get_tasks()->get_taskset_size()<<endl;
          cout<<"resource:"<<resource->get_resource_id()<<"
  utilization:"<<resource->get_utilization();
  }
  */
  resources.sort_by_utilization();

  foreach(resources.get_resources(), resource) {
    if (abs(resource->get_utilization()) <= _EPS) continue;
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

bool RTA_PFP_RO_OPA::is_first_fit_for_tasks_schedulable(uint start_processor) {
  bool schedulable;
  uint p_num = processors.get_processor_num();
  // tasks.RM_Order();
  foreach(tasks.get_tasks(), ti) {
    uint assignment;
    schedulable = false;
    for (uint k = start_processor; k < start_processor + p_num; k++) {
      assignment = k % p_num;
      Processor& processor = processors.get_processors()[assignment];

      // cout<<"Try to assign task:"<<ti->get_id()<<" to
      // processor:"<<assignment<<endl;  try to assign ti to processor k
      if (!processor.add_task(ti->get_id())) {
        continue;
      }
      ti->set_partition(assignment);

      if (OPA()) {
        // cout<<"OPA true."<<endl;
        schedulable = true;
        break;
      } else {
        processor.remove_task(ti->get_id());
        ti->set_partition(MAX_INT);
      }
    }
    if (!schedulable) {
      return schedulable;
    }
  }
  return schedulable;
}

bool RTA_PFP_RO_OPA::OPA() {
  set<uint> unassigned_queue;

  foreach(tasks.get_tasks(), ti)
    unassigned_queue.insert(ti->get_id());

  for (int pi = tasks.get_taskset_size(); pi > 0; pi--) {
    bool flag = false;
    // cout<<"Try to assign priority "<<pi<<endl;
    foreach(unassigned_queue, t_id) {
      // cout<<"\ttask:"<<*t_id;
      Task& ti = tasks.get_task_by_id(*t_id);
      ti.set_priority(pi);

      foreach(unassigned_queue, th_id) {
        if ((*th_id) == (*t_id)) continue;

        Task& th = tasks.get_task_by_id(*th_id);
        th.set_priority(0);
        th.set_response_time(th.get_deadline());
      }

      if (alloc_schedulable(&ti)) {
        // cout<<"\tsuccess."<<endl;
        flag = true;
        unassigned_queue.erase(ti.get_id());
        break;
      }
      // else
      //   cout<<"\ffailed."<<endl;
    }

    if (!flag) {
      foreach(unassigned_queue, t_id) {
        Task& ti = tasks.get_task_by_id(*t_id);
        ti.set_priority(MAX_INT);
      }

      return flag;
    }
  }
  return true;
}

bool RTA_PFP_RO_OPA::alloc_schedulable() { return true; }

bool RTA_PFP_RO_OPA::alloc_schedulable(Task* ti) {
  uint p_id = ti->get_partition();
  Processor& processor = processors.get_processors()[p_id];
  ulong response_bound = 0;
  if (MAX_INT == p_id) {
    response_bound = response_time_UA(*ti);
    ti->set_response_time(response_bound);
  } else {
    if (0 == processor.get_resourcequeue().size()) {  // Application Processor
      response_bound = response_time_AP(*ti);
      ti->set_response_time(response_bound);
    } else {  // Synchronization Processor
      response_bound = response_time_SP(*ti);
      ti->set_response_time(response_bound);
    }
  }
  if (response_bound <= ti->get_deadline()) {
    return true;
  } else {
    return false;
  }
}

bool RTA_PFP_RO_OPA::is_schedulable() {
  bool schedulable = false;
  uint p_num = processors.get_processor_num();
  uint r_num = resources.get_resourceset_size();

  for (uint i = 1; i <= p_num; i++) {
    // initialzation
    tasks.init();
    processors.init();
    resources.init();

    if (!worst_fit_for_resources(i)) continue;

    // erase all priorities
    foreach(tasks.get_tasks(), ti)
      ti->set_priority(MAX_INT);

    schedulable = is_first_fit_for_tasks_schedulable(i % p_num);
    if (schedulable) {
      /*
      cout<<"=====success====="<<endl;
              foreach(tasks.get_tasks(), task)
              {
                      cout<<"Task:"<<task->get_id()<<"
      Partition:"<<task->get_partition()<<" priority:"<<task->get_priority()<<"
      U:"<<task->get_utilization()<<endl;
                      cout<<"----------------------------"<<endl;
              }

              for(uint k = 0; k < p_num; k++)
              {
                      cout<<"Processor "<<k<<"
      utilization:"<<processors.get_processors()[k].get_utilization()<<endl;
                      foreach(processors.get_processors()[k].get_taskqueue(),
      t_id)
                      {
                              Task& task = tasks.get_task_by_id(*t_id);
                              cout<<"task"<<task.get_id()<<"\t";
                      }
                      cout<<endl;
              }
      */
      return schedulable;
    }
  }
  /*
  cout<<"=====fail====="<<endl;
                          foreach(tasks.get_tasks(), task)
                          {
                                  cout<<"Task:"<<task->get_id()<<"
  Partition:"<<task->get_partition()<<" priority:"<<task->get_priority()<<endl;
                                  cout<<"----------------------------"<<endl;
                          }

          for(uint k = 0; k < p_num; k++)
          {
                  cout<<"Processor "<<k<<"
  utilization:"<<processors.get_processors()[k].get_utilization()<<endl;
                  foreach(processors.get_processors()[k].get_taskqueue(), t_id)
                  {
                          Task& task = tasks.get_task_by_id(*t_id);
                          cout<<"task"<<task.get_id()<<"\t";
                  }
                  cout<<endl;
          }
  */
  return schedulable;
}
