// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <lp.h>
#include <lp_rta_dag_gfp_fmlp.h>
#include <math-helper.h>
#include <processors.h>
#include <resources.h>
#include <solution.h>
#include <tasks.h>
#include <iostream>
#include <sstream>

using std::min;
using std::ostringstream;

/** Class DAG_FMLPMapper */

uint64_t DAG_FMLPMapper::encode_request(uint64_t task_id, uint64_t res_id,
                                    uint64_t req_id, uint64_t type) {
  uint64_t one = 1;
  uint64_t key = 0;

  // cout << "task_id " << task_id << endl;
  // cout << "res_id " << res_id << endl;
  // cout << "req_id " << req_id << endl;
  // cout << "type " << type << endl;
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

uint64_t DAG_FMLPMapper::get_type(uint64_t var) {
  return (var >> 30) & (uint64_t)0x7;  // 3 bits
}

uint64_t DAG_FMLPMapper::get_task(uint64_t var) {
  return (var >> 20) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t DAG_FMLPMapper::get_res_id(uint64_t var) {
  return (var >> 16) & (uint64_t)0xf;  // 4 bits
}

uint64_t DAG_FMLPMapper::get_req_id(uint64_t var) {
  return var & (uint64_t)0xffff;  // 16 bits
}

DAG_FMLPMapper::DAG_FMLPMapper(uint start_var) : VarMapperBase(start_var) {}

uint DAG_FMLPMapper::lookup(uint task_id, uint res_id, uint req_id,
                            var_type type) {
  uint64_t key = encode_request(task_id, res_id, req_id, type);
  uint var = var_for_key(key);
  return var;
}

string DAG_FMLPMapper::key2str(uint64_t key, uint var) const {
  ostringstream buf;

  switch (get_type(key)) {
    case DAG_FMLPMapper::BLOCKING_DIRECT:
      buf << "Xd[";
      break;
    case DAG_FMLPMapper::BLOCKING_INDIRECT:
      buf << "Xi[";
      break;
    case DAG_FMLPMapper::BLOCKING_PREEMPT:
      buf << "Xp[";
      break;
    case DAG_FMLPMapper::BLOCKING_OTHER:
      buf << "Xo[";
      break;
    case DAG_FMLPMapper::INTERF_REGULAR:
      buf << "Ir[";
      break;
    case DAG_FMLPMapper::INTERF_CO_BOOSTING:
      buf << "Ic[";
      break;
    case DAG_FMLPMapper::INTERF_STALLING:
      buf << "Is[";
      break;
    case DAG_FMLPMapper::INTERF_OTHER:
      buf << "Io[";
      break;
    default:
      buf << "?[";
  }

  buf << get_task(key) << ", " << get_res_id(key) << ", " << get_req_id(key)
      << "]";

  return buf.str();
}

/** Class LP_RTA_DAG_GFP_FMLP */

LP_RTA_DAG_GFP_FMLP::LP_RTA_DAG_GFP_FMLP()
    : GlobalSched(true, RTA, FIX_PRIORITY, FMLP, "", "FMLP") {}

LP_RTA_DAG_GFP_FMLP::LP_RTA_DAG_GFP_FMLP(DAG_TaskSet tasks,
                                         ProcessorSet processors,
                                         ResourceSet resources)
    : GlobalSched(true, RTA, FIX_PRIORITY, FMLP, "", "FMLP") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;

  this->resources.update(&(this->tasks));
  this->processors.update(&(this->tasks), &(this->resources));

  this->tasks.RM_Order();
  this->processors.init();
}

LP_RTA_DAG_GFP_FMLP::~LP_RTA_DAG_GFP_FMLP() {}
bool LP_RTA_DAG_GFP_FMLP::is_schedulable() {
  bool update = false;

  foreach(tasks.get_tasks(), ti) {
    if (ti->get_deadline() < ti->get_wcet())
      return false;
  }

  do {
    update = false;
    foreach(tasks.get_tasks(), ti) {
      ulong old_response_time = ti->get_response_time();
      ulong response_time = fmlp_get_response_time(*ti);

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

ulong LP_RTA_DAG_GFP_FMLP::fmlp_get_response_time(const DAG_Task& ti) {
  ulong response_time = 0;
  
  if (ti.get_wcet() > ti.get_deadline())
    return ti.get_deadline() + 100;

  DAG_FMLPMapper vars;
  LinearProgram response_bound;
  LinearExpression* obj = new LinearExpression();

  lp_fmlp_objective(ti, &response_bound, &vars, obj);

  response_bound.set_objective(obj);

  lp_fmlp_add_constraints(ti, &response_bound, &vars);

  // vars->seal();

  GLPKSolution* rb_solution =
      new GLPKSolution(response_bound, vars.get_num_vars());

  assert(rb_solution != NULL);

  if (rb_solution->is_solved()) {
    // cout<<"solved."<<endl;
    assert(ti.get_response_time() >= ti.get_wcet());
    ulong gap = ti.get_response_time() - ti.get_wcet();
    double result = rb_solution->evaluate(*(response_bound.get_objective()));
    // cout<<"original gap:"<<gap<<endl;
    // cout<<"lp result:"<<result<<endl;

    if ((result < gap) && (gap - result < _EPS)) {
      response_time = ti.get_response_time();
      // cout<<"bingo!!!"<<endl;
    } else {
      response_time = result + ti.get_wcet();
      assert(response_time < MAX_LONG);
    }
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

ulong LP_RTA_DAG_GFP_FMLP::fmlp_workload_bound(const DAG_Task& tx, ulong Ri) {
  ulong e = tx.get_wcet();
  ulong d = tx.get_deadline();
  ulong p = tx.get_period();
  ulong r = tx.get_response_time();
  assert(d >= r);

  ulong N = (Ri - e + r) / p;

  return N * e + min(e, Ri + r - e - N * p);
}
/*
ulong LP_RTA_DAG_GFP_FMLP::fmlp_holding_bound(const DAG_Task& ti, Task& tx, uint
resource_id)
{

        uint x = tx.get_priority(), q = resource_id;
        uint p_num = processors.get_processor_num();
        ulong L_x_q = tx.get_request_by_id(q).get_max_length();
        ulong holding_time = L_x_q;

        if(p_num < tx.get_priority())
        {
                uint y = min(ti.get_priority(), tx.get_priority());
                uint z = max(ti.get_priority(), tx.get_priority());
                bool update;
                do
                {
                        update = false;
                        ulong temp = 0;

                        foreach_higher_priority_task_then(tasks.get_tasks(), y,
th)
                        {
                                temp += fmlp_workload_bound(*th, holding_time);
                        }

                        foreach_lower_priority_task_then(tasks.get_tasks(), y,
tl)
                        {
                                uint l = tl->get_priority();
                                if(z != l)
                                {
                                        foreach(tl->get_requests(), request)
                                        {
                                                uint u =
request->get_resource_id(); Resource& resource = resources.get_resource_by_id(u);
                                                if(y > resource.get_ceiling())
                                                {
                                                        uint N_l_u =
request->get_num_requests(); uint L_l_u = request->get_max_length(); temp +=
tl->get_max_job_num(holding_time)*N_l_u*L_l_u;
                                                }
                                        }
                                }
                        }
                        temp /= p_num;
                        temp += L_x_q;
                        assert(temp >= holding_time);
                        if(temp > ti.get_deadline())
                                return MAX_LONG;

                        if(temp > holding_time)
                        {
                                update = true;
                                holding_time = temp;
                        }
                }
                while(update);
        }

        return holding_time;
}

ulong LP_RTA_DAG_GFP_FMLP::fmlp_wait_time_bound(const DAG_Task& ti, uint resource_id)
{
        ulong wait_time = 0;
        ulong holding_time_l = 0;

        foreach_lower_priority_task(tasks.get_tasks(), ti, tl)
        {

                ulong temp = 0;

                if(tl->is_request_exist(resource_id))
                        temp = fmlp_holding_bound(ti, *tl, resource_id);

                if(temp > holding_time_l)
                        holding_time_l = temp;
        }

        if(holding_time_l == MAX_LONG)
        {
//cout<<"holding_time_l:"<<holding_time_l<<endl;
                return MAX_LONG;
        }

        bool update;
        do
        {
                update = false;
                ulong temp = 1 + holding_time_l;

                foreach_higher_priority_task(tasks.get_tasks(), ti, th)
                {
                        if(th->is_request_exist(resource_id))
                        {
                                uint N_h_q =
th->get_request_by_id(resource_id).get_num_requests(); ulong H_h_q =
fmlp_holding_bound(ti, *th, resource_id); temp +=
th->get_max_job_num(wait_time)*N_h_q *H_h_q;
                        }
                }

                if(temp > ti.get_deadline())
                {
//cout<<"temp > ti.get_deadline():"<<temp<<" > "<<ti.get_deadline()<<endl;
                        return MAX_LONG;
                }

//cout<<"temp:"<<temp<<endl;
//cout<<"wait_time:"<<wait_time<<endl;
                //assert(temp >= wait_time);

                if(temp != wait_time)
                {
                        update = true;
                        wait_time = temp;
                }
        }
        while(update);

        return wait_time;
}
*/
/** Expressions */
void LP_RTA_DAG_GFP_FMLP::lp_fmlp_directed_blocking(const DAG_Task& ti, const DAG_Task& tx,
                                                DAG_FMLPMapper* vars,
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

      var_id = vars->lookup(x, q, v, DAG_FMLPMapper::BLOCKING_DIRECT);
      exp->add_term(var_id, coef * length);
    }
  }
}

void LP_RTA_DAG_GFP_FMLP::lp_fmlp_indirected_blocking(const DAG_Task& ti,
                                                  const DAG_Task& tx,
                                                  DAG_FMLPMapper* vars,
                                                  LinearExpression* exp,
                                                  double coef) {
  // cout<<"coef:"<<coef<<endl;
  uint x = tx.get_priority();
  foreach(tx.get_requests(), request) {
    uint q = request->get_resource_id();
    ulong length = request->get_max_length();
    foreach_request_instance(ti, tx, q, v) {
      uint var_id;

      var_id = vars->lookup(x, q, v, DAG_FMLPMapper::BLOCKING_INDIRECT);
      exp->add_term(var_id, coef * length);
    }
  }
}

void LP_RTA_DAG_GFP_FMLP::lp_fmlp_preemption_blocking(const DAG_Task& ti,
                                                  const DAG_Task& tx,
                                                  DAG_FMLPMapper* vars,
                                                  LinearExpression* exp,
                                                  double coef) {
  // cout<<"coef:"<<coef<<endl;
  uint x = tx.get_priority();
  foreach(tx.get_requests(), request) {
    uint q = request->get_resource_id();
    ulong length = request->get_max_length();
    foreach_request_instance(ti, tx, q, v) {
      uint var_id;

      var_id = vars->lookup(x, q, v, DAG_FMLPMapper::BLOCKING_PREEMPT);
      exp->add_term(var_id, coef * length);
    }
  }
}

void LP_RTA_DAG_GFP_FMLP::lp_fmlp_OD(const DAG_Task& ti, DAG_FMLPMapper* vars,
                                 LinearExpression* exp, double coef) {
  // cout<<"coef:"<<coef<<endl;
  uint p_num = processors.get_processor_num();
  uint var_id;

  foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
    uint h = th->get_priority();

    var_id = vars->lookup(h, 0, 0, DAG_FMLPMapper::INTERF_REGULAR);
    exp->add_term(var_id, coef * (1.0 / p_num));
  }

  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    uint l = tl->get_priority();

    var_id = vars->lookup(l, 0, 0, DAG_FMLPMapper::INTERF_CO_BOOSTING);
    exp->add_term(var_id, coef * (1.0 / p_num));

    var_id = vars->lookup(l, 0, 0, DAG_FMLPMapper::INTERF_STALLING);
    exp->add_term(var_id, coef * (1.0 / p_num));

    lp_fmlp_indirected_blocking(ti, *tl, vars, exp, coef * (1.0 / p_num));

    lp_fmlp_preemption_blocking(ti, *tl, vars, exp, coef * (1.0 / p_num));
  }
}

void LP_RTA_DAG_GFP_FMLP::lp_fmlp_declare_variable_bounds(const DAG_Task& ti,
                                                      LinearProgram* lp,
                                                      DAG_FMLPMapper* vars) {
  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_priority();
    uint var_id;

    var_id = vars->lookup(x, 0, 0, DAG_FMLPMapper::INTERF_REGULAR);
    lp->declare_variable_bounds(var_id, true, 0, false, -1);

    var_id = vars->lookup(x, 0, 0, DAG_FMLPMapper::INTERF_CO_BOOSTING);
    lp->declare_variable_bounds(var_id, true, 0, false, -1);

    var_id = vars->lookup(x, 0, 0, DAG_FMLPMapper::INTERF_STALLING);
    lp->declare_variable_bounds(var_id, true, 0, false, -1);
  }
}

void LP_RTA_DAG_GFP_FMLP::lp_fmlp_objective(const DAG_Task& ti, LinearProgram* lp,
                                        DAG_FMLPMapper* vars,
                                        LinearExpression* obj) {
  uint p_num = processors.get_processor_num();
  uint var_id;
  // cout<<"foreachhigher priority task then:"<<ti.get_priority()<<endl;
  foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
    uint h = th->get_priority();
    // cout<<"task:"<<h<<endl;
    var_id = vars->lookup(h, 0, 0, DAG_FMLPMapper::INTERF_REGULAR);
    obj->add_term(var_id, 1.0 / p_num);
  }
  // cout<<"foreachlower priority task then:"<<ti.get_priority()<<endl;
  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    uint l = tl->get_priority();
    // cout<<"task:"<<l<<endl;
    var_id = vars->lookup(l, 0, 0, DAG_FMLPMapper::INTERF_CO_BOOSTING);
    obj->add_term(var_id, 1.0 / p_num);

    var_id = vars->lookup(l, 0, 0, DAG_FMLPMapper::INTERF_STALLING);
    obj->add_term(var_id, 1.0 / p_num);

    lp_fmlp_indirected_blocking(ti, *tl, vars, obj, 1.0 / p_num);

    lp_fmlp_preemption_blocking(ti, *tl, vars, obj, 1.0 / p_num);
  }

  foreach_task_except(tasks.get_tasks(), ti, tx) {
    lp_fmlp_directed_blocking(ti, *tx, vars, obj, 1.0);
  }

  // vars->seal();
}

void LP_RTA_DAG_GFP_FMLP::lp_fmlp_add_constraints(const DAG_Task& ti, LinearProgram* lp,
                                              DAG_FMLPMapper* vars) {
  lp_fmlp_constraint_1(ti, lp, vars);
  lp_fmlp_constraint_2(ti, lp, vars);
  lp_fmlp_constraint_3(ti, lp, vars);
  lp_fmlp_constraint_4(ti, lp, vars);
  lp_fmlp_constraint_5(ti, lp, vars);
  vars->seal();
  lp_fmlp_declare_variable_bounds(ti, lp, vars);
  lp_fmlp_constraint_6(ti, lp, vars);
  lp_fmlp_constraint_7(ti, lp, vars);
  lp_fmlp_constraint_8(ti, lp, vars);
  lp_fmlp_constraint_9(ti, lp, vars);
  lp_fmlp_constraint_10(ti, lp, vars);
}

void LP_RTA_DAG_GFP_FMLP::lp_fmlp_constraint_1(const DAG_Task& ti, LinearProgram* lp,
                                           DAG_FMLPMapper* vars) {
  foreach_task_except(tasks.get_tasks(), ti, tx) {
    LinearExpression* exp = new LinearExpression();

    uint var_id;
    uint x = tx->get_priority();

    var_id = vars->lookup(x, 0, 0, DAG_FMLPMapper::INTERF_REGULAR);
    exp->add_var(var_id);

    var_id = vars->lookup(x, 0, 0, DAG_FMLPMapper::INTERF_CO_BOOSTING);
    exp->add_var(var_id);

    var_id = vars->lookup(x, 0, 0, DAG_FMLPMapper::INTERF_STALLING);
    exp->add_var(var_id);

    lp_fmlp_directed_blocking(ti, *tx, vars, exp);

    lp_fmlp_indirected_blocking(ti, *tx, vars, exp);

    lp_fmlp_preemption_blocking(ti, *tx, vars, exp);

    lp->add_inequality(exp, fmlp_workload_bound(*tx, ti.get_response_time()));
  }
}

void LP_RTA_DAG_GFP_FMLP::lp_fmlp_constraint_2(const DAG_Task& ti, LinearProgram* lp,
                                           DAG_FMLPMapper* vars) {
  foreach_task_except(tasks.get_tasks(), ti, tx) {
    LinearExpression* exp = new LinearExpression();
    uint var_id;
    uint x = tx->get_priority();

    var_id = vars->lookup(x, 0, 0, DAG_FMLPMapper::INTERF_REGULAR);
    exp->add_var(var_id);

    var_id = vars->lookup(x, 0, 0, DAG_FMLPMapper::INTERF_CO_BOOSTING);
    exp->add_var(var_id);

    var_id = vars->lookup(x, 0, 0, DAG_FMLPMapper::INTERF_STALLING);
    exp->add_var(var_id);

    lp_fmlp_indirected_blocking(ti, *tx, vars, exp);

    lp_fmlp_preemption_blocking(ti, *tx, vars, exp);

    lp_fmlp_OD(ti, vars, exp, -1);

    lp->add_inequality(exp, 0);
  }
}

void LP_RTA_DAG_GFP_FMLP::lp_fmlp_constraint_3(const DAG_Task& ti, LinearProgram* lp,
                                           DAG_FMLPMapper* vars) {
  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_priority();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      foreach_request_instance(ti, *tx, q, v) {
        LinearExpression* exp = new LinearExpression();
        uint var_id;

        var_id = vars->lookup(x, q, v, DAG_FMLPMapper::BLOCKING_DIRECT);
        exp->add_var(var_id);

        var_id = vars->lookup(x, q, v, DAG_FMLPMapper::BLOCKING_INDIRECT);
        exp->add_var(var_id);

        var_id = vars->lookup(x, q, v, DAG_FMLPMapper::BLOCKING_PREEMPT);
        exp->add_var(var_id);

        lp->add_inequality(exp, 1);
      }
    }
  }
}

void LP_RTA_DAG_GFP_FMLP::lp_fmlp_constraint_4(const DAG_Task& ti, LinearProgram* lp,
                                           DAG_FMLPMapper* vars) {
  if (0 == ti.get_requests().size()) {
    foreach_lower_priority_task(tasks.get_tasks(), ti, tx) {
      LinearExpression* exp = new LinearExpression();
      uint x = tx->get_priority();
      uint var_id;

      var_id = vars->lookup(x, 0, 0, DAG_FMLPMapper::INTERF_STALLING);
      exp->add_var(var_id);

      lp->add_equality(exp, 0);
    }
  }
}

void LP_RTA_DAG_GFP_FMLP::lp_fmlp_constraint_5(const DAG_Task& ti, LinearProgram* lp,
                                           DAG_FMLPMapper* vars) {
  LinearExpression* exp = new LinearExpression();

  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_priority();
    foreach(tx->get_requests(), request) {
      uint q = request->get_resource_id();
      if (!ti.is_request_exist(q)) {
        foreach_request_instance(ti, *tx, q, v) {
          uint var_id;

          var_id = vars->lookup(x, q, v, DAG_FMLPMapper::BLOCKING_DIRECT);
          exp->add_var(var_id);
        }
      }
    }
  }

  lp->add_equality(exp, 0);
}

void LP_RTA_DAG_GFP_FMLP::lp_fmlp_constraint_6(const DAG_Task& ti, LinearProgram* lp,
                                           DAG_FMLPMapper* vars) {
  LinearExpression* exp = new LinearExpression();

  foreach_lower_priority_task(tasks.get_tasks(), ti, tx) {
    uint var_id;
    uint x = tx->get_priority();

    var_id = vars->lookup(x, 0, 0, DAG_FMLPMapper::INTERF_CO_BOOSTING);
    exp->add_var(var_id);
  }

  lp->add_equality(exp, 0);
}

void LP_RTA_DAG_GFP_FMLP::lp_fmlp_constraint_7(const DAG_Task& ti, LinearProgram* lp,
                                           DAG_FMLPMapper* vars) {
  uint p_num = processors.get_processor_num();

  if (p_num >= ti.get_priority()) {
    foreach_task_except(tasks.get_tasks(), ti, tx) {
      LinearExpression* exp = new LinearExpression();

      uint var_id;
      uint x = tx->get_priority();

      var_id = vars->lookup(x, 0, 0, DAG_FMLPMapper::INTERF_REGULAR);
      exp->add_var(var_id);

      var_id = vars->lookup(x, 0, 0, DAG_FMLPMapper::INTERF_CO_BOOSTING);
      exp->add_var(var_id);

      var_id = vars->lookup(x, 0, 0, DAG_FMLPMapper::INTERF_STALLING);
      exp->add_var(var_id);

      lp_fmlp_indirected_blocking(ti, *tx, vars, exp);

      lp_fmlp_preemption_blocking(ti, *tx, vars, exp);

      lp->add_equality(exp, 0);
    }
  }
}

void LP_RTA_DAG_GFP_FMLP::lp_fmlp_constraint_8(const DAG_Task& ti, LinearProgram* lp,
                                           DAG_FMLPMapper* vars) {
  foreach_task_except(tasks.get_tasks(), ti, tx) {
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

        var_id = vars->lookup(x, q, v, DAG_FMLPMapper::BLOCKING_DIRECT);
        exp->add_var(var_id);
      }

      lp->add_inequality(exp, N_i_q);
    }
  }
}

void LP_RTA_DAG_GFP_FMLP::lp_fmlp_constraint_9(const DAG_Task& ti, LinearProgram* lp,
                                           DAG_FMLPMapper* vars) {
  LinearExpression* exp = new LinearExpression();

  foreach_lower_priority_task(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_priority();
    uint var_id;

    var_id = vars->lookup(x, 0, 0, DAG_FMLPMapper::INTERF_STALLING);
    exp->add_var(var_id);
  }
  lp->add_equality(exp, 0);
}

void LP_RTA_DAG_GFP_FMLP::lp_fmlp_constraint_10(const DAG_Task& ti, LinearProgram* lp,
                                            DAG_FMLPMapper* vars) {
  ulong R_i = ti.get_response_time();

  foreach(resources.get_resources(), resource) {
    uint q = resource->get_resource_id();

    foreach_lower_priority_task(tasks.get_tasks(), ti, tx) {
      uint x = tx->get_priority();

      if (tx->is_request_exist(q)) {
        uint x = tx->get_priority();
        LinearExpression* exp = new LinearExpression();

        foreach_request_instance(ti, *tx, q, v) {
          uint var_id;

          var_id = vars->lookup(x, q, v, DAG_FMLPMapper::BLOCKING_INDIRECT);
          exp->add_var(var_id);

          var_id = vars->lookup(x, q, v, DAG_FMLPMapper::BLOCKING_PREEMPT);
          exp->add_var(var_id);
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
  }
}
