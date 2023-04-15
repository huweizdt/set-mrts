// Copyright [2021] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <lp.h>
#include <lp_rta_gfp_spin.h>
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

/** Class GFP_SPINMapper */

uint64_t GFP_SPINMapper::encode_request(uint64_t task_1, uint64_t request_1,
                                        uint64_t task_2, uint64_t request_2, 
                                        uint64_t type) {
  uint64_t one = 1;
  uint64_t key = 0;
  // cout << "task_1:" << task_1 << endl;
  // cout << "task_2:" << task_2 << endl;
  // cout << "request_1:" << request_1 << endl;
  // cout << "request_2:" << request_2 << endl;
  assert(task_1 < (one << 5));
  assert(task_2 < (one << 5));
  assert(request_1 < (one << 10));
  assert(request_2 < (one << 10));
  assert(type < (one << 3));

  key |= (type << 30);
  key |= (task_1 << 25);
  key |= (request_1 << 15);
  key |= (task_2 << 10);
  key |= request_2;
  return key;
}

uint64_t GFP_SPINMapper::get_type(uint64_t var) {
  return (var >> 30) & (uint64_t)0x7;  // 3 bits
}

uint64_t GFP_SPINMapper::get_task_1(uint64_t var) {
  return (var >> 25) & (uint64_t)0x1f;  // 5 bits
}

uint64_t GFP_SPINMapper::get_task_2(uint64_t var) {
  return (var >> 10) & (uint64_t)0x1f;  // 5 bits
}

uint64_t GFP_SPINMapper::get_req_1(uint64_t var) {
  return (var >> 15) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t GFP_SPINMapper::get_req_2(uint64_t var) {
  return (var) & (uint64_t)0x3ff;  // 10 bits
}

GFP_SPINMapper::GFP_SPINMapper(uint start_var) : VarMapperBase(start_var) {}

uint GFP_SPINMapper::lookup(uint task_1, uint request_1, uint task_2, uint request_2, var_type type) {
  // cout << "x,u,y,v:" << task_1 << "," << request_1 << "," << task_2 << "," << request_2 << endl;
  uint64_t key = encode_request(task_1, request_1, task_2, request_2, type);
  uint var = var_for_key(key);
  return var;
}

string GFP_SPINMapper::key2str(uint64_t key, uint var) const {
  ostringstream buf;

  switch (get_type(key)) {
    case GFP_SPINMapper::BLOCKING_SPIN:
      buf << "B^S[";
      break;
    case GFP_SPINMapper::BLOCKING_TRANS:
      buf << "B^T[";
      break;
    default:
      buf << "?[";
  }

  buf << get_task_1(key) << ", " << get_req_1(key) << ", " << get_task_2(key) << ", " << get_req_2(key)
      << "]";

  return buf.str();
}

/** Class LP_RTA_GFP_SPIN_FIFO */

LP_RTA_GFP_SPIN_FIFO::LP_RTA_GFP_SPIN_FIFO()
    : RTA_DAG_GFP_JOSE() {}

LP_RTA_GFP_SPIN_FIFO::LP_RTA_GFP_SPIN_FIFO(DAG_TaskSet tasks, ProcessorSet processors,
                                 ResourceSet resources)
    : RTA_DAG_GFP_JOSE(tasks, processors, resources) {}

LP_RTA_GFP_SPIN_FIFO::~LP_RTA_GFP_SPIN_FIFO() {}

bool LP_RTA_GFP_SPIN_FIFO::is_schedulable() {
  bool update = false;

  foreach(tasks.get_tasks(), ti) {
    ti->set_response_time(ti->get_critical_path_length());
  }

  foreach(tasks.get_tasks(), ti) {
    ulong response_time = get_response_time(*ti);

    if (response_time <= ti->get_deadline()) {
      ti->set_response_time(response_time);
    } else {
      return false;
    }
  }

  // do {
  //   update = false;
  //   foreach(tasks.get_tasks(), ti) {
  //     ulong old_response_time = ti->get_response_time();
  //     ulong response_time = get_response_time(*ti);

  //     if (old_response_time != response_time) update = true;

  //     if (response_time <= ti->get_deadline()) {
  //       ti->set_response_time(response_time);
  //     } else {
  //       return false;
  //     }
  //   }
  // } while (update);

  return true;
}


uint64_t LP_RTA_GFP_SPIN_FIFO::resource_delay(const DAG_Task& ti, uint res_id) {
  uint32_t request_num = ti.get_request_by_id(res_id).get_num_requests();
  uint64_t max_delay = 0;
  uint32_t max_d_x = 0;

  for (uint x = 0; x <= request_num; x++) {
    GFP_SPINMapper vars;
    LinearProgram delay;
    LinearExpression* obj = new LinearExpression();
    objective(ti, res_id, x, &vars, obj);
    delay.set_objective(obj);
    vars.seal();
    add_constraints(ti, res_id, x, &delay, &vars);

    GLPKSolution* rb_solution =
        new GLPKSolution(delay, vars.get_num_vars());
    // cout << "max_var_num:" << vars.get_num_vars() << endl;

    if (rb_solution->is_solved()) {
      double result = rb_solution->evaluate(*(delay.get_objective()));
      if (max_delay < result) {
        max_delay = result;
        max_d_x = x;
      }
      delete rb_solution;
    } else {
      cout << "[LP_RTA_GFP_SPIN_FIFO]unsolved." << endl;
      cout << "LP solver encounters a problem at T[" << ti.get_id() << "] l[" << res_id << "] X[" << x << "]" << endl;
      // tasks.display();
      // rb_solution->show_error();
      delete rb_solution;
      continue;
      // exit(1);
      // return ti.get_deadline();
      // return MAX_LONG;
    }

#if GLPK_MEM_USAGE_CHECK == 1
    int peak;
    glp_mem_usage(NULL, &peak, NULL, NULL);
    cout << "Peak memory usage:" << peak << endl;
#endif
  }
  // cout << "max delay occurs at x = " << max_d_x << " of " << request_num << endl; 
  return max_delay;
}


uint64_t LP_RTA_GFP_SPIN_FIFO::total_resource_delay(const DAG_Task& ti) {
  uint64_t sum = 0;
  foreach(ti.get_requests(), r_i_q) {
    uint res_id = r_i_q->get_resource_id();
    sum += resource_delay(ti, res_id);
  }
  return sum;
}

ulong LP_RTA_GFP_SPIN_FIFO::get_response_time(const DAG_Task& ti) {
  uint64_t C_i = ti.get_wcet();
  uint64_t L_i = ti.get_critical_path_length();
  uint64_t D_i = ti.get_deadline();
  // uint64_t B_i = total_resource_delay(ti);
  uint32_t p_num = processors.get_processor_num();
  // uint64_t test_start = L_i + B_i + (C_i - L_i)/p_num;
  // cout << "total resource delay:" << B_i << endl;
  // cout << "test start:" << test_start << endl;
  uint64_t response_time = L_i + (C_i - L_i)/p_num;

  uint64_t interf = 0;
  foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
    interf += workload_bound((*tx), D_i);
    // interf += workload_bound((*tx), response_time);
  }
  response_time += interf/p_num;

  // cout << "[FIFO]R_nb:" << response_time << endl;

  if (response_time <= D_i) {
    uint64_t B_i = total_resource_delay(ti);
    cout << "[FIFO]TRD:" << B_i << endl;
    response_time += B_i;
  }

  return response_time;
}

uint64_t LP_RTA_GFP_SPIN_FIFO::workload_bound(const DAG_Task& tx, int64_t interval) {
  int64_t L_x = tx.get_critical_path_length();
  int64_t T_x = tx.get_period();
  int64_t W_x = tx.get_wcet();
  int64_t delta_C = interval - max((int64_t)0, (interval - L_x)/ T_x) * T_x;
  int64_t bound = max((int64_t)0, (interval - delta_C)/ T_x) * W_x + (int64_t)get_WC(tx, delta_C);
  return bound;
}

void LP_RTA_GFP_SPIN_FIFO::declare_variable_bounds(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                                      GFP_SPINMapper* vars) {

}

void LP_RTA_GFP_SPIN_FIFO::objective(const DAG_Task& ti, uint res_id, uint res_num, GFP_SPINMapper* vars,
                        LinearExpression* obj) {
  int64_t var_id;
  uint request_num_x, request_num_y;
  double coef = 1.0 / processors.get_processor_num();
// cout << "OBJ" << endl;
  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num_x = res_num;
    else
      request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
    int32_t L_x_q = tx->get_request_by_id(res_id).get_max_length();
    // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request_num_x:" << request_num_x << endl;
    for (uint u = 1; u <= request_num_x; u++) {
      var_id = vars->lookup(x, u, 0, 0, GFP_SPINMapper::BLOCKING_SPIN);
      obj->add_term(var_id, L_x_q);

      foreach(tasks.get_tasks(), ty) {  // for transitive delay
        uint y = ty->get_id();
        if (!ty->is_request_exist(res_id))
          continue;
        if (y == ti.get_id())
          request_num_y = res_num;
        else
          request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());
        for (uint v = 1; v <= request_num_y; v++) {
          double temp = coef * L_x_q;
          // cout << "temp:" << temp << endl;
          var_id = vars->lookup(x, u, y, v, GFP_SPINMapper::BLOCKING_TRANS);
          obj->add_term(var_id, temp);
        }
      }

      if (tx->get_priority() <= ti.get_priority()) {  // to counteract the interference
        double temp_2 = -1.0 * coef * L_x_q;
          // cout << "temp_2:" << temp_2 << endl;
        var_id = vars->lookup(x, u, 0, 0, GFP_SPINMapper::BLOCKING_SPIN);
        obj->add_term(var_id, temp_2);
      }
    }
  }
}

void LP_RTA_GFP_SPIN_FIFO::add_constraints(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                              GFP_SPINMapper* vars) {
  constraint_1(ti, res_id, res_num, lp, vars);
  constraint_2(ti, res_id, res_num, lp, vars);
  constraint_3(ti, res_id, res_num, lp, vars);
  constraint_4(ti, res_id, res_num, lp, vars);
  constraint_5(ti, res_id, res_num, lp, vars);
  constraint_6(ti, res_id, res_num, lp, vars);
}

void LP_RTA_GFP_SPIN_FIFO::constraint_1(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          GFP_SPINMapper* vars) {
  int64_t var_id;
  uint request_num_x, request_num_y;

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num_x = res_num;
    else
      request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
    // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    for (uint u = 1; u <= request_num_x; u++) {
      foreach(tasks.get_tasks(), ty) {
        uint y = ty->get_id();
        if (!ty->is_request_exist(res_id))
          continue;
        if (y == ti.get_id())
          request_num_y = res_num;
        else
          request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());

        for (uint v = 1; v <= request_num_y; v++) {
          LinearExpression *exp = new LinearExpression();

          var_id = vars->lookup(x, u, y, v, GFP_SPINMapper::BLOCKING_TRANS);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(x, u, 0, 0, GFP_SPINMapper::BLOCKING_SPIN);
          exp->add_term(var_id, 1);

          lp->add_inequality(exp, 1);
        }
      }
    }
  }
}

// 
void LP_RTA_GFP_SPIN_FIFO::constraint_2(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          GFP_SPINMapper* vars) {
  int64_t var_id;
  uint request_num_x, request_num_y;

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num_x = res_num;
    else
      request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
    // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    for (uint u = 1; u <= request_num_x; u++) {
      foreach(tasks.get_tasks(), ty) {
        uint y = ty->get_id();
        if (!ty->is_request_exist(res_id))
          continue;
        if (y == ti.get_id())
          request_num_y = res_num;
        else
          request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());

        for (uint v = 1; v <= request_num_y; v++) {
          LinearExpression *exp = new LinearExpression();
          var_id = vars->lookup(x, u, y, v, GFP_SPINMapper::BLOCKING_TRANS);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(y, v, x, u, GFP_SPINMapper::BLOCKING_TRANS);
          exp->add_term(var_id, 1);

          lp->add_inequality(exp, 1);
        }
      }
    }
  }
}

// 
void LP_RTA_GFP_SPIN_FIFO::constraint_3(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          GFP_SPINMapper* vars) {
  int64_t var_id;
  uint request_num_x, request_num_y;

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num_x = res_num;
    else
      request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
    // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    for (uint u = 1; u <= request_num_x; u++) {
      LinearExpression *exp = new LinearExpression();
      var_id = vars->lookup(x, u, x, u, GFP_SPINMapper::BLOCKING_TRANS);
      exp->add_term(var_id, 1);

      lp->add_equality(exp, 0);
    }
  }
}

// 
void LP_RTA_GFP_SPIN_FIFO::constraint_4(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          GFP_SPINMapper* vars) {
  int64_t var_id;
  uint request_num_x, request_num_y;
  uint p_num = processors.get_processor_num();

  // cout << "C4:" << endl;
  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num_x = res_num;
    else
      request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
    // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    for (uint u = 1; u <= request_num_x; u++) {
      LinearExpression *exp = new LinearExpression();

      foreach(tasks.get_tasks(), ty) {
        uint y = ty->get_id();
        if (!ty->is_request_exist(res_id))
          continue;
        if (y == ti.get_id())
          request_num_y = res_num;
        else
          request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());
        for (uint v = 1; v <= request_num_y; v++) {
          var_id = vars->lookup(x, u, y, v, GFP_SPINMapper::BLOCKING_TRANS);
          exp->add_term(var_id, 1);
          // cout << "B^T[" << x << "," << u << "," << y << "," << v << "]" << "+";
        }
      }

      var_id = vars->lookup(x, u, 0, 0, GFP_SPINMapper::BLOCKING_SPIN);
      exp->add_term(var_id, 1);
      // cout << "B^S[" << x << "," << u << "]" << "<=" << p_num - 1 << endl;

      lp->add_inequality(exp, p_num - 1);
    }
  }
}

// 
void LP_RTA_GFP_SPIN_FIFO::constraint_5(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          GFP_SPINMapper* vars) {
  int64_t var_id;
  uint request_num_x, request_num_y;
  uint p_num = processors.get_processor_num();
  // cout << "C5:" << endl;

  foreach(tasks.get_tasks(), ty) {
    uint y = ty->get_id();
    if (!ty->is_request_exist(res_id))
      continue;
    if (y == ti.get_id())
      request_num_y = res_num;
    else
      request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());
    for (uint v = 1; v <= request_num_y; v++) {
      LinearExpression *exp = new LinearExpression();

      foreach(tasks.get_tasks(), tx) {
        uint x = tx->get_id();
        if (!tx->is_request_exist(res_id))
          continue;
        if (x == ti.get_id())
          request_num_x = res_num;
        else
          request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
        // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
        for (uint u = 1; u <= request_num_x; u++) {
          var_id = vars->lookup(x, u, y, v, GFP_SPINMapper::BLOCKING_TRANS);
          exp->add_term(var_id, 1); 
          // if (u < request_num_x)
          //   cout << "B^T[" << x << "," << u << "," << y << "," << v << "]" << "+";
          // else
          //   cout << "B^T[" << x << "," << u << "," << y << "," << v << "]";
        }
        // cout << "+";
      }
      // cout << "<=" << p_num - 1 << endl;
      lp->add_inequality(exp, p_num - 1);
    }
  }
}

// 
void LP_RTA_GFP_SPIN_FIFO::constraint_6(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          GFP_SPINMapper* vars) {
  int64_t var_id;
  uint request_num_x;
  uint p_num = processors.get_processor_num();
  uint N_i_q = ti.get_request_by_id(res_id).get_num_requests();
  // cout << "C6:" << endl;

  LinearExpression *exp = new LinearExpression();

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num_x = res_num;
    else
      request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
    // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    for (uint u = 1; u <= request_num_x; u++) {
      var_id = vars->lookup(x, u, 0, 0, GFP_SPINMapper::BLOCKING_SPIN);
      exp->add_term(var_id, 1);
      // if (u < request_num_x)
      //   cout << "B^S[" << x << "," << u << "]" << "+";
      // else
      //   cout << "B^S[" << x << "," << u << "]";
    }
  }
  // cout << "<=" << (N_i_q - res_num) * (p_num - 1) << endl;
  lp->add_inequality(exp, (N_i_q - res_num) * (p_num - 1));
}



/** Class LP_RTA_GFP_SPIN_PRIO */

LP_RTA_GFP_SPIN_PRIO::LP_RTA_GFP_SPIN_PRIO()
    : RTA_DAG_GFP_JOSE() {}

LP_RTA_GFP_SPIN_PRIO::LP_RTA_GFP_SPIN_PRIO(DAG_TaskSet tasks, ProcessorSet processors,
                                 ResourceSet resources)
    : RTA_DAG_GFP_JOSE(tasks, processors, resources) {}

LP_RTA_GFP_SPIN_PRIO::~LP_RTA_GFP_SPIN_PRIO() {}

bool LP_RTA_GFP_SPIN_PRIO::is_schedulable() {
  bool update = false;

  foreach(tasks.get_tasks(), ti) {
    ti->set_response_time(ti->get_critical_path_length());
  }

  foreach(tasks.get_tasks(), ti) {
    ulong response_time = get_response_time(*ti);

    if (response_time <= ti->get_deadline()) {
      ti->set_response_time(response_time);
    } else {
      return false;
    }
  }

  // do {
  //   update = false;
  //   foreach(tasks.get_tasks(), ti) {
  //     ulong old_response_time = ti->get_response_time();
  //     ulong response_time = get_response_time(*ti);

  //     if (old_response_time != response_time) update = true;

  //     if (response_time <= ti->get_deadline()) {
  //       ti->set_response_time(response_time);
  //     } else {
  //       return false;
  //     }
  //   }
  // } while (update);

  return true;
}

uint64_t LP_RTA_GFP_SPIN_PRIO::higher(const DAG_Task& ti, uint32_t res_id,
                                       uint64_t interval) {
  uint64_t sum = 0;
  foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
    if (th->is_request_exist(res_id)) {
      const Request& request_q = th->get_request_by_id(res_id);
      uint32_t n = th->get_max_job_num(interval);
      uint32_t N_h_q = request_q.get_num_requests();
      uint64_t L_h_q = request_q.get_max_length();
      sum += n * N_h_q * L_h_q;
    }
  }
  return sum;
}

uint64_t LP_RTA_GFP_SPIN_PRIO::lower(const DAG_Task& ti,
                                      uint32_t res_id) {
  uint64_t max = 0;
  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    if (tl->is_request_exist(res_id)) {
      const Request& request_q = tl->get_request_by_id(res_id);
      uint64_t L_l_q = request_q.get_max_length();
      if (L_l_q > max) max = L_l_q;
    }
  }
  return max;
}

uint64_t LP_RTA_GFP_SPIN_PRIO::equal(const DAG_Task& ti,
                                      uint32_t res_id) {
  if (ti.is_request_exist(res_id)) {
    const Request& request_q = ti.get_request_by_id(res_id);
    uint32_t N_i_q = request_q.get_num_requests();
    uint64_t L_i_q = request_q.get_max_length();
    uint32_t n_i = ti.get_dcores();
    uint32_t m_i = min(n_i - 1, N_i_q - 1);
    return m_i * L_i_q;
  } else {
    return 0;
  }
}

uint64_t LP_RTA_GFP_SPIN_PRIO::dpr(const DAG_Task& ti, uint32_t res_id) {
  uint64_t test_start = lower(ti, res_id) + equal(ti, res_id);
  uint64_t test_end = ti.get_deadline();
  uint64_t delay = test_start;

  while (delay <= test_end) {
    uint64_t temp = test_start;

    temp += higher(ti, res_id, delay);

    if (temp > delay)
      delay = temp;
    else if (temp == delay)
      return delay;
  }
  return test_end + 100;
}


uint64_t LP_RTA_GFP_SPIN_PRIO::resource_delay(const DAG_Task& ti, uint res_id) {
  uint32_t request_num = ti.get_request_by_id(res_id).get_num_requests();
  uint64_t max_delay = 0;

  for (uint x = 0; x <= request_num; x++) {
    GFP_SPINMapper vars;
    LinearProgram delay;
    LinearExpression* obj = new LinearExpression();
    objective(ti, res_id, x, &vars, obj);
    delay.set_objective(obj);
    vars.seal();
    add_constraints(ti, res_id, x, &delay, &vars);

    GLPKSolution* rb_solution =
        new GLPKSolution(delay, vars.get_num_vars());

    if (rb_solution->is_solved()) {
      double result = rb_solution->evaluate(*(delay.get_objective()));
      if (max_delay < result) {
        max_delay = result;
      }
      delete rb_solution;
    } else {
      cout << "[LP_RTA_GFP_SPIN_PRIO]unsolved." << endl;
      cout << "LP solver encounters a problem at T[" << ti.get_id() << "] l[" << res_id << "] X[" << x << "]" << endl;
      // tasks.display();
      delete rb_solution;
      // exit(1);
      // return ti.get_deadline();
      continue;
      // return MAX_LONG;
    }

#if GLPK_MEM_USAGE_CHECK == 1
    int peak;
    glp_mem_usage(NULL, &peak, NULL, NULL);
    cout << "Peak memory usage:" << peak << endl;
#endif
  }
  return max_delay;
}


uint64_t LP_RTA_GFP_SPIN_PRIO::total_resource_delay(const DAG_Task& ti) {
  uint64_t sum = 0;
  foreach(ti.get_requests(), r_i_q) {
    uint res_id = r_i_q->get_resource_id();
    sum += resource_delay(ti, res_id);
  }
  return sum;
}

ulong LP_RTA_GFP_SPIN_PRIO::get_response_time(const DAG_Task& ti) {
  uint64_t C_i = ti.get_wcet();
  uint64_t L_i = ti.get_critical_path_length();
  uint64_t D_i = ti.get_deadline();
  // uint64_t B_i = total_resource_delay(ti);
  uint32_t p_num = processors.get_processor_num();
  // uint64_t test_start = L_i + B_i + (C_i - L_i)/p_num;
  // cout << "total resource delay:" << B_i << endl;
  // cout << "test start:" << test_start << endl;
  uint64_t response_time = L_i + (C_i - L_i)/p_num;

  uint64_t interf = 0;
  foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
    interf += workload_bound((*tx), D_i);
    // interf += workload_bound((*tx), response_time);
  }
  response_time += interf/p_num;

  if (response_time <= D_i) {
    uint64_t B_i = total_resource_delay(ti);
    cout << "[PRIO]TRD:" << B_i << endl;
    response_time += B_i;
  }

  return response_time;
}

uint64_t LP_RTA_GFP_SPIN_PRIO::workload_bound(const DAG_Task& tx, int64_t interval) {
  int64_t L_x = tx.get_critical_path_length();
  int64_t T_x = tx.get_period();
  int64_t W_x = tx.get_wcet();
  int64_t delta_C = interval - max((int64_t)0, (interval - L_x)/ T_x) * T_x;
  int64_t bound = max((int64_t)0, (interval - delta_C)/ T_x) * W_x + (int64_t)get_WC(tx, delta_C);
  return bound;
}

void LP_RTA_GFP_SPIN_PRIO::declare_variable_bounds(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                                      GFP_SPINMapper* vars) {

}

void LP_RTA_GFP_SPIN_PRIO::objective(const DAG_Task& ti, uint res_id, uint res_num, GFP_SPINMapper* vars,
                        LinearExpression* obj) {
  int64_t var_id;
  uint request_num_x, request_num_y;
  double coef = 1.0 / processors.get_processor_num();

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num_x = res_num;
    else
      request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
    int32_t L_x_q = tx->get_request_by_id(res_id).get_max_length();
    // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    for (uint u = 1; u <= request_num_x; u++) {
      var_id = vars->lookup(x, u, 0, 0, GFP_SPINMapper::BLOCKING_SPIN);
      obj->add_term(var_id, L_x_q);

      foreach(tasks.get_tasks(), ty) {  // for transitive delay
        uint y = ty->get_id();
        if (!ty->is_request_exist(res_id))
          continue;
        if (y == ti.get_id())
          request_num_y = res_num;
        else
          request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());
        for (uint v = 1; v <= request_num_y; v++) {
          double temp = coef * L_x_q;
          // cout << "temp" << temp << endl;
          var_id = vars->lookup(x, u, y, v, GFP_SPINMapper::BLOCKING_TRANS);
          obj->add_term(var_id, temp);
        }
      }

      if (tx->get_priority() <= ti.get_priority()) {  // to counteract the interference
        double temp_2 = -1.0 * coef * L_x_q;
        var_id = vars->lookup(x, u, 0, 0, GFP_SPINMapper::BLOCKING_SPIN);
        obj->add_term(var_id, temp_2);
      }
    }
  }
}

void LP_RTA_GFP_SPIN_PRIO::add_constraints(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                              GFP_SPINMapper* vars) {
  constraint_1(ti, res_id, res_num, lp, vars);
  constraint_2(ti, res_id, res_num, lp, vars);
  constraint_3(ti, res_id, res_num, lp, vars);
  constraint_4(ti, res_id, res_num, lp, vars);
  constraint_7(ti, res_id, res_num, lp, vars);
  constraint_8(ti, res_id, res_num, lp, vars);
  constraint_9(ti, res_id, res_num, lp, vars);
  constraint_10(ti, res_id, res_num, lp, vars);
}

void LP_RTA_GFP_SPIN_PRIO::constraint_1(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          GFP_SPINMapper* vars) {
  int64_t var_id;
  uint request_num_x, request_num_y;

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num_x = res_num;
    else
      request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
    // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    for (uint u = 1; u <= request_num_x; u++) {
      foreach(tasks.get_tasks(), ty) {
        uint y = ty->get_id();
        if (!ty->is_request_exist(res_id))
          continue;
        if (y == ti.get_id())
          request_num_y = res_num;
        else
          request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());

        for (uint v = 1; v <= request_num_y; v++) {
          LinearExpression *exp = new LinearExpression();

          var_id = vars->lookup(x, u, y, v, GFP_SPINMapper::BLOCKING_TRANS);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(x, u, 0, 0, GFP_SPINMapper::BLOCKING_SPIN);
          exp->add_term(var_id, 1);

          lp->add_inequality(exp, 1);
        }
      }
    }
  }
}

// 
void LP_RTA_GFP_SPIN_PRIO::constraint_2(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          GFP_SPINMapper* vars) {
  int64_t var_id;
  uint request_num_x, request_num_y;

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num_x = res_num;
    else
      request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
    // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    for (uint u = 1; u <= request_num_x; u++) {
      foreach(tasks.get_tasks(), ty) {
        uint y = ty->get_id();
        if (!ty->is_request_exist(res_id))
          continue;
        if (y == ti.get_id())
          request_num_y = res_num;
        else
          request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());

        for (uint v = 1; v <= request_num_y; v++) {
          LinearExpression *exp = new LinearExpression();
          var_id = vars->lookup(x, u, y, v, GFP_SPINMapper::BLOCKING_TRANS);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(y, v, x, u, GFP_SPINMapper::BLOCKING_TRANS);
          exp->add_term(var_id, 1);

          lp->add_inequality(exp, 1);
        }
      }
    }
  }
}

// 
void LP_RTA_GFP_SPIN_PRIO::constraint_3(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          GFP_SPINMapper* vars) {
  int64_t var_id;
  uint request_num_x, request_num_y;

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num_x = res_num;
    else
      request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
    // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    for (uint u = 1; u <= request_num_x; u++) {
      LinearExpression *exp = new LinearExpression();
      var_id = vars->lookup(x, u, x, u, GFP_SPINMapper::BLOCKING_TRANS);
      exp->add_term(var_id, 1);

      lp->add_equality(exp, 0);
    }
  }
}

// 
void LP_RTA_GFP_SPIN_PRIO::constraint_4(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          GFP_SPINMapper* vars) {
  int64_t var_id;
  uint request_num_x, request_num_y;
  uint p_num = processors.get_processor_num();

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num_x = res_num;
    else
      request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
    // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    for (uint u = 1; u <= request_num_x; u++) {
      LinearExpression *exp = new LinearExpression();

      foreach(tasks.get_tasks(), ty) {
        uint y = ty->get_id();
        if (!ty->is_request_exist(res_id))
          continue;
        if (y == ti.get_id())
          request_num_y = res_num;
        else
          request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());
        for (uint v = 1; v <= request_num_y; v++) {
          var_id = vars->lookup(x, u, y, v, GFP_SPINMapper::BLOCKING_TRANS);
          exp->add_term(var_id, 1);
        }
      }

      var_id = vars->lookup(x, u, 0, 0, GFP_SPINMapper::BLOCKING_SPIN);
      exp->add_term(var_id, 1);

      lp->add_inequality(exp, p_num - 1);
    }
  }
}

// 
void LP_RTA_GFP_SPIN_PRIO::constraint_7(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          GFP_SPINMapper* vars) {
  int64_t var_id;
  uint request_num_x, request_num_y;
  uint p_num = processors.get_processor_num();

  foreach(tasks.get_tasks(), ty) {
    uint y = ty->get_id();
    if (!ty->is_request_exist(res_id))
      continue;
    if (y == ti.get_id())
      request_num_y = res_num;
    else
      request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());
    for (uint v = 1; v <= request_num_y; v++) {
      LinearExpression *exp = new LinearExpression();

      foreach_lower_priority_task(tasks.get_tasks(), (*ty), tx) {
        uint x = tx->get_id();
        if (!tx->is_request_exist(res_id))
          continue;
        if (x == ti.get_id())
          request_num_x = res_num;
        else
          request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
        for (uint u = 1; u <= request_num_x; u++) {
          var_id = vars->lookup(x, u, y, v, GFP_SPINMapper::BLOCKING_TRANS);
          exp->add_term(var_id, 1);
        }
      }

      lp->add_inequality(exp, 1);
    }
  }
}

// 
void LP_RTA_GFP_SPIN_PRIO::constraint_8(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          GFP_SPINMapper* vars) {
  int64_t var_id;
  uint request_num_x;
  uint p_num = processors.get_processor_num();
  uint N_i_q = ti.get_request_by_id(res_id).get_num_requests();

  LinearExpression *exp = new LinearExpression();

  foreach_lower_priority_task(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num_x = res_num;
    else
      request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
    // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    for (uint u = 1; u <= request_num_x; u++) {
      var_id = vars->lookup(x, u, 0, 0, GFP_SPINMapper::BLOCKING_SPIN);
      exp->add_term(var_id, 1);
    }
  }
  lp->add_inequality(exp, (N_i_q - res_num));
}

// 
void LP_RTA_GFP_SPIN_PRIO::constraint_9(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          GFP_SPINMapper* vars) {
  int64_t var_id;
  uint request_num_x, request_num_y;
  uint p_num = processors.get_processor_num();

  foreach(tasks.get_tasks(), ty) {
    uint y = ty->get_id();
    if (!ty->is_request_exist(res_id))
      continue;
    if (y == ti.get_id())
      request_num_y = res_num;
    else
      request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());
    for (uint v = 1; v <= request_num_y; v++) {
      foreach_higher_priority_task(tasks.get_tasks(), (*ty), tx) {
        uint x = tx->get_id();
        if (!tx->is_request_exist(res_id))
          continue;
        if (x == ti.get_id())
          request_num_x = res_num;
        else
          request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());

        uint32_t ub = ceiling(dpr((*ty), res_id)+tx->get_deadline(), tx->get_period()) * tx->get_request_by_id(res_id).get_num_requests();
        LinearExpression *exp = new LinearExpression();

        for (uint u = 1; u <= request_num_x; u++) {
          var_id = vars->lookup(x, u, y, v, GFP_SPINMapper::BLOCKING_TRANS);
          exp->add_term(var_id, 1);
        }

        lp->add_inequality(exp, ub);
      }
    }
  }
}

// 
void LP_RTA_GFP_SPIN_PRIO::constraint_10(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          GFP_SPINMapper* vars) {
  int64_t var_id;
  uint request_num_x;
  uint p_num = processors.get_processor_num();
  uint N_i_q = ti.get_request_by_id(res_id).get_num_requests();

  foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num_x = res_num;
    else
      request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
    uint32_t ub = ceiling(dpr(ti, res_id)+tx->get_deadline(), tx->get_period()) * tx->get_request_by_id(res_id).get_num_requests() * (N_i_q - res_num);
    LinearExpression *exp = new LinearExpression();

    for (uint u = 1; u <= request_num_x; u++) {
      var_id = vars->lookup(x, u, 0, 0, GFP_SPINMapper::BLOCKING_SPIN);
      exp->add_term(var_id, 1);
    }

    lp->add_inequality(exp, ub);
  }
}



/** Class LP_RTA_GFP_SPIN_UNOR */

LP_RTA_GFP_SPIN_UNOR::LP_RTA_GFP_SPIN_UNOR()
    : RTA_DAG_GFP_JOSE() {}

LP_RTA_GFP_SPIN_UNOR::LP_RTA_GFP_SPIN_UNOR(DAG_TaskSet tasks, ProcessorSet processors,
                                 ResourceSet resources)
    : RTA_DAG_GFP_JOSE(tasks, processors, resources) {}

LP_RTA_GFP_SPIN_UNOR::~LP_RTA_GFP_SPIN_UNOR() {}

bool LP_RTA_GFP_SPIN_UNOR::is_schedulable() {
  bool update = false;

  foreach(tasks.get_tasks(), ti) {
    ti->set_response_time(ti->get_critical_path_length());
  }

  foreach(tasks.get_tasks(), ti) {
    ulong response_time = get_response_time(*ti);

    if (response_time <= ti->get_deadline()) {
      ti->set_response_time(response_time);
    } else {
      return false;
    }
  }

  // do {
  //   update = false;
  //   foreach(tasks.get_tasks(), ti) {
  //     ulong old_response_time = ti->get_response_time();
  //     ulong response_time = get_response_time(*ti);

  //     if (old_response_time != response_time) update = true;

  //     if (response_time <= ti->get_deadline()) {
  //       ti->set_response_time(response_time);
  //     } else {
  //       return false;
  //     }
  //   }
  // } while (update);

  return true;
}

uint64_t LP_RTA_GFP_SPIN_UNOR::higher(const DAG_Task& ti, uint32_t res_id,
                                       uint64_t interval) {
  uint64_t sum = 0;
  foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
    if (th->is_request_exist(res_id)) {
      const Request& request_q = th->get_request_by_id(res_id);
      uint32_t n = th->get_max_job_num(interval);
      uint32_t N_h_q = request_q.get_num_requests();
      uint64_t L_h_q = request_q.get_max_length();
      sum += n * N_h_q * L_h_q;
    }
  }
  return sum;
}

uint64_t LP_RTA_GFP_SPIN_UNOR::lower(const DAG_Task& ti,
                                      uint32_t res_id) {
  uint64_t max = 0;
  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    if (tl->is_request_exist(res_id)) {
      const Request& request_q = tl->get_request_by_id(res_id);
      uint64_t L_l_q = request_q.get_max_length();
      if (L_l_q > max) max = L_l_q;
    }
  }
  return max;
}

uint64_t LP_RTA_GFP_SPIN_UNOR::equal(const DAG_Task& ti,
                                      uint32_t res_id) {
  if (ti.is_request_exist(res_id)) {
    const Request& request_q = ti.get_request_by_id(res_id);
    uint32_t N_i_q = request_q.get_num_requests();
    uint64_t L_i_q = request_q.get_max_length();
    uint32_t n_i = ti.get_dcores();
    uint32_t m_i = min(n_i - 1, N_i_q - 1);
    return m_i * L_i_q;
  } else {
    return 0;
  }
}

uint64_t LP_RTA_GFP_SPIN_UNOR::dpr(const DAG_Task& ti, uint32_t res_id) {
  uint64_t test_start = lower(ti, res_id) + equal(ti, res_id);
  uint64_t test_end = ti.get_deadline();
  uint64_t delay = test_start;

  while (delay <= test_end) {
    uint64_t temp = test_start;

    temp += higher(ti, res_id, delay);

    if (temp > delay)
      delay = temp;
    else if (temp == delay)
      return delay;
  }
  return test_end + 100;
}


uint64_t LP_RTA_GFP_SPIN_UNOR::resource_delay(const DAG_Task& ti, uint res_id) {
  uint32_t request_num = ti.get_request_by_id(res_id).get_num_requests();
  uint64_t max_delay = 0;

  for (uint x = 0; x <= request_num; x++) {
    GFP_SPINMapper vars;
    LinearProgram delay;
    LinearExpression* obj = new LinearExpression();
    objective(ti, res_id, x, &vars, obj);
    delay.set_objective(obj);
    vars.seal();
    add_constraints(ti, res_id, x, &delay, &vars);

    GLPKSolution* rb_solution =
        new GLPKSolution(delay, vars.get_num_vars());

    if (rb_solution->is_solved()) {
      double result = rb_solution->evaluate(*(delay.get_objective()));
      if (max_delay < result) {
        max_delay = result;
      }
      delete rb_solution;
    } else {
      cout << "[LP_RTA_GFP_SPIN_UNOR]unsolved." << endl;
      cout << "LP solver encounters a problem at T[" << ti.get_id() << "] l[" << res_id << "] X[" << x << "]" << endl;
      // tasks.display();
      delete rb_solution;
      continue;
      // exit(1);
      // return ti.get_deadline();
      // return MAX_LONG;
    }

#if GLPK_MEM_USAGE_CHECK == 1
    int peak;
    glp_mem_usage(NULL, &peak, NULL, NULL);
    cout << "Peak memory usage:" << peak << endl;
#endif
  }
  return max_delay;
}


uint64_t LP_RTA_GFP_SPIN_UNOR::total_resource_delay(const DAG_Task& ti) {
  uint64_t sum = 0;
  foreach(ti.get_requests(), r_i_q) {
    uint res_id = r_i_q->get_resource_id();
    sum += resource_delay(ti, res_id);
  }
  return sum;
}

ulong LP_RTA_GFP_SPIN_UNOR::get_response_time(const DAG_Task& ti) {
  uint64_t C_i = ti.get_wcet();
  uint64_t L_i = ti.get_critical_path_length();
  uint64_t D_i = ti.get_deadline();
  // uint64_t B_i = total_resource_delay(ti);
  uint32_t p_num = processors.get_processor_num();
  // uint64_t test_start = L_i + B_i + (C_i - L_i)/p_num;
  // cout << "total resource delay:" << B_i << endl;
  // cout << "test start:" << test_start << endl;
  uint64_t response_time = L_i + (C_i - L_i)/p_num;

  uint64_t interf = 0;
  foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
    interf += workload_bound((*tx), D_i);
    // interf += workload_bound((*tx), response_time);
  }
  response_time += interf/p_num;

  if (response_time <= D_i) {
    uint64_t B_i = total_resource_delay(ti);
    cout << "[UNOR]TRD:" << B_i << endl;
    response_time += B_i;
  }

  return response_time;
}

uint64_t LP_RTA_GFP_SPIN_UNOR::workload_bound(const DAG_Task& tx, int64_t interval) {
  int64_t L_x = tx.get_critical_path_length();
  int64_t T_x = tx.get_period();
  int64_t W_x = tx.get_wcet();
  int64_t delta_C = interval - max((int64_t)0, (interval - L_x)/ T_x) * T_x;
  int64_t bound = max((int64_t)0, (interval - delta_C)/ T_x) * W_x + (int64_t)get_WC(tx, delta_C);
  return bound;
}

void LP_RTA_GFP_SPIN_UNOR::declare_variable_bounds(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                                      GFP_SPINMapper* vars) {

}

void LP_RTA_GFP_SPIN_UNOR::objective(const DAG_Task& ti, uint res_id, uint res_num, GFP_SPINMapper* vars,
                        LinearExpression* obj) {
  int64_t var_id;
  uint request_num_x, request_num_y;
  double coef = 1.0 / processors.get_processor_num();

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num_x = res_num;
    else
      request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
    int32_t L_x_q = tx->get_request_by_id(res_id).get_max_length();
    // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    for (uint u = 1; u <= request_num_x; u++) {
      var_id = vars->lookup(x, u, 0, 0, GFP_SPINMapper::BLOCKING_SPIN);
      obj->add_term(var_id, L_x_q);

      foreach(tasks.get_tasks(), ty) {  // for transitive delay
        uint y = ty->get_id();
        if (!ty->is_request_exist(res_id))
          continue;
        if (y == ti.get_id())
          request_num_y = res_num;
        else
          request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());
        for (uint v = 1; v <= request_num_y; v++) {
          double temp = coef * L_x_q;
          // cout << "temp" << temp << endl;
          var_id = vars->lookup(x, u, y, v, GFP_SPINMapper::BLOCKING_TRANS);
          obj->add_term(var_id, temp);
        }
      }

      if (tx->get_priority() <= ti.get_priority()) {  // to counteract the interference
        double temp_2 = -1.0 * coef * L_x_q;
        var_id = vars->lookup(x, u, 0, 0, GFP_SPINMapper::BLOCKING_SPIN);
        obj->add_term(var_id, temp_2);
      }
    }
  }
}

void LP_RTA_GFP_SPIN_UNOR::add_constraints(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                              GFP_SPINMapper* vars) {
  constraint_1(ti, res_id, res_num, lp, vars);
  constraint_2(ti, res_id, res_num, lp, vars);
  constraint_3(ti, res_id, res_num, lp, vars);
  constraint_4(ti, res_id, res_num, lp, vars);
  constraint_11(ti, res_id, res_num, lp, vars);
  constraint_12(ti, res_id, res_num, lp, vars);
}

void LP_RTA_GFP_SPIN_UNOR::constraint_1(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          GFP_SPINMapper* vars) {
  int64_t var_id;
  uint request_num_x, request_num_y;

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num_x = res_num;
    else
      request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
    // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    for (uint u = 1; u <= request_num_x; u++) {
      foreach(tasks.get_tasks(), ty) {
        uint y = ty->get_id();
        if (!ty->is_request_exist(res_id))
          continue;
        if (y == ti.get_id())
          request_num_y = res_num;
        else
          request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());

        for (uint v = 1; v <= request_num_y; v++) {
          LinearExpression *exp = new LinearExpression();

          var_id = vars->lookup(x, u, y, v, GFP_SPINMapper::BLOCKING_TRANS);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(x, u, 0, 0, GFP_SPINMapper::BLOCKING_SPIN);
          exp->add_term(var_id, 1);

          lp->add_inequality(exp, 1);
        }
      }
    }
  }
}

// 
void LP_RTA_GFP_SPIN_UNOR::constraint_2(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          GFP_SPINMapper* vars) {
  int64_t var_id;
  uint request_num_x, request_num_y;

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num_x = res_num;
    else
      request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
    // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    for (uint u = 1; u <= request_num_x; u++) {
      foreach(tasks.get_tasks(), ty) {
        uint y = ty->get_id();
        if (!ty->is_request_exist(res_id))
          continue;
        if (y == ti.get_id())
          request_num_y = res_num;
        else
          request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());

        for (uint v = 1; v <= request_num_y; v++) {
          LinearExpression *exp = new LinearExpression();
          var_id = vars->lookup(x, u, y, v, GFP_SPINMapper::BLOCKING_TRANS);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(y, v, x, u, GFP_SPINMapper::BLOCKING_TRANS);
          exp->add_term(var_id, 1);

          lp->add_inequality(exp, 1);
        }
      }
    }
  }
}

// 
void LP_RTA_GFP_SPIN_UNOR::constraint_3(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          GFP_SPINMapper* vars) {
  int64_t var_id;
  uint request_num_x, request_num_y;

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num_x = res_num;
    else
      request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
    // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    for (uint u = 1; u <= request_num_x; u++) {
      LinearExpression *exp = new LinearExpression();
      var_id = vars->lookup(x, u, x, u, GFP_SPINMapper::BLOCKING_TRANS);
      exp->add_term(var_id, 1);

      lp->add_equality(exp, 0);
    }
  }
}

// 
void LP_RTA_GFP_SPIN_UNOR::constraint_4(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          GFP_SPINMapper* vars) {
  int64_t var_id;
  uint request_num_x, request_num_y;
  uint p_num = processors.get_processor_num();

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num_x = res_num;
    else
      request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
    // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    for (uint u = 1; u <= request_num_x; u++) {
      LinearExpression *exp = new LinearExpression();

      foreach(tasks.get_tasks(), ty) {
        uint y = ty->get_id();
        if (!ty->is_request_exist(res_id))
          continue;
        if (y == ti.get_id())
          request_num_y = res_num;
        else
          request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());
        for (uint v = 1; v <= request_num_y; v++) {
          var_id = vars->lookup(x, u, y, v, GFP_SPINMapper::BLOCKING_TRANS);
          exp->add_term(var_id, 1);
        }
      }

      var_id = vars->lookup(x, u, 0, 0, GFP_SPINMapper::BLOCKING_SPIN);
      exp->add_term(var_id, 1);

      lp->add_inequality(exp, p_num - 1);
    }
  }
}

// 
void LP_RTA_GFP_SPIN_UNOR::constraint_11(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          GFP_SPINMapper* vars) {
  int64_t var_id;
  uint request_num_x, request_num_y;
  uint p_num = processors.get_processor_num();

  foreach(tasks.get_tasks(), ty) {
    uint y = ty->get_id();
    if (!ty->is_request_exist(res_id))
      continue;
    if (y == ti.get_id())
      request_num_y = res_num;
    else
      request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());
    for (uint v = 1; v <= request_num_y; v++) {
      foreach_task_except(tasks.get_tasks(), ti, tx) {
        uint x = tx->get_id();
        if (!tx->is_request_exist(res_id))
          continue;
        if (x == ti.get_id())
          request_num_x = res_num;
        else
          request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());

        uint32_t ub = ceiling(dpr((*ty), res_id)+tx->get_deadline(), tx->get_period()) * tx->get_request_by_id(res_id).get_num_requests();
        LinearExpression *exp = new LinearExpression();

        for (uint u = 1; u <= request_num_x; u++) {
          var_id = vars->lookup(x, u, y, v, GFP_SPINMapper::BLOCKING_TRANS);
          exp->add_term(var_id, 1);
        }

        lp->add_inequality(exp, ub);
      }
    }
  }
}

// 
void LP_RTA_GFP_SPIN_UNOR::constraint_12(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          GFP_SPINMapper* vars) {
  int64_t var_id;
  uint request_num_x;
  uint p_num = processors.get_processor_num();
  uint N_i_q = ti.get_request_by_id(res_id).get_num_requests();

  foreach_task_except(tasks.get_tasks(), ti, tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num_x = res_num;
    else
      request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
    uint32_t ub = ceiling(dpr(ti, res_id)+tx->get_deadline(), tx->get_period()) * tx->get_request_by_id(res_id).get_num_requests() * (N_i_q - res_num);
    LinearExpression *exp = new LinearExpression();

    for (uint u = 1; u <= request_num_x; u++) {
      var_id = vars->lookup(x, u, 0, 0, GFP_SPINMapper::BLOCKING_SPIN);
      exp->add_term(var_id, 1);
    }

    lp->add_inequality(exp, ub);
  }
}


/** Class LP_RTA_GFP_SPIN_GENERAL */
LP_RTA_GFP_SPIN_GENERAL::LP_RTA_GFP_SPIN_GENERAL()
    : RTA_DAG_GFP_JOSE() {}

LP_RTA_GFP_SPIN_GENERAL::LP_RTA_GFP_SPIN_GENERAL(DAG_TaskSet tasks, ProcessorSet processors,
                                 ResourceSet resources)
    : RTA_DAG_GFP_JOSE(tasks, processors, resources) {}

LP_RTA_GFP_SPIN_GENERAL::~LP_RTA_GFP_SPIN_GENERAL() {}

bool LP_RTA_GFP_SPIN_GENERAL::is_schedulable() {
  bool update = false;

  foreach(tasks.get_tasks(), ti) {
    ti->set_response_time(ti->get_critical_path_length());
  }

  foreach(tasks.get_tasks(), ti) {
    ulong response_time = get_response_time(*ti);

    if (response_time <= ti->get_deadline()) {
      ti->set_response_time(response_time);
    } else {
      return false;
    }
  }

  // do {
  //   update = false;
  //   foreach(tasks.get_tasks(), ti) {
  //     ulong old_response_time = ti->get_response_time();
  //     ulong response_time = get_response_time(*ti);

  //     if (old_response_time != response_time) update = true;

  //     if (response_time <= ti->get_deadline()) {
  //       ti->set_response_time(response_time);
  //     } else {
  //       return false;
  //     }
  //   }
  // } while (update);

  return true;
}


uint64_t LP_RTA_GFP_SPIN_GENERAL::resource_delay(const DAG_Task& ti, uint res_id) {
  uint32_t request_num = ti.get_request_by_id(res_id).get_num_requests();
  uint64_t max_delay = 0;

  for (uint x = 0; x <= request_num; x++) {
    GFP_SPINMapper vars;
    LinearProgram delay;
    LinearExpression* obj = new LinearExpression();
    objective(ti, res_id, x, &vars, obj);
    delay.set_objective(obj);
    vars.seal();
    add_constraints(ti, res_id, x, &delay, &vars);

    GLPKSolution* rb_solution =
        new GLPKSolution(delay, vars.get_num_vars());

    if (rb_solution->is_solved()) {
      double result = rb_solution->evaluate(*(delay.get_objective()));
      if (max_delay < result) {
        max_delay = result;
      }
      delete rb_solution;
    } else {
      cout << "[LP_RTA_GFP_SPIN_GENERAL]unsolved." << endl;
      cout << "LP solver encounters a problem at T[" << ti.get_id() << "] l[" << res_id << "] X[" << x << "]" << endl;
      // tasks.display();
      delete rb_solution;
      continue;
      // exit(1);
      // return ti.get_deadline();
      // return MAX_LONG;
    }

#if GLPK_MEM_USAGE_CHECK == 1
    int peak;
    glp_mem_usage(NULL, &peak, NULL, NULL);
    cout << "Peak memory usage:" << peak << endl;
#endif
  }
  return max_delay;
}


uint64_t LP_RTA_GFP_SPIN_GENERAL::total_resource_delay(const DAG_Task& ti) {
  uint64_t sum = 0;
  foreach(ti.get_requests(), r_i_q) {
    uint res_id = r_i_q->get_resource_id();
    sum += resource_delay(ti, res_id);
  }
  return sum;
}

ulong LP_RTA_GFP_SPIN_GENERAL::get_response_time(const DAG_Task& ti) {
  uint64_t C_i = ti.get_wcet();
  uint64_t L_i = ti.get_critical_path_length();
  uint64_t D_i = ti.get_deadline();
  // uint64_t B_i = total_resource_delay(ti);
  uint32_t p_num = processors.get_processor_num();
  // uint64_t test_start = L_i + B_i + (C_i - L_i)/p_num;
  // cout << "total resource delay:" << B_i << endl;
  // cout << "test start:" << test_start << endl;
  uint64_t response_time = L_i + (C_i - L_i)/p_num;

  uint64_t interf = 0;
  foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
    interf += workload_bound((*tx), D_i);
    // interf += workload_bound((*tx), response_time);
  }
  response_time += interf/p_num;

  if (response_time <= D_i) {
    uint64_t B_i = total_resource_delay(ti);
    cout << "[GENERAL]TRD:" << B_i << endl;
    response_time += B_i;
  }

  return response_time;
}

uint64_t LP_RTA_GFP_SPIN_GENERAL::workload_bound(const DAG_Task& tx, int64_t interval) {
  int64_t L_x = tx.get_critical_path_length();
  int64_t T_x = tx.get_period();
  int64_t W_x = tx.get_wcet();
  int64_t delta_C = interval - max((int64_t)0, (interval - L_x)/ T_x) * T_x;
  int64_t bound = max((int64_t)0, (interval - delta_C)/ T_x) * W_x + (int64_t)get_WC(tx, delta_C);
  return bound;
}

void LP_RTA_GFP_SPIN_GENERAL::declare_variable_bounds(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                                      GFP_SPINMapper* vars) {

}

void LP_RTA_GFP_SPIN_GENERAL::objective(const DAG_Task& ti, uint res_id, uint res_num, GFP_SPINMapper* vars,
                        LinearExpression* obj) {
  int64_t var_id;
  uint request_num_x, request_num_y;
  double coef = 1.0 / processors.get_processor_num();

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num_x = res_num;
    else
      request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
    int32_t L_x_q = tx->get_request_by_id(res_id).get_max_length();
    // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    for (uint u = 1; u <= request_num_x; u++) {
      var_id = vars->lookup(x, u, 0, 0, GFP_SPINMapper::BLOCKING_SPIN);
      obj->add_term(var_id, L_x_q);

      foreach(tasks.get_tasks(), ty) {  // for transitive delay
        uint y = ty->get_id();
        if (!ty->is_request_exist(res_id))
          continue;
        if (y == ti.get_id())
          request_num_y = res_num;
        else
          request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());
        for (uint v = 1; v <= request_num_y; v++) {
          double temp = coef * L_x_q;
          // cout << "temp" << temp << endl;
          var_id = vars->lookup(x, u, y, v, GFP_SPINMapper::BLOCKING_TRANS);
          obj->add_term(var_id, temp);
        }
      }

      if (tx->get_priority() <= ti.get_priority()) {  // to counteract the interference
        double temp_2 = -1.0 * coef * L_x_q;
        var_id = vars->lookup(x, u, 0, 0, GFP_SPINMapper::BLOCKING_SPIN);
        obj->add_term(var_id, temp_2);
      }
    }
  }
}

void LP_RTA_GFP_SPIN_GENERAL::add_constraints(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                              GFP_SPINMapper* vars) {
  constraint_1(ti, res_id, res_num, lp, vars);
  constraint_2(ti, res_id, res_num, lp, vars);
  constraint_3(ti, res_id, res_num, lp, vars);
  constraint_4(ti, res_id, res_num, lp, vars);
  // constraint_5(ti, res_id, res_num, lp, vars);
  // constraint_6(ti, res_id, res_num, lp, vars);
}

void LP_RTA_GFP_SPIN_GENERAL::constraint_1(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          GFP_SPINMapper* vars) {
  int64_t var_id;
  uint request_num_x, request_num_y;

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num_x = res_num;
    else
      request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
    // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    for (uint u = 1; u <= request_num_x; u++) {
      foreach(tasks.get_tasks(), ty) {
        uint y = ty->get_id();
        if (!ty->is_request_exist(res_id))
          continue;
        if (y == ti.get_id())
          request_num_y = res_num;
        else
          request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());

        for (uint v = 1; v <= request_num_y; v++) {
          LinearExpression *exp = new LinearExpression();

          var_id = vars->lookup(x, u, y, v, GFP_SPINMapper::BLOCKING_TRANS);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(x, u, 0, 0, GFP_SPINMapper::BLOCKING_SPIN);
          exp->add_term(var_id, 1);

          lp->add_inequality(exp, 1);
        }
      }
    }
  }
}

// 
void LP_RTA_GFP_SPIN_GENERAL::constraint_2(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          GFP_SPINMapper* vars) {
  int64_t var_id;
  uint request_num_x, request_num_y;

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num_x = res_num;
    else
      request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
    // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    for (uint u = 1; u <= request_num_x; u++) {
      foreach(tasks.get_tasks(), ty) {
        uint y = ty->get_id();
        if (!ty->is_request_exist(res_id))
          continue;
        if (y == ti.get_id())
          request_num_y = res_num;
        else
          request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());

        for (uint v = 1; v <= request_num_y; v++) {
          LinearExpression *exp = new LinearExpression();
          var_id = vars->lookup(x, u, y, v, GFP_SPINMapper::BLOCKING_TRANS);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(y, v, x, u, GFP_SPINMapper::BLOCKING_TRANS);
          exp->add_term(var_id, 1);

          lp->add_inequality(exp, 1);
        }
      }
    }
  }
}

// 
void LP_RTA_GFP_SPIN_GENERAL::constraint_3(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          GFP_SPINMapper* vars) {
  int64_t var_id;
  uint request_num_x;

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num_x = res_num;
    else
      request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
    // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    for (uint u = 1; u <= request_num_x; u++) {
      LinearExpression *exp = new LinearExpression();
      var_id = vars->lookup(x, u, x, u, GFP_SPINMapper::BLOCKING_TRANS);
      exp->add_term(var_id, 1);

      lp->add_equality(exp, 0);
    }
  }
}

// 
void LP_RTA_GFP_SPIN_GENERAL::constraint_4(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                          GFP_SPINMapper* vars) {
  int64_t var_id;
  uint request_num_x, request_num_y;
  uint p_num = processors.get_processor_num();

  foreach(tasks.get_tasks(), tx) {
    uint x = tx->get_id();
    if (!tx->is_request_exist(res_id))
      continue;
    if (x == ti.get_id())
      request_num_x = res_num;
    else
      request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
    // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
    for (uint u = 1; u <= request_num_x; u++) {
      LinearExpression *exp = new LinearExpression();

      foreach(tasks.get_tasks(), ty) {
        uint y = ty->get_id();
        if (!ty->is_request_exist(res_id))
          continue;
        if (y == ti.get_id())
          request_num_y = res_num;
        else
          request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());
        for (uint v = 1; v <= request_num_y; v++) {
          var_id = vars->lookup(x, u, y, v, GFP_SPINMapper::BLOCKING_TRANS);
          exp->add_term(var_id, 1);
        }
      }

      var_id = vars->lookup(x, u, 0, 0, GFP_SPINMapper::BLOCKING_SPIN);
      exp->add_term(var_id, 1);

      lp->add_inequality(exp, p_num - 1);
    }
  }
}
