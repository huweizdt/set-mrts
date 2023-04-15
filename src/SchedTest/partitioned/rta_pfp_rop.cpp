// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <math-helper.h>
#include <math.h>
#include <rta_pfp_rop.h>

using std::min;

RTA_PFP_ROP::RTA_PFP_ROP()
    : PartitionedSched(false, RTA, FIX_PRIORITY, DPCP, "",
                       "Resource-Oriented") {}

RTA_PFP_ROP::RTA_PFP_ROP(TaskSet tasks, ProcessorSet processors,
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

RTA_PFP_ROP::~RTA_PFP_ROP() {}

ulong RTA_PFP_ROP::blocking_bound(const Task& ti, uint r_id) {
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

ulong RTA_PFP_ROP::request_bound(const Task& ti, uint r_id) {
  ulong deadline = ti.get_deadline();
  const Request& request_q = ti.get_request_by_id(r_id);
  Resource& resource_q = resources.get_resource_by_id(r_id);
  uint p_id = resource_q.get_locality();
  uint64_t blocking = blocking_bound(ti, r_id);
  ulong test_start = request_q.get_max_length() + blocking;
  // ulong temp = tasks.get_task_by_id(ti.get_id()).get_total_blocking();
  // tasks.get_task_by_id(ti.get_id()).set_total_blocking(blocking);
  ulong bound = test_start;

  // cout<<"resuest_bound for R_"<<ti.get_id()<<"_"<<r_id<<":"<<endl;

  while (bound <= deadline) {
    ulong temp = test_start;
    // cout << bound << endl;
    foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
      foreach(th->get_requests(), request_v) {
        Resource& resource_v =
            resources.get_resource_by_id(request_v->get_resource_id());
        if (p_id == resource_v.get_locality()) {
          uint64_t CSW = CS_workload(*th, request_v->get_resource_id(), bound);
          // cout << "preempt by R_"<<th->get_id()<<"_"<<request_v->get_resource_id()<<":"<<CSW<<endl;
          temp += CSW;
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

ulong RTA_PFP_ROP::formula_30(const Task& ti, uint p_id, ulong interval) {
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

ulong RTA_PFP_ROP::angent_exec_bound(const Task& ti, uint p_id,
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
        // cout<<"exceed."<<endl;
        return r_b;
      }
      lambda += N_i_q * r_b;
    }
  }

  ulong miu = formula_30(ti, p_id, interval);

  return lambda;
  // return min(lambda, miu);
}

ulong RTA_PFP_ROP::NCS_workload(const Task& ti, ulong interval) {
  ulong e = ti.get_wcet_non_critical_sections();
  ulong p = ti.get_period();
  ulong r = ti.get_response_time();
  return ceiling((interval + r - e), p) * e;
}

ulong RTA_PFP_ROP::CS_workload(const Task& ti, uint resource_id,
                               ulong interval) {
  ulong p = ti.get_period();
  ulong r = ti.get_response_time();
  const Request& request = ti.get_request_by_id(resource_id);
  ulong agent_length = request.get_num_requests() * request.get_max_length();
  return ceiling((interval + r - agent_length), p) * agent_length;
}

ulong RTA_PFP_ROP::response_time_AP(const Task& ti) {
  uint p_id = ti.get_partition();
  ulong test_bound = ti.get_deadline();
  ulong test_start = ti.get_wcet_non_critical_sections();
  ulong response_time = test_start;
  // cout << "Task " << ti.get_id()<<endl;

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
      if (p_id == k) continue;
      uint64_t agent_bound = angent_exec_bound(ti, k, response_time);
      // cout<<"AB of processor"<<k<<":"<<agent_bound<<endl;
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

ulong RTA_PFP_ROP::response_time_SP(const Task& ti) {
  uint p_id = ti.get_partition();
  ulong test_bound = ti.get_deadline();
  ulong test_start = ti.get_wcet_non_critical_sections();
  ulong response_time = test_start;

  // cout << "Task " << ti.get_id()<<endl;

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
    ulong t2 = temp;
    for (uint k = 0; k < processors.get_processor_num(); k++) {  // Theata
      if (p_id == k) continue;
      uint64_t agent_bound = angent_exec_bound(ti, k, response_time);
      // cout << "Theta:" << agent_bound <<endl;
      temp += agent_bound;
    }
#if RTA_DEBUG == 1
    cout << "Theata:" << temp - temp2 << endl;
    temp2 = temp;

    cout << "response time:" << temp << endl;
#endif
    tasks.get_task_by_id(ti.get_id()).set_other_attr(temp - t2);

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

bool RTA_PFP_ROP::worst_fit_for_resources(uint active_processor_num) {
  
  // foreach(resources.get_resources(), resource)
  // {
  //         cout<<"task size:"<<resource->get_tasks()->get_taskset_size()<<endl;
  //         cout<<"resource:"<<resource->get_resource_id()<<" utilization:"<<resource->get_utilization();
  // }
  
  resources.sort_by_utilization();

  // processors.update(&tasks, &resources);

  foreach(resources.get_resources(), resource) {
    // cout<<"assign resource:"<<resource->get_resource_id()<<endl;
    // if(abs(resource->get_utilization()) <= _EPS)
    // continue;
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

bool RTA_PFP_ROP::is_first_fit_for_tasks_schedulable(uint start_processor) {
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
        ti->set_partition(assignment);
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

bool RTA_PFP_ROP::is_specific_partition_schedulable() {
  uint p_id = 0;

  // resource allocation
  Resources &rs = resources.get_resources();

  p_id = 0;
  rs[0].set_locality(p_id);
  processors.get_processors()[p_id].add_resource(rs[0].get_resource_id());

  p_id = 0;
  rs[1].set_locality(p_id);
  processors.get_processors()[p_id].add_resource(rs[1].get_resource_id());

  // task allocation
  tasks.update_requests(resources);
  Tasks &ts = tasks.get_tasks();

  p_id = 1;
  ts[0].set_partition(p_id);
  if (!processors.get_processors()[p_id].add_task(ts[0].get_id())) {
    cout<<"task allocation failed"<<endl;
    return false;
  }
  if (!alloc_schedulable(&ts[0])) {
    return false;
  }

  p_id = 2;
  ts[1].set_partition(p_id);
  if (!processors.get_processors()[p_id].add_task(ts[1].get_id())) {
    cout<<"task allocation failed"<<endl;
    return false;
  }
  if (!alloc_schedulable(&ts[1])) {
    return false;
  }

  p_id = 2;
  ts[2].set_partition(p_id);
  if (!processors.get_processors()[p_id].add_task(ts[2].get_id())) {
    cout<<"task allocation failed"<<endl;
    return false;
  }
  if (!alloc_schedulable(&ts[2])) {
    return false;
  }

  p_id = 1;
  ts[3].set_partition(p_id);
  if (!processors.get_processors()[p_id].add_task(ts[3].get_id())) {
    cout<<"task allocation failed"<<endl;
    return false;
  }
  if (!alloc_schedulable(&ts[3])) {
    return false;
  }

  p_id = 2;
  ts[4].set_partition(p_id);
  if (!processors.get_processors()[p_id].add_task(ts[4].get_id())) {
    cout<<"task allocation failed"<<endl;
    return false;
  }
  if (!alloc_schedulable(&ts[4])) {
    return false;
  }

  p_id = 1;
  ts[5].set_partition(p_id);
  if (!processors.get_processors()[p_id].add_task(ts[5].get_id())) {
    cout<<"task allocation failed"<<endl;
    return false;
  }
  if (!alloc_schedulable(&ts[5])) {
    return false;
  }

  p_id = 1;
  ts[6].set_partition(p_id);
  if (!processors.get_processors()[p_id].add_task(ts[6].get_id())) {
    cout<<"task allocation failed"<<endl;
    return false;
  }
  if (!alloc_schedulable(&ts[6])) {
    return false;
  }

  p_id = 3;
  ts[7].set_partition(p_id);
  if (!processors.get_processors()[p_id].add_task(ts[7].get_id())) {
    cout<<"task allocation failed"<<endl;
    return false;
  }
  if (!alloc_schedulable(&ts[7])) {
    return false;
  }

  p_id = 3;
  ts[8].set_partition(p_id);
  if (!processors.get_processors()[p_id].add_task(ts[8].get_id())) {
    cout<<"task allocation failed"<<endl;
    return false;
  }
  if (!alloc_schedulable(&ts[8])) {
    return false;
  }

  p_id = 1;
  ts[9].set_partition(p_id);
  if (!processors.get_processors()[p_id].add_task(ts[9].get_id())) {
    cout<<"task allocation failed"<<endl;
    return false;
  }
  if (!alloc_schedulable(&ts[9])) {
    return false;
  }

  return true;
}

bool RTA_PFP_ROP::alloc_schedulable() {
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

bool RTA_PFP_ROP::alloc_schedulable(Task* ti) {
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

bool RTA_PFP_ROP::is_schedulable() {
  bool schedulable = false;
  uint p_num = processors.get_processor_num();
  uint r_num = resources.get_resourceset_size();

  for (uint i = 1; i <= p_num; i++) {
    // initialzation
    tasks.init();
    processors.init();
    resources.init();

    if (!worst_fit_for_resources(i)) continue;

    tasks.update_requests(resources);

    /*
                    foreach(processors.get_processors(), processor)
                    {
                            cout<<"Processor:"<<processor->get_processor_id()<<endl;
                            cout<<"\t";
                            foreach(processor->get_resourcequeue(), id)
                                    cout<<(*id)<<"\t";
                            cout<<endl;
                    }
    */
    // foreach(tasks.get_tasks(), task) {
    //   foreach(task->get_requests(), request) {
    //     Resource &resource = resources.get_resource_by_id(request->get_resource_id());
    //     cout << "request" << request->get_resource_id() << ":"
    //           << " num:" << request->get_num_requests()
    //           << " length:" << request->get_max_length() << " locality:"
    //           << request->get_locality() << " Resource:" 
    //           << resource.get_resource_id() << " locality2:"
    //           << resource.get_locality() << " resource holding:"
    //           << request_bound((*task), request->get_resource_id())
    //           << endl;
    //   }
    // }

    // foreach(resources.get_resources(), resource) {
    //   cout << "Resource:" << resource->get_resource_id()
    //        << " locality:" << resource->get_locality() << endl;
    //   if (p_num < resource->get_locality()) exit(0);
    // }
// cout << "I:" << i << endl;
    schedulable = is_first_fit_for_tasks_schedulable(i % p_num);
    // schedulable = is_specific_partition_schedulable();
    if (schedulable) {
      // cout << "<=====R-ROP=====>" << endl;
      // if (schedulable) 
      //   cout << "schedulable" << endl;
      // else
      //   cout << "unschedulable" << endl;
        
      // foreach(resources.get_resources(), resource) {
      //   cout << "Resource:" << resource->get_resource_id()
      //        << " locality:" << resource->get_locality()
      //        << " priority ceiling:" << resource->get_ceiling()
      //        << " Utilization:" << resource->get_utilization() << endl;
      //   if (p_num < resource->get_locality()) exit(0);
      //   cout << "Taskqueue:";
      //   foreach(resource->get_taskqueue(), t_id) {
      //     cout << *t_id << " ";
      //   }
      //   cout << endl;
      // }
      // foreach(tasks.get_tasks(), task) {
      //   cout << "Task" << task->get_id()
      //        << ": partition:" << task->get_partition()
      //        << " priority:" << task->get_priority() << endl;
      //   cout << "ncs-wcet:" << task->get_wcet_non_critical_sections()
      //        << " cs-wcet:" << task->get_wcet_critical_sections()
      //        << " wcet:" << task->get_wcet()
      //        << " response time:" << task->get_response_time()
      //        << " deadline:" << task->get_deadline()
      //        << " period:" << task->get_period() << endl;
      //   foreach(task->get_requests(), request) {
      //     cout << "request" << request->get_resource_id() << ":"
      //          << " num:" << request->get_num_requests()
      //          << " length:" << request->get_max_length() << " locality:"
      //          << resources.get_resource_by_id(request->get_resource_id())
      //                 .get_locality()
      //          << endl;
      //   }
      //   cout << "-------------------------------------------" << endl;
      //   if (task->get_wcet() > task->get_response_time()) exit(0);
      // }

      return schedulable;
    }
  }

    //   foreach(tasks.get_tasks(), task) {
    //   cout << "Task" << task->get_id()
    //         << ": partition:" << task->get_partition()
    //         << ": priority:" << task->get_priority() << endl;
    //   cout << "ncs-wcet:" << task->get_wcet_non_critical_sections()
    //         << " cs-wcet:" << task->get_wcet_critical_sections()
    //         << " wcet:" << task->get_wcet()
    //         << " response time:" << task->get_response_time()
    //         << " deadline:" << task->get_deadline()
    //         << " period:" << task->get_period() << endl;
    //   cout << "blocking:" << task->get_total_blocking()
    //         << " Theata:" << task->get_other_attr() << endl;
    //   foreach(task->get_requests(), request) {
    //     cout << "request" << request->get_resource_id() << ":"
    //           << " num:" << request->get_num_requests()
    //           << " length:" << request->get_max_length() << " locality:"
    //           << resources.get_resource_by_id(request->get_resource_id())
    //                 .get_locality()
    //           << endl;
    //   }
    //   cout << "-------------------------------------------" << endl;
    //   if (task->get_wcet() > task->get_response_time()) exit(0);
    // }

    // foreach(processors.get_processors(), processor) {
    //   cout << "Processor " << processor->get_processor_id()
    //         << " utilization:" << processor->get_utilization() << endl;
    //   if (1 <= processor->get_utilization()) {
    //     cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
    //   }
    // }

  return schedulable;
}


TaskSet RTA_PFP_ROP::get_TaskSet() {
  return tasks;
}

ResourceSet RTA_PFP_ROP::get_ResourceSet() {
  return resources;
}

ProcessorSet RTA_PFP_ROP::get_ProcessorSet() {
  return processors;
}
