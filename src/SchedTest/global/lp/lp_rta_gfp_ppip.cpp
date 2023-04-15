// Copyright [2021] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <lp.h>
#include <lp_rta_gfp_ppip.h>
#include <math-helper.h>
#include <processors.h>
#include <resources.h>
#include <solution.h>
#include <tasks.h>
#include <iostream>
#include <sstream>

using std::max;
using std::min;
using std::exception;
using std::ostringstream;

/** Class PPIPMapper */

uint64_t PPIPMapper::encode_request(uint64_t task_id, uint64_t res_id,
                                    uint64_t req_id, uint64_t type) {
  uint64_t one = 1;
  uint64_t key = 0;
  assert(task_id < (one << 10));
  assert(res_id < (one << 4));
  assert(req_id < (one << 16));
  assert(type < (one << 3));

  key |= (type << 30);
  key |= (task_id << 20);
  key |= (res_id << 16);
  key |= req_id;
  return key;
}

uint64_t PPIPMapper::get_type(uint64_t var) {
  return (var >> 30) & (uint64_t)0x7;  // 3 bits
}

uint64_t PPIPMapper::get_task(uint64_t var) {
  return (var >> 20) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t PPIPMapper::get_res_id(uint64_t var) {
  return (var >> 10) & (uint64_t)0xf;  // 4 bits
}

uint64_t PPIPMapper::get_req_id(uint64_t var) {
  return var & (uint64_t)0xffff;  // 16 bits
}

PPIPMapper::PPIPMapper(uint start_var) : VarMapperBase(start_var) {}

uint PPIPMapper::lookup(uint task_id, uint res_id, uint req_id, var_type type) {
  uint64_t key = encode_request(task_id, res_id, req_id, type);
  uint var = var_for_key(key);
  return var;
}

string PPIPMapper::key2str(uint64_t key, uint var) const {
  ostringstream buf;

  switch (get_type(key)) {
    case PPIPMapper::REQUEST_NUM:
      buf << "Y[";
      break;
    case PPIPMapper::BLOCKING_DIRECT:
      buf << "Bd[";
      break;
    case PPIPMapper::BLOCKING_INDIRECT:
      buf << "Bi[";
      break;
    case PPIPMapper::BLOCKING_PREEMPT:
      buf << "Bp[";
      break;
    case PPIPMapper::BLOCKING_OTHER:
      buf << "Bo[";
      break;
    case PPIPMapper::INTERF_REGULAR:
      buf << "I[";
      break;
    default:
      buf << "?[";
  }

  buf << get_task(key) << ", " << get_res_id(key) << ", " << get_req_id(key)
      << "]";

  return buf.str();
}

/** Class LP_RTA_GFP_PPIP */

LP_RTA_GFP_PPIP::LP_RTA_GFP_PPIP()
    : RTA_DAG_GFP_JOSE() {}

LP_RTA_GFP_PPIP::LP_RTA_GFP_PPIP(DAG_TaskSet tasks, ProcessorSet processors,
                                 ResourceSet resources)
    : RTA_DAG_GFP_JOSE(tasks, processors, resources) {}

LP_RTA_GFP_PPIP::~LP_RTA_GFP_PPIP() {}

bool LP_RTA_GFP_PPIP::is_schedulable() {
  bool update = false;

  foreach(tasks.get_tasks(), ti) {
    ti->set_response_time(ti->get_critical_path_length());
  }

  do {
    update = false;
    foreach(tasks.get_tasks(), ti) {
      ulong old_response_time = ti->get_response_time();
      ulong response_time = get_response_time(*ti);

      if (old_response_time != response_time) update = true;

      // cout<<"current response time:"<<temp<<endl;

      if (response_time <= ti->get_deadline()) {
        ti->set_response_time(response_time);
      } else {
        return false;
      }
    }
  } while (update);

  return true;
}

ulong LP_RTA_GFP_PPIP::get_response_time(const DAG_Task& ti) {
  ulong response_time = 0;

  PPIPMapper vars;
  LinearProgram response_bound;
  LinearExpression* obj = new LinearExpression();

  objective(ti, &response_bound, &vars, obj);
  response_bound.set_objective(obj);
  add_constraints(ti, &response_bound, &vars);

  GLPKSolution* rb_solution =
      new GLPKSolution(response_bound, vars.get_num_vars());

  assert(rb_solution != NULL);

  if (rb_solution->is_solved()) {
    double result = rb_solution->evaluate(*(response_bound.get_objective()));
    response_time = result + ti.get_critical_path_length();
  } else {
    cout << "unsolved." << endl;
    delete rb_solution;
    return MAX_LONG;
  }

#if GLPK_MEM_USAGE_CHECK == 1
  int peak;
  glp_mem_usage(NULL, &peak, NULL, NULL);
  cout << "Peak memory usage:" << peak << endl;
#endif

  delete rb_solution;
  return response_time;
}


uint64_t LP_RTA_GFP_PPIP::workload_bound(const DAG_Task& tx, int64_t interval) {
  int64_t L_x = tx.get_critical_path_length();
  int64_t T_x = tx.get_period();
  int64_t W_x = tx.get_wcet();
  int64_t delta_C = interval - max((int64_t)0, (interval - L_x)/ T_x) * T_x;
  int64_t bound = max((int64_t)0, (interval - delta_C)/ T_x) * W_x + (int64_t)get_WC(tx, delta_C);
  return bound;
}

uint32_t LP_RTA_GFP_PPIP::instance_number(const DAG_Task& ti,
                                                int64_t interval) {
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  return ceiling(interval + r, p);
}


uint64_t LP_RTA_GFP_PPIP::resource_holding(const DAG_Task& ti, const DAG_Task& tx, uint32_t r_id) {
  if (!tx.is_request_exist(r_id))
    return 0;
  uint32_t p_num = processors.get_processor_num();
  uint32_t pi_y = min(ti.get_priority(), tx.get_priority());
  uint32_t pi_z = max(ti.get_priority(), tx.get_priority());
  uint64_t rht = tx.get_request_by_id(r_id).get_max_length();
  uint64_t test_start = rht, test_end = ti.get_deadline();

  while (rht < test_end) {
    uint64_t temp = test_start;

    uint interf = 0;
    foreach_higher_priority_task_then(tasks.get_tasks(), pi_y, th) {
      interf += workload_bound((*th),rht);
    }

    foreach(resources.get_resources(), resource) {
      uint32_t res_u = resource->get_resource_id();
      if (resource->get_ceiling() > pi_y || res_u == r_id)
        continue;
      
      if (ti.is_request_exist(res_u)) {
        const Request& r_i_u = ti.get_request_by_id(res_u);
        interf += r_i_u.get_num_requests() * r_i_u.get_max_length();
      }

      if (tx.is_request_exist(res_u)) {
        const Request& r_x_u = tx.get_request_by_id(res_u);
        interf += r_x_u.get_num_requests() * r_x_u.get_max_length();
      }

      foreach_lower_priority_task_then(tasks.get_tasks(), pi_y, tl) {
        if (pi_z == tl->get_priority())
          continue;
        if (tl->is_request_exist(res_u)) {
          const Request& r_l_u = tl->get_request_by_id(res_u);
          interf += instance_number((*tl), rht) * r_l_u.get_num_requests() * r_l_u.get_max_length();
        }
      }
    }

    temp += interf / p_num;

    if (temp > rht)
      rht = temp;
    else {
      return rht;
    }
  }
  return test_end + 100;
}

uint64_t LP_RTA_GFP_PPIP::delay_per_request(const DAG_Task& ti, uint32_t r_id) {
  if (!ti.is_request_exist(r_id))
    return 0;

  uint32_t p_num = processors.get_processor_num();
  uint32_t priority = ti.get_priority();
  uint64_t dpr = 0;


  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    if(tl->is_request_exist(r_id)) {
      uint64_t rht = resource_holding(ti, (*tl), r_id);
      if (dpr < rht)
        dpr = rht;
    }
  }

  uint32_t m_i = ti.get_parallel_degree();
  uint32_t N_i_q = ti.get_request_by_id(r_id).get_num_requests();
  dpr += (min(m_i, N_i_q) - 1) * resource_holding(ti, ti, r_id);

  uint64_t test_start = dpr, test_end = ti.get_deadline();

  while (dpr < test_end) {
    uint64_t temp = test_start;

    foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
      if (!th->is_request_exist(r_id))
        continue;
      temp += instance_number((*th), dpr) * th->get_request_by_id(r_id).get_num_requests() * resource_holding(ti, (*th), r_id);
    }

    if (temp > dpr)
      dpr = temp;
    else {
      return dpr;
    }
  }
  return test_end + 100;
}

/** Expressions */

void LP_RTA_GFP_PPIP::declare_variable_bounds(const DAG_Task& ti,
                                                      LinearProgram* lp,
                                                      PPIPMapper* vars) {
  uint var_id;

  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    var_id = vars->lookup(ti.get_id(), q, 0, PPIPMapper::REQUEST_NUM);
    lp->declare_variable_integer(var_id);
    lp->declare_variable_bounds(var_id, true, 0, true, request->get_num_requests());
  }

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      uint max_request_num;
      if (ti.get_id() == tx->get_id()) {
        max_request_num = ti.get_request_by_id(q).get_num_requests();
      } else {
        max_request_num = tx->get_max_request_num(q, ti.get_response_time());
      }
      for (uint v = 0; v < max_request_num; v++ ) {
        var_id = vars->lookup(x, q, v, PPIPMapper::BLOCKING_DIRECT);
        lp->declare_variable_bounds(var_id, true, 0, true, 1);

        var_id = vars->lookup(x, q, v, PPIPMapper::BLOCKING_INDIRECT);
        lp->declare_variable_bounds(var_id, true, 0, true, 1);

        var_id = vars->lookup(x, q, v, PPIPMapper::BLOCKING_PREEMPT);
        lp->declare_variable_bounds(var_id, true, 0, true, 1);
      }
    }
  }

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    var_id = vars->lookup(x, 0, 0, PPIPMapper::INTERF_REGULAR);
    lp->declare_variable_bounds(var_id, true, 0, false, -1);
  }
}

void LP_RTA_GFP_PPIP::objective(const DAG_Task& ti, LinearProgram* lp,
                                        PPIPMapper* vars,
                                        LinearExpression* obj) {
  uint p_num = processors.get_processor_num();
  int64_t var_id; 

  // cout << "task set size:" << tasks.get_taskset_size() << endl;

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    // uint x_2 = tx->get_index();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      uint max_request_num;
      if (ti.get_id() == tx->get_id()) {
        max_request_num = ti.get_request_by_id(q).get_num_requests();
      } else {
        max_request_num = tx->get_max_request_num(q, ti.get_response_time());
      }
      uint64_t length = request->get_max_length();
      // cout << "max request num [" << ti.get_id() << "," << tx->get_id() << "]:" << max_request_num << endl;
      // cout << "max request num 2[" << ti.get_index() << "," << tx->get_index() << "]:" << max_request_num << endl;
      for (uint v = 0; v < max_request_num; v++ ) {
        var_id = vars->lookup(x, q, v, PPIPMapper::BLOCKING_DIRECT);
        obj->add_term(var_id, length);
      }
    }
  }

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      uint max_request_num;
      if (ti.get_id() == tx->get_id()) {
        max_request_num = ti.get_request_by_id(q).get_num_requests();
      } else {
        max_request_num = tx->get_max_request_num(q, ti.get_response_time());
      }
      uint64_t length = request->get_max_length();
      for (uint v = 0; v < max_request_num; v++ ) {
        var_id = vars->lookup(x, q, v, PPIPMapper::BLOCKING_INDIRECT);
        obj->add_term(var_id, (1.0 /p_num) * length);
      }
    }
  }

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      uint max_request_num;
      if (ti.get_id() == tx->get_id()) {
        max_request_num = ti.get_request_by_id(q).get_num_requests();
      } else {
        max_request_num = tx->get_max_request_num(q, ti.get_response_time());
      }
      uint64_t length = request->get_max_length();
      for (uint v = 0; v < max_request_num; v++ ) {
        var_id = vars->lookup(x, q, v, PPIPMapper::BLOCKING_PREEMPT);
        obj->add_term(var_id, (1.0 /p_num) * length);
      }
    }
  }

  foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    var_id = vars->lookup(x, 0, 0, PPIPMapper::INTERF_REGULAR);
    obj->add_term(var_id, 1.0/p_num);
  }

  var_id = vars->lookup(ti.get_id(), 0, 0, PPIPMapper::INTERF_REGULAR);
  obj->add_term(var_id, 1.0/p_num);

  // vars->seal();
}

void LP_RTA_GFP_PPIP::add_constraints(const DAG_Task& ti, LinearProgram* lp,
                                              PPIPMapper* vars) {
  constraint_1(ti, lp, vars);
  constraint_2(ti, lp, vars);
  declare_variable_bounds(ti, lp, vars);
  vars->seal();
  constraint_3(ti, lp, vars);
  constraint_4(ti, lp, vars);
  constraint_5(ti, lp, vars);
  constraint_6(ti, lp, vars);
  constraint_7(ti, lp, vars);
  constraint_8(ti, lp, vars);
}

// Constraint 1&2
void LP_RTA_GFP_PPIP::constraint_1(const DAG_Task& ti, LinearProgram* lp,
                                           PPIPMapper* vars) {

  int64_t var_id; 
  
  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    LinearExpression* exp = new LinearExpression();
    var_id = vars->lookup(x, 0, 0, PPIPMapper::INTERF_REGULAR);
    exp->add_term(var_id);
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      uint64_t length = request->get_max_length();
      uint max_request_num;
      if (ti.get_id() == tx->get_id()) {
        max_request_num = ti.get_request_by_id(q).get_num_requests();
      } else {
        max_request_num = tx->get_max_request_num(q, ti.get_response_time());
      }
      for (uint v = 0; v < max_request_num; v++ ) {
        var_id = vars->lookup(x, q, v, PPIPMapper::BLOCKING_DIRECT);
        exp->add_term(var_id, length);

        var_id = vars->lookup(x, q, v, PPIPMapper::BLOCKING_INDIRECT);
        exp->add_term(var_id, length);

        var_id = vars->lookup(x, q, v, PPIPMapper::BLOCKING_PREEMPT);
        exp->add_term(var_id, length);
      }
    }

    if (ti.get_id() == tx->get_id()) {
      lp->add_inequality(exp, ti.get_wcet()-ti.get_critical_path_length());
    } else {
      lp->add_inequality(exp, workload_bound((*tx),ti.get_response_time()));
    }
  }
}

// Constraint 3
void LP_RTA_GFP_PPIP::constraint_2(const DAG_Task& ti, LinearProgram* lp,
                                           PPIPMapper* vars) {
  int64_t var_id;                                         
  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      uint max_request_num;
      if (x == ti.get_id()) {
        max_request_num = ti.get_request_by_id(q).get_num_requests();
      } else {
        max_request_num = tx->get_max_request_num(q, ti.get_response_time());
      }
      for (uint v = 0; v < max_request_num; v++ ) {
        LinearExpression* exp = new LinearExpression();

        var_id = vars->lookup(x, q, v, PPIPMapper::BLOCKING_DIRECT);
        exp->add_term(var_id);

        var_id = vars->lookup(x, q, v, PPIPMapper::BLOCKING_INDIRECT);
        exp->add_term(var_id);

        var_id = vars->lookup(x, q, v, PPIPMapper::BLOCKING_PREEMPT);
        exp->add_term(var_id);

        lp->add_inequality(exp, 1);
      }
    }
  }
}

// Constraint 4
void LP_RTA_GFP_PPIP::constraint_3(const DAG_Task& ti, LinearProgram* lp,
                                           PPIPMapper* vars) {
  int64_t var_id;       
  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    LinearExpression* exp = new LinearExpression();
    uint max_request_num = request->get_num_requests();
    for (uint v = 0; v < max_request_num; v++ ) {
      var_id = vars->lookup(ti.get_id(), q, v, PPIPMapper::BLOCKING_DIRECT);
      exp->add_term(var_id, 1);
    }
    var_id = vars->lookup(ti.get_id(), q, 0, PPIPMapper::REQUEST_NUM);
    exp->add_term(var_id, 1);

    lp->add_inequality(exp, request->get_num_requests());
  }
}

// Constraint 5
void LP_RTA_GFP_PPIP::constraint_4(const DAG_Task& ti, LinearProgram* lp,
                                           PPIPMapper* vars) {
  int64_t var_id;    
  int32_t term;   
  foreach(ti.get_requests(), request) {
    uint q = request->get_resource_id();
    LinearExpression* exp = new LinearExpression();
    uint max_request_num = request->get_num_requests();
    for (uint v = 0; v < max_request_num; v++ ) {
      var_id = vars->lookup(ti.get_id(), q, v, PPIPMapper::BLOCKING_DIRECT);
      exp->add_term(var_id, 1);
    }
    term = 1 - ti.get_parallel_degree();
    var_id = vars->lookup(ti.get_id(), q, 0, PPIPMapper::REQUEST_NUM);
    exp->add_term(var_id, term);

    lp->add_inequality(exp, 0);
  }
}

// Constraint 7
void LP_RTA_GFP_PPIP::constraint_5(const DAG_Task& ti, LinearProgram* lp,
                                           PPIPMapper* vars) {
  uint var_id;  

  foreach(resources.get_resources(), resource) {
    uint q = resource->get_resource_id();
    LinearExpression* exp = new LinearExpression();
    foreach_lower_priority_task(tasks.get_tasks(), ti, tx) {
      uint x = tx->get_id();
      if (!tx->is_request_exist(q))
        continue;
      uint max_request_num= tx->get_max_request_num(q, ti.get_response_time());
      for (uint v = 0; v < max_request_num; v++ ) {
        var_id = vars->lookup(x, q, v, PPIPMapper::BLOCKING_DIRECT);
        exp->add_term(var_id, 1);
      }
    }
    if (ti.is_request_exist(q)) {
      var_id = vars->lookup(ti.get_id(), q, 0, PPIPMapper::REQUEST_NUM);
      exp->add_term(var_id, -1);
    }

    lp->add_inequality(exp, 0);
  }
}

// Constraint 8
void LP_RTA_GFP_PPIP::constraint_6(const DAG_Task& ti, LinearProgram* lp,
                            PPIPMapper* vars) {
  uint var_id;  

  foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      LinearExpression* exp = new LinearExpression();

      uint max_request_num= tx->get_max_request_num(q, ti.get_response_time());
      for (uint v = 0; v < max_request_num; v++ ) {
        var_id = vars->lookup(x, q, v, PPIPMapper::BLOCKING_DIRECT);
        exp->add_term(var_id, 1);
      }

      int32_t term = instance_number((*tx), delay_per_request(ti, q)) * request->get_num_requests();
      if (ti.is_request_exist(q)) {
        var_id = vars->lookup(ti.get_id(), q, 0, PPIPMapper::REQUEST_NUM);
        exp->add_term(var_id, -1 * term);
      }
      lp->add_inequality(exp, 0);
    }
  }
}

// Constraint 10
void LP_RTA_GFP_PPIP::constraint_7(const DAG_Task& ti, LinearProgram* lp,
                                           PPIPMapper* vars) {
  uint var_id;  
  int32_t term = 0; 

  foreach(resources.get_resources(), resource) {
    uint q = resource->get_resource_id();
    LinearExpression* exp = new LinearExpression();
    foreach_lower_priority_task(tasks.get_tasks(), ti, tx) {
      uint x = tx->get_id();
      if (!tx->is_request_exist(q))
        continue;
      uint max_request_num= tx->get_max_request_num(q, ti.get_response_time());
      for (uint v = 0; v < max_request_num; v++ ) {
        var_id = vars->lookup(x, q, v, PPIPMapper::BLOCKING_INDIRECT);
        exp->add_term(var_id, 1);

        var_id = vars->lookup(x, q, v, PPIPMapper::BLOCKING_PREEMPT);
        exp->add_term(var_id, 1);
      }
    }

    if (ti.is_request_exist(q)) {
      var_id = vars->lookup(ti.get_id(), q, 0, PPIPMapper::REQUEST_NUM);
      exp->add_term(var_id);
      term = ti.get_request_by_id(q).get_num_requests();
    }

    foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
      if (th->is_request_exist(q)) {
        term += th->get_max_request_num(q, ti.get_response_time());
      }
    }
    lp->add_inequality(exp, term);
  }
}

// Constraint 11
void LP_RTA_GFP_PPIP::constraint_8(const DAG_Task& ti, LinearProgram* lp,
                                           PPIPMapper* vars) {
  uint var_id;  
  int32_t term = 0; 

  foreach(resources.get_resources(), resource) {
    uint q = resource->get_resource_id();
    if (ti.get_priority() >= resource->get_ceiling())
      continue;
    LinearExpression* exp = new LinearExpression();
    foreach_lower_priority_task(tasks.get_tasks(), ti, tx) {
      uint x = tx->get_id();
      if (!tx->is_request_exist(q))
        continue;
      uint max_request_num= tx->get_max_request_num(q, ti.get_response_time());
      for (uint v = 0; v < max_request_num; v++ ) {
        var_id = vars->lookup(x, q, v, PPIPMapper::BLOCKING_INDIRECT);
        exp->add_term(var_id);

        var_id = vars->lookup(x, q, v, PPIPMapper::BLOCKING_PREEMPT);
        exp->add_term(var_id);
      }
    }
    lp->add_equality(exp, 0);
  }
}
