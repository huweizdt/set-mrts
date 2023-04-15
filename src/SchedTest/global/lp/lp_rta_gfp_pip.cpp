// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <lp.h>
#include <lp_rta_gfp_pip.h>
#include <math-helper.h>
#include <solution.h>
#include <iostream>
#include <sstream>

using std::min;
using std::max;
using std::ostringstream;

/** Class PIPMapper */
uint64_t PIPMapper::encode_request(uint64_t task_id, uint64_t res_id,
                                   uint64_t req_id, uint64_t type) {
  uint64_t one = 1;
  uint64_t key = 0;
  assert(task_id < (one << 10));
  assert(res_id < (one << 4));
  assert(req_id < (one << 16));
  assert(type < (one << 3));

  key |= (type << 30);
  key |= (task_id << 20);
  key |= (res_id << 10);
  key |= req_id;
  return key;
}

uint64_t PIPMapper::get_type(uint64_t var) {
  return (var >> 30) & (uint64_t)0x7;  // 3 bits
}

uint64_t PIPMapper::get_task(uint64_t var) {
  return (var >> 20) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t PIPMapper::get_res_id(uint64_t var) {
  return (var >> 10) & (uint64_t)0xf;  // 4 bits
}

uint64_t PIPMapper::get_req_id(uint64_t var) {
  return var & (uint64_t)0xffff;  // 16 bits
}

PIPMapper::PIPMapper(uint start_var) : VarMapperBase(start_var) {}

uint PIPMapper::lookup(uint task_id, uint res_id, uint req_id, var_type type) {
  uint64_t key = encode_request(task_id, res_id, req_id, type);
  uint var = var_for_key(key);
  return var;
}

string PIPMapper::key2str(uint64_t key, uint var) const {
  ostringstream buf;

  switch (get_type(key)) {
    case PIPMapper::BLOCKING_DIRECT:
      buf << "Xd[";
      break;
    case PIPMapper::BLOCKING_INDIRECT:
      buf << "Xi[";
      break;
    case PIPMapper::BLOCKING_PREEMPT:
      buf << "Xp[";
      break;
    case PIPMapper::BLOCKING_OTHER:
      buf << "Xo[";
      break;
    case PIPMapper::INTERF_REGULAR:
      buf << "Ir[";
      break;
    case PIPMapper::INTERF_CO_BOOSTING:
      buf << "Ic[";
      break;
    case PIPMapper::INTERF_STALLING:
      buf << "Is[";
      break;
    case PIPMapper::INTERF_OTHER:
      buf << "Io[";
      break;
    default:
      buf << "?[";
  }

  buf << get_task(key) << ", " << get_res_id(key) << ", " << get_req_id(key)
      << "]";

  return buf.str();
}

/** Class LP_RTA_GFP_PIP */

LP_RTA_GFP_PIP::LP_RTA_GFP_PIP()
    : GlobalSched(true, RTA, FIX_PRIORITY, PIP, "", "PIP") {}

LP_RTA_GFP_PIP::LP_RTA_GFP_PIP(TaskSet tasks, ProcessorSet processors,
                               ResourceSet resources)
    : GlobalSched(true, RTA, FIX_PRIORITY, PIP, "", "PIP") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;

  this->resources.update(&(this->tasks));
  this->processors.update(&(this->tasks), &(this->resources));

  this->tasks.RM_Order();
  this->processors.init();
}

LP_RTA_GFP_PIP::~LP_RTA_GFP_PIP() {}

bool LP_RTA_GFP_PIP::is_schedulable() {
  bool update = false;

  do {
    update = false;
    foreach(tasks.get_tasks(), ti) {
      ulong old_response_time = ti->get_response_time();
      ulong response_t = response_time(*ti);

      if (old_response_time != response_t) update = true;

      if (response_t <= ti->get_deadline()) {
        ti->set_response_time(response_t);
      } else {
        return false;
      }
    }
  } while (update);

  return true;
}

ulong LP_RTA_GFP_PIP::response_time(const Task& ti) {
  ulong response_t = 0;

  PIPMapper vars;
  LinearProgram response_bound;
  LinearExpression* obj = new LinearExpression();

  lp_pip_objective(ti, &response_bound, &vars, obj);

  response_bound.set_objective(obj);

  lp_pip_add_constraints(ti, &response_bound, &vars);

  // vars->seal();

  GLPKSolution* rb_solution =
      new GLPKSolution(response_bound, vars.get_num_vars());

  assert(rb_solution != NULL);

  if (rb_solution->is_solved()) {
    assert(ti.get_response_time() >= ti.get_wcet());
    ulong gap = ti.get_response_time() - ti.get_wcet();
    double result = rb_solution->evaluate(*(response_bound.get_objective()));

    if ((result < gap) && (gap - result < _EPS)) {
      response_t = ti.get_response_time();
    } else {
      response_t = result + ti.get_wcet();
      assert(response_t < MAX_LONG);
    }
  } else {
    delete rb_solution;
    return MAX_LONG;
  }

#if GLPK_MEM_USAGE_CHECK == 1
  int peak;
  glp_mem_usage(NULL, &peak, NULL, NULL);
  cout << "Peak memory usage:" << peak << endl;
#endif

  delete rb_solution;
  return response_t;
}

ulong LP_RTA_GFP_PIP::workload_bound(const Task& tx, ulong Ri) {
  ulong e = tx.get_wcet();
  ulong d = tx.get_deadline();
  ulong p = tx.get_period();
  ulong r = tx.get_response_time();
  assert(d >= r);

  ulong N = (Ri - e + r) / p;

  return N * e + min(e, Ri + r - e - N * p);
}

ulong LP_RTA_GFP_PIP::holding_bound(const Task& ti, const Task& tx,
                                    uint resource_id) {
  uint x = tx.get_priority(), q = resource_id;
  uint p_num = processors.get_processor_num();
  ulong L_x_q = tx.get_request_by_id(q).get_max_length();
  ulong holding_time = L_x_q;

  if (p_num < tx.get_priority()) {
    uint y = min(ti.get_priority(), tx.get_priority());
    uint z = max(ti.get_priority(), tx.get_priority());
    bool update;
    do {
      update = false;
      ulong temp = 0;

      foreach_higher_priority_task_then(tasks.get_tasks(), y, th) {
        temp += workload_bound(*th, holding_time);
      }

      foreach_lower_priority_task_then(tasks.get_tasks(), y, tl) {
        uint l = tl->get_priority();
        if (z != l) {
          foreach(tl->get_requests(), request) {
            uint u = request->get_resource_id();
            Resource& resource = resources.get_resource_by_id(u);
            if (y > resource.get_ceiling()) {
              uint N_l_u = request->get_num_requests();
              uint L_l_u = request->get_max_length();
              temp += tl->get_max_job_num(holding_time) * N_l_u * L_l_u;
            }
          }
        }
      }
      temp /= p_num;
      temp += L_x_q;
      // assert(temp >= holding_time);
      if (temp > ti.get_deadline()) return MAX_LONG;

      if (temp > holding_time) {
        update = true;
        holding_time = temp;
      }
    } while (update);
  }

  return holding_time;
}

ulong LP_RTA_GFP_PIP::wait_time_bound(const Task& ti, uint resource_id) {
  ulong wait_time = 0;
  ulong holding_time_l = 0;

  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    ulong temp = 0;

    if (tl->is_request_exist(resource_id))
      temp = holding_bound(ti, *tl, resource_id);

    if (temp > holding_time_l) holding_time_l = temp;
  }

  if (holding_time_l == MAX_LONG) {
    // cout<<"holding_time_l:"<<holding_time_l<<endl;
    return MAX_LONG;
  }

  bool update;
  do {
    update = false;
    ulong temp = 1 + holding_time_l;

    foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
      if (th->is_request_exist(resource_id)) {
        uint N_h_q = th->get_request_by_id(resource_id).get_num_requests();
        ulong H_h_q = holding_bound(ti, *th, resource_id);
        temp += th->get_max_job_num(wait_time) * N_h_q * H_h_q;
      }
    }

    if (temp > ti.get_deadline()) {
      // cout<<"temp > ti.get_deadline():"<<temp<<" >
      // "<<ti.get_deadline()<<endl;
      return MAX_LONG;
    }

    // cout<<"temp:"<<temp<<endl;
    // cout<<"wait_time:"<<wait_time<<endl;
    // assert(temp >= wait_time);

    if (temp != wait_time) {
      update = true;
      wait_time = temp;
    }
  } while (update);

  return wait_time;
}

/** Expressions */
void LP_RTA_GFP_PIP::lp_pip_directed_blocking(const Task& ti, const Task& tx,
                                              PIPMapper* vars,
                                              LinearExpression* exp,
                                              double coef) {
  // cout<<"coef:"<<coef<<endl;
  uint x = tx.get_priority();
  // cout<<"task:"<<x<<endl
  foreach(tx.get_requests(), request) {
    uint q = request->get_resource_id();
    ulong length = request->get_max_length();
    foreach_request_instance(ti, tx, q, v) {
      uint var_id;

      var_id = vars->lookup(x, q, v, PIPMapper::BLOCKING_DIRECT);
      exp->add_term(var_id, coef * length);
    }
  }
}

void LP_RTA_GFP_PIP::lp_pip_indirected_blocking(const Task& ti, const Task& tx,
                                                PIPMapper* vars,
                                                LinearExpression* exp,
                                                double coef) {
  // cout<<"coef:"<<coef<<endl;
  uint x = tx.get_priority();
  foreach(tx.get_requests(), request) {
    uint q = request->get_resource_id();
    ulong length = request->get_max_length();
    foreach_request_instance(ti, tx, q, v) {
      uint var_id;

      var_id = vars->lookup(x, q, v, PIPMapper::BLOCKING_INDIRECT);
      exp->add_term(var_id, coef * length);
    }
  }
}

void LP_RTA_GFP_PIP::lp_pip_preemption_blocking(const Task& ti, const Task& tx,
                                                PIPMapper* vars,
                                                LinearExpression* exp,
                                                double coef) {
  // cout<<"coef:"<<coef<<endl;
  uint x = tx.get_priority();
  foreach(tx.get_requests(), request) {
    uint q = request->get_resource_id();
    ulong length = request->get_max_length();
    foreach_request_instance(ti, tx, q, v) {
      uint var_id;

      var_id = vars->lookup(x, q, v, PIPMapper::BLOCKING_PREEMPT);
      exp->add_term(var_id, coef * length);
    }
  }
}

void LP_RTA_GFP_PIP::lp_pip_OD(const Task& ti, PIPMapper* vars,
                               LinearExpression* exp, double coef) {
  // cout<<"coef:"<<coef<<endl;
  uint p_num = processors.get_processor_num();
  uint var_id;

  foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
    uint h = th->get_priority();

    var_id = vars->lookup(h, 0, 0, PIPMapper::INTERF_REGULAR);
    exp->add_term(var_id, coef * (1.0 / p_num));
  }

  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    uint l = tl->get_priority();

    var_id = vars->lookup(l, 0, 0, PIPMapper::INTERF_CO_BOOSTING);
    exp->add_term(var_id, coef * (1.0 / p_num));

    var_id = vars->lookup(l, 0, 0, PIPMapper::INTERF_STALLING);
    exp->add_term(var_id, coef * (1.0 / p_num));

    lp_pip_indirected_blocking(ti, *tl, vars, exp, coef * (1.0 / p_num));

    lp_pip_preemption_blocking(ti, *tl, vars, exp, coef * (1.0 / p_num));
  }
}

void LP_RTA_GFP_PIP::lp_pip_declare_variable_bounds(const Task& ti,
                                                    LinearProgram* lp,
                                                    PIPMapper* vars) {
  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_priority();
    uint var_id;

    var_id = vars->lookup(x, 0, 0, PIPMapper::INTERF_REGULAR);
    lp->declare_variable_bounds(var_id, true, 0, false, -1);

    var_id = vars->lookup(x, 0, 0, PIPMapper::INTERF_CO_BOOSTING);
    lp->declare_variable_bounds(var_id, true, 0, false, -1);

    var_id = vars->lookup(x, 0, 0, PIPMapper::INTERF_STALLING);
    lp->declare_variable_bounds(var_id, true, 0, false, -1);
  }
}

void LP_RTA_GFP_PIP::lp_pip_objective(const Task& ti, LinearProgram* lp,
                                      PIPMapper* vars, LinearExpression* obj) {
  uint p_num = processors.get_processor_num();
  uint var_id;
  // cout<<"foreachhigher priority task then:"<<ti.get_priority()<<endl;
  foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
    uint h = th->get_priority();
    // cout<<"task:"<<h<<endl;
    var_id = vars->lookup(h, 0, 0, PIPMapper::INTERF_REGULAR);
    obj->add_term(var_id, 1.0 / p_num);
  }
  // cout<<"foreachlower priority task then:"<<ti.get_priority()<<endl;
  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    uint l = tl->get_priority();
    // cout<<"task:"<<l<<endl;
    var_id = vars->lookup(l, 0, 0, PIPMapper::INTERF_CO_BOOSTING);
    obj->add_term(var_id, 1.0 / p_num);

    var_id = vars->lookup(l, 0, 0, PIPMapper::INTERF_STALLING);
    obj->add_term(var_id, 1.0 / p_num);

    lp_pip_indirected_blocking(ti, *tl, vars, obj, 1.0 / p_num);

    lp_pip_preemption_blocking(ti, *tl, vars, obj, 1.0 / p_num);
  }

  foreach_task_except(tasks.get_tasks(), ti, tx) {
    lp_pip_directed_blocking(ti, *tx, vars, obj, 1.0);
  }

  // vars->seal();
}

void LP_RTA_GFP_PIP::lp_pip_add_constraints(const Task& ti, LinearProgram* lp,
                                            PIPMapper* vars) {
  lp_pip_constraint_1(ti, lp, vars);
  lp_pip_constraint_2(ti, lp, vars);
  lp_pip_constraint_3(ti, lp, vars);
  lp_pip_constraint_4(ti, lp, vars);
  lp_pip_constraint_5(ti, lp, vars);
  vars->seal();
  lp_pip_declare_variable_bounds(ti, lp, vars);
  lp_pip_constraint_6(ti, lp, vars);
  lp_pip_constraint_7(ti, lp, vars);
  lp_pip_constraint_8(ti, lp, vars);
  lp_pip_constraint_9(ti, lp, vars);
  lp_pip_constraint_10(ti, lp, vars);
  lp_pip_constraint_11(ti, lp, vars);
}

void LP_RTA_GFP_PIP::lp_pip_constraint_1(const Task& ti, LinearProgram* lp,
                                         PIPMapper* vars) {
  foreach_task_except(tasks.get_tasks(), ti, tx) {
    LinearExpression* exp = new LinearExpression();

    uint var_id;
    uint x = tx->get_priority();

    var_id = vars->lookup(x, 0, 0, PIPMapper::INTERF_REGULAR);
    exp->add_var(var_id);

    var_id = vars->lookup(x, 0, 0, PIPMapper::INTERF_CO_BOOSTING);
    exp->add_var(var_id);

    var_id = vars->lookup(x, 0, 0, PIPMapper::INTERF_STALLING);
    exp->add_var(var_id);

    lp_pip_directed_blocking(ti, *tx, vars, exp);

    lp_pip_indirected_blocking(ti, *tx, vars, exp);

    lp_pip_preemption_blocking(ti, *tx, vars, exp);

    lp->add_inequality(exp, workload_bound(*tx, ti.get_response_time()));
  }
}

void LP_RTA_GFP_PIP::lp_pip_constraint_2(const Task& ti, LinearProgram* lp,
                                         PIPMapper* vars) {
  foreach_task_except(tasks.get_tasks(), ti, tx) {
    LinearExpression* exp = new LinearExpression();
    uint var_id;
    uint x = tx->get_priority();

    var_id = vars->lookup(x, 0, 0, PIPMapper::INTERF_REGULAR);
    exp->add_var(var_id);

    var_id = vars->lookup(x, 0, 0, PIPMapper::INTERF_CO_BOOSTING);
    exp->add_var(var_id);

    var_id = vars->lookup(x, 0, 0, PIPMapper::INTERF_STALLING);
    exp->add_var(var_id);

    lp_pip_indirected_blocking(ti, *tx, vars, exp);

    lp_pip_preemption_blocking(ti, *tx, vars, exp);

    lp_pip_OD(ti, vars, exp, -1);

    lp->add_inequality(exp, 0);
  }
}

void LP_RTA_GFP_PIP::lp_pip_constraint_3(const Task& ti, LinearProgram* lp,
                                         PIPMapper* vars) {
  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_priority();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      foreach_request_instance(ti, *tx, q, v) {
        LinearExpression* exp = new LinearExpression();
        uint var_id;

        var_id = vars->lookup(x, q, v, PIPMapper::BLOCKING_DIRECT);
        exp->add_var(var_id);

        var_id = vars->lookup(x, q, v, PIPMapper::BLOCKING_INDIRECT);
        exp->add_var(var_id);

        var_id = vars->lookup(x, q, v, PIPMapper::BLOCKING_PREEMPT);
        exp->add_var(var_id);

        lp->add_inequality(exp, 1);
      }
    }
  }
}

void LP_RTA_GFP_PIP::lp_pip_constraint_4(const Task& ti, LinearProgram* lp,
                                         PIPMapper* vars) {
  if (0 == ti.get_requests().size()) {
    foreach_lower_priority_task(tasks.get_tasks(), ti, tx) {
      LinearExpression* exp = new LinearExpression();
      uint x = tx->get_priority();
      uint var_id;

      var_id = vars->lookup(x, 0, 0, PIPMapper::INTERF_STALLING);
      exp->add_var(var_id);

      lp->add_equality(exp, 0);
    }
  }
}

void LP_RTA_GFP_PIP::lp_pip_constraint_5(const Task& ti, LinearProgram* lp,
                                         PIPMapper* vars) {
  LinearExpression* exp = new LinearExpression();

  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_priority();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      if (!ti.is_request_exist(q)) {
        foreach_request_instance(ti, *tx, q, v) {
          uint var_id;

          var_id = vars->lookup(x, q, v, PIPMapper::BLOCKING_DIRECT);
          exp->add_var(var_id);
        }
      }
    }
  }

  lp->add_equality(exp, 0);
}

void LP_RTA_GFP_PIP::lp_pip_constraint_6(const Task& ti, LinearProgram* lp,
                                         PIPMapper* vars) {
  LinearExpression* exp = new LinearExpression();

  foreach_lower_priority_task(tasks.get_tasks(), ti, tx) {
    uint var_id;
    uint x = tx->get_priority();

    var_id = vars->lookup(x, 0, 0, PIPMapper::INTERF_CO_BOOSTING);
    exp->add_var(var_id);
  }

  lp->add_equality(exp, 0);
}

void LP_RTA_GFP_PIP::lp_pip_constraint_7(const Task& ti, LinearProgram* lp,
                                         PIPMapper* vars) {
  uint p_num = processors.get_processor_num();

  if (p_num >= ti.get_priority()) {
    foreach_task_except(tasks.get_tasks(), ti, tx) {
      LinearExpression* exp = new LinearExpression();

      uint var_id;
      uint x = tx->get_priority();

      var_id = vars->lookup(x, 0, 0, PIPMapper::INTERF_REGULAR);
      exp->add_var(var_id);

      var_id = vars->lookup(x, 0, 0, PIPMapper::INTERF_CO_BOOSTING);
      exp->add_var(var_id);

      var_id = vars->lookup(x, 0, 0, PIPMapper::INTERF_STALLING);
      exp->add_var(var_id);

      lp_pip_indirected_blocking(ti, *tx, vars, exp);

      lp_pip_preemption_blocking(ti, *tx, vars, exp);

      lp->add_equality(exp, 0);
    }
  }
}

void LP_RTA_GFP_PIP::lp_pip_constraint_8(const Task& ti, LinearProgram* lp,
                                         PIPMapper* vars) {
  foreach_lower_priority_task(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_priority();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      uint N_i_q;
      if (ti.is_request_exist(q))
        N_i_q = ti.get_request_by_id(q).get_num_requests();
      else
        N_i_q = 0;

      LinearExpression* exp = new LinearExpression();
      foreach_request_instance(ti, *tx, q, v) {
        uint var_id;

        var_id = vars->lookup(x, q, v, PIPMapper::BLOCKING_DIRECT);
        exp->add_var(var_id);
      }

      lp->add_inequality(exp, N_i_q);
    }
  }
}

void LP_RTA_GFP_PIP::lp_pip_constraint_9(const Task& ti, LinearProgram* lp,
                                         PIPMapper* vars) {
  foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_priority();
    foreach(resources.get_resources(), resource) {
      uint q = resource->get_resource_id();
      uint N_i_q;
      ulong W_i_q;

      if (ti.is_request_exist(q)) {
        N_i_q = ti.get_request_by_id(q).get_num_requests();

        W_i_q = wait_time_bound(ti, q);
        if (MAX_LONG == W_i_q) {
          continue;
        }

      } else {
        N_i_q = 0;
        W_i_q = 0;
      }

      if (tx->is_request_exist(q)) {
        LinearExpression* exp = new LinearExpression();

        uint N_x_q = tx->get_request_by_id(q).get_num_requests();

        ulong bound = N_i_q * (tx->get_max_job_num(W_i_q)) * N_x_q;

        foreach_request_instance(ti, *tx, q, v) {
          uint var_id;

          var_id = vars->lookup(x, q, v, PIPMapper::BLOCKING_DIRECT);
          exp->add_var(var_id);
        }

        lp->add_inequality(exp, bound);
      }
    }
  }
}

void LP_RTA_GFP_PIP::lp_pip_constraint_10(const Task& ti, LinearProgram* lp,
                                          PIPMapper* vars) {
  LinearExpression* exp = new LinearExpression();

  foreach_lower_priority_task(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_priority();
    uint var_id;

    var_id = vars->lookup(x, 0, 0, PIPMapper::INTERF_STALLING);
    exp->add_var(var_id);
  }
  lp->add_equality(exp, 0);
}

void LP_RTA_GFP_PIP::lp_pip_constraint_11(const Task& ti, LinearProgram* lp,
                                          PIPMapper* vars) {
  ulong bound = 0;
  ulong R_i = ti.get_response_time();

  foreach(resources.get_resources(), resource) {
    LinearExpression* exp = new LinearExpression();
    uint q = resource->get_resource_id();

    foreach_lower_priority_task(tasks.get_tasks(), ti, tx) {
      uint x = tx->get_priority();
      if (tx->is_request_exist(q)) {
        uint x = tx->get_priority();

        foreach_request_instance(ti, *tx, q, v) {
          uint var_id;

          var_id = vars->lookup(x, q, v, PIPMapper::BLOCKING_INDIRECT);
          exp->add_var(var_id);

          var_id = vars->lookup(x, q, v, PIPMapper::BLOCKING_PREEMPT);
          exp->add_var(var_id);
        }
      } else {
        // delete exp;
        continue;
      }
    }

    if (!exp->has_terms()) {
      delete exp;
      continue;
    }

    ulong bound = 0;

    foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
      bound += th->get_max_request_num(q, R_i);
    }

    lp->add_inequality(exp, bound);
  }
}
