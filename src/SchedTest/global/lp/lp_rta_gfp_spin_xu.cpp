// Copyright [2021] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <lp.h>
#include <lp_rta_gfp_spin_xu.h>
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

/** Class GFP_SPIN_XUMapper */

uint64_t GFP_SPIN_XUMapper::encode_request(uint64_t task, uint64_t resource,
                                        uint64_t request, uint64_t processor, 
                                        uint64_t type) {
  uint64_t one = 1;
  uint64_t key = 0;
  // cout << "task_1:" << task_1 << endl;
  // cout << "task_2:" << task_2 << endl;
  // cout << "request_1:" << request_1 << endl;
  // cout << "request_2:" << request_2 << endl;
  assert(task < (one << 10));
  assert(resource < (one << 5));
  assert(request < (one << 10));
  assert(processor < (one << 10));
  assert(type < (one << 4));

  key |= (type << 35);
  key |= (task << 25);
  key |= (resource << 20);
  key |= (request << 10);
  key |= processor;
  return key;
}

uint64_t GFP_SPIN_XUMapper::get_type(uint64_t var) {
  return (var >> 35) & (uint64_t)0xf;  // 4 bits
}

uint64_t GFP_SPIN_XUMapper::get_task(uint64_t var) {
  return (var >> 25) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t GFP_SPIN_XUMapper::get_resource(uint64_t var) {
  return (var >> 20) & (uint64_t)0x1f;  // 5 bits
}

uint64_t GFP_SPIN_XUMapper::get_request(uint64_t var) {
  return (var >> 10) & (uint64_t)0x3ff;  // 10 bits
}

uint64_t GFP_SPIN_XUMapper::get_processor(uint64_t var) {
  return (var) & (uint64_t)0x3ff;  // 10 bits
}

GFP_SPIN_XUMapper::GFP_SPIN_XUMapper(uint start_var) : VarMapperBase(start_var) {}

uint GFP_SPIN_XUMapper::lookup(uint task, uint resource, uint request, uint processor, var_type type) {
  // cout << "x,u,y,v:" << task_1 << "," << request_1 << "," << task_2 << "," << request_2 << endl;
  uint64_t key = encode_request(task, resource, request, processor, type);
  uint var = var_for_key(key);
  return var;
}

string GFP_SPIN_XUMapper::key2str(uint64_t key, uint var) const {
  ostringstream buf;

  switch (get_type(key)) {
    case GFP_SPIN_XUMapper::BLOCKING_KP:
      buf << "B^KP[";
      break;
    case GFP_SPIN_XUMapper::BLOCKING_NPD:
      buf << "B^NPD[";
      break;
    case GFP_SPIN_XUMapper::BLOCKING_DD:
      buf << "B^DD[";
      break;
    case GFP_SPIN_XUMapper::REQUEST_NUM:
      buf << "Y[";
      break;
    default:
      buf << "?[";
  }

  buf << get_task(key) << ", " << get_resource(key) << ", " << get_request(key) << ", " << get_processor(key)
      << "]";

  return buf.str();
}


/** Class LP_RTA_GFP_SPIN_XU_UNOR */

LP_RTA_GFP_SPIN_XU_UNOR::LP_RTA_GFP_SPIN_XU_UNOR()
    : RTA_DAG_GFP_JOSE() {}

LP_RTA_GFP_SPIN_XU_UNOR::LP_RTA_GFP_SPIN_XU_UNOR(DAG_TaskSet tasks, ProcessorSet processors,
                                 ResourceSet resources)
    : RTA_DAG_GFP_JOSE(tasks, processors, resources) {}

LP_RTA_GFP_SPIN_XU_UNOR::~LP_RTA_GFP_SPIN_XU_UNOR() {}

bool LP_RTA_GFP_SPIN_XU_UNOR::is_schedulable() {
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

      if (response_time <= ti->get_deadline()) {
        ti->set_response_time(response_time);
      } else {
        return false;
      }
    }
  } while (update);

  // foreach(tasks.get_tasks(), ti) {
  //   uint64_t TRD = resource_delay((*ti))/processors.get_processor_num();
  //   cout << "TRD:" << TRD << endl;
  // }

  return true;
}

// bool LP_RTA_GFP_SPIN_XU_UNOR::is_schedulable() {
//   bool update = false;

//   foreach(tasks.get_tasks(), ti) {
//     ti->set_response_time(ti->get_critical_path_length());
//   }

//   foreach(tasks.get_tasks(), ti) {
//     ulong response_time = get_response_time(*ti);

//     if (response_time <= ti->get_deadline()) {
//       ti->set_response_time(response_time);
//     } else {
//       return false;
//     }
//   }

//   return true;
// }

uint64_t LP_RTA_GFP_SPIN_XU_UNOR::resource_delay(const DAG_Task& ti) {
  uint64_t max_delay;
  uint32_t p_num = processors.get_processor_num();
  GFP_SPIN_XUMapper vars;
  LinearProgram delay;
  LinearExpression* obj = new LinearExpression();
  objective(ti, &vars, obj);
  delay.set_objective(obj);
  declare_variable_bounds(ti, &delay, &vars);
  vars.seal();
  add_constraints(ti, &delay, &vars);

  GLPKSolution* rb_solution =
      new GLPKSolution(delay, vars.get_num_vars());
  // cout << "max_var_num:" << vars.get_num_vars() << endl;

  if (rb_solution->is_solved()) {
    int64_t var_id; 
    // foreach(resources.get_resources(), l_q) {
    //   uint q = l_q->get_resource_id();
    //   if (!ti.is_request_exist(q))
    //     continue;

    //   LinearExpression* exp = new LinearExpression();
    //   var_id = vars.lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
    //   exp->add_term(var_id,1);
    //   cout << "Y[" << q << "]" << rb_solution->evaluate(*exp) << endl; 
    //   delete exp;
    // }

    
    // LinearExpression* exp = new LinearExpression();
    // foreach(tasks.get_tasks(), tj) {
    //   uint j = tj->get_id();
    //   uint32_t request_num_j;
    //   foreach(resources.get_resources(), l_q) {
    //     uint q = l_q->get_resource_id();
    //     if (!tj->is_request_exist(q))
    //       continue;
    //     int32_t L_j_q = tj->get_request_by_id(q).get_max_length();
    //     if (j == ti.get_id()) {
    //       request_num_j = tj->get_request_by_id(q).get_num_requests();
    //     } else {
    //       request_num_j = tj->get_max_request_num(q, ti.get_response_time());
    //     }
    //     for (uint k = 1; k <= request_num_j; k++) {
    //       var_id = vars.lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
    //       exp->add_term(var_id, L_j_q);
    //     }
    //   }
    // }
    // cout << "BKP:" << rb_solution->evaluate(*exp) << endl; 

    double result = rb_solution->evaluate(*(delay.get_objective()));
    delete rb_solution;
    max_delay = result;
  } else {
    cout << "[LP_RTA_GFP_SPIN_XU_UNOR]unsolved." << endl;
    // cout << "LP solver encounters a problem at T[" << ti.get_id() << "] l[" << res_id << "] X[" << x << "]" << endl;
    // tasks.display();
    rb_solution->show_error();
    if (GLP_FEAS == rb_solution->get_status()) {
      max_delay = rb_solution->evaluate(*(delay.get_objective()));
    } else {
      delete rb_solution;
      exit(1);
      return ti.get_deadline() * p_num;;
    }

    // continue;
    // exit(1);
    // return ti.get_deadline() * p_num;
    // return MAX_LONG;
  }

#if GLPK_MEM_USAGE_CHECK == 1
  int peak;
  glp_mem_usage(NULL, &peak, NULL, NULL);
  cout << "Peak memory usage:" << peak << endl;
#endif

  return max_delay;
}


// uint64_t LP_RTA_GFP_SPIN_XU_UNOR::getresponse_time(const DAG_Task& ti) {
//   uint32_t t_id = ti.get_id();
//   uint64_t CPL = ti.get_critical_path_length();
//   uint64_t intra_interf = intra_interference(ti);
//   uint p_num = processors.get_processor_num();

//   uint64_t test_end = ti.get_deadline();
//   uint64_t test_start = CPL + intra_interf / p_num;
//   uint64_t response = test_start;
//   uint64_t demand = 0;

//   while (response <= test_end) {
//     uint64_t inter_interf = inter_interference(ti, response);
//     demand = test_start + inter_interf / p_num;

//     if (response >= demand) {
//       return response;
//     } else {
//       response = demand;
//     }
//   }
//   return test_end + 100;
// }


ulong LP_RTA_GFP_SPIN_XU_UNOR::get_response_time(const DAG_Task& ti) {
  uint64_t C_i = ti.get_wcet();
  uint64_t L_i = ti.get_critical_path_length();
  uint64_t D_i = ti.get_deadline();
  // uint64_t B_i = total_resource_delay(ti);
  uint32_t p_num = processors.get_processor_num();
  uint64_t test_start = L_i + (C_i - L_i)/p_num  + resource_delay(ti)/p_num;
  // cout << "total resource delay:" << B_i << endl;
  // cout << "test start:" << test_start << endl;
  uint64_t response_time = test_start;

  while (response_time <= D_i) {
    uint64_t update = test_start;
    uint64_t interf = 0;
    foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
      // interf += workload_bound((*tx), D_i);
      interf += workload_bound((*tx), response_time);
    }
    update += interf/p_num;

    if (update > response_time) {
      response_time = update;
      // ti.set_response_time(response_time);
    } else {
      return response_time;
    }

  }

  return D_i + 100;
}

uint64_t LP_RTA_GFP_SPIN_XU_UNOR::workload_bound(const DAG_Task& tx, int64_t interval) {
  int64_t L_x = tx.get_critical_path_length();
  int64_t T_x = tx.get_period();
  int64_t W_x = tx.get_wcet();
  int64_t delta_C = interval - max((int64_t)0, (interval - L_x)/ T_x) * T_x;
  int64_t bound = max((int64_t)0, (interval - delta_C)/ T_x) * W_x + (int64_t)get_WC(tx, delta_C);
  return bound;
}

void LP_RTA_GFP_SPIN_XU_UNOR::declare_variable_bounds(const DAG_Task& ti, LinearProgram* lp,
                                      GFP_SPIN_XUMapper* vars) {
  int64_t var_id; 
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();

  foreach(resources.get_resources(), l_q) {
    uint q = l_q->get_resource_id();
    if (!ti.is_request_exist(q))
      continue;
    var_id = vars->lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
    lp->declare_variable_integer(var_id);
    lp->declare_variable_bounds(var_id, true, 0, true, ti.get_request_by_id(q).get_num_requests());
  }

  // foreach(resources.get_resources(), l_q) {
  //   uint q = l_q->get_resource_id();
  //   if (!ti.is_request_exist(q))
  //     continue;
  //   LinearExpression* exp = new LinearExpression();
  //   var_id = vars->lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
  //   exp->add_term(var_id);
  //   lp->add_inequality(exp, ti.get_request_by_id(q).get_num_requests());
  // }

  foreach(tasks.get_tasks(), tj) {
    uint j = tj->get_id();
    foreach(resources.get_resources(), l_q) {
      uint q = l_q->get_resource_id();
      if (!tj->is_request_exist(q))
        continue;
      int32_t L_j_q = tj->get_request_by_id(q).get_max_length();
      if (j == ti.get_id()) {
        request_num_j = tj->get_request_by_id(q).get_num_requests();
      } else {
        request_num_j = tj->get_max_request_num(q, ti.get_response_time());
      }
      for (uint k = 1; k <= request_num_j; k++) {
        var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
        lp->declare_variable_bounds(var_id, true, 0, true, 1);

        for (uint u = 1; u <= p_num; u++) {
          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          lp->declare_variable_bounds(var_id, true, 0, true, 1);

          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
          lp->declare_variable_bounds(var_id, true, 0, true, 1);
        }
      }
    }
  }
}

void LP_RTA_GFP_SPIN_XU_UNOR::objective(const DAG_Task& ti, GFP_SPIN_XUMapper* vars,
                        LinearExpression* obj) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();
  foreach(tasks.get_tasks(), tj) {
    int m_i_j;
    if (tj->get_priority() <= ti.get_priority())
      m_i_j = p_num - 1;
    else
      m_i_j = p_num;
    uint j = tj->get_id();
    foreach(resources.get_resources(), l_q) {
      uint q = l_q->get_resource_id();
      if (!tj->is_request_exist(q))
        continue;
      int32_t L_j_q = tj->get_request_by_id(q).get_max_length();
      if (j == ti.get_id()) {
        request_num_j = tj->get_request_by_id(q).get_num_requests();
      } else {
        request_num_j = tj->get_max_request_num(q, ti.get_response_time());
      }
      for (uint k = 1; k <= request_num_j; k++) {
        var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
        obj->add_term(var_id, m_i_j * L_j_q);

        for (uint u = 1; u <= p_num; u++) {
          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          obj->add_term(var_id, L_j_q);

          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
          obj->add_term(var_id, L_j_q);
        }
      }
    }
  }
}

void LP_RTA_GFP_SPIN_XU_UNOR::add_constraints(const DAG_Task& ti, LinearProgram* lp,
                              GFP_SPIN_XUMapper* vars) {
// cout << "C1" << endl;
  constraint_1(ti, lp, vars);
// cout << "C2" << endl;
  constraint_2(ti, lp, vars);
// cout << "C3" << endl;
  constraint_3(ti, lp, vars);
// cout << "C4" << endl;
  constraint_4(ti, lp, vars);
// cout << "C5" << endl;
  constraint_5(ti, lp, vars);
// cout << "C6" << endl;
  // constraint_6(ti, lp, vars);
}

void LP_RTA_GFP_SPIN_XU_UNOR::constraint_1(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();


  foreach(tasks.get_tasks(), tj) {
    uint j = tj->get_id();
    foreach(resources.get_resources(), l_q) {
      uint q = l_q->get_resource_id();
      if (!tj->is_request_exist(q))
        continue;
      if (j == ti.get_id())
        request_num_j = tj->get_request_by_id(q).get_num_requests();
      else 
        request_num_j = tj->get_max_request_num(q, ti.get_response_time());

      for (uint k = 1; k <= request_num_j; k++) {

        for (uint u = 1; u <= p_num; u++) {
          LinearExpression *exp = new LinearExpression(); 

          var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
          exp->add_term(var_id, 1);

          lp->add_inequality(exp, 1);
        }
      }
    }
  }
}

// 
void LP_RTA_GFP_SPIN_XU_UNOR::constraint_2(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint32_t request_num_j;

  foreach(resources.get_resources(), l_q) {
      uint32_t q = l_q->get_resource_id();
      if (!ti.is_request_exist(q))
        continue;
      int32_t sum = 0;
      LinearExpression *exp = new LinearExpression(); 

      foreach(tasks.get_tasks(), tj) {
        uint32_t j = tj->get_id();
        if (!tj->is_request_exist(q))
          continue;
        if (j == ti.get_id())
          request_num_j = tj->get_request_by_id(q).get_num_requests();
        else 
          request_num_j = tj->get_max_request_num(q, ti.get_response_time());
        sum += request_num_j;

        for (uint32_t k = 1; k <= request_num_j; k++) {
          var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
          exp->add_term(var_id, 1);
        }
      }

      var_id = vars->lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
      exp->add_term(var_id, -1 * sum);

      lp->add_inequality(exp, 0);
  }
}

// 
void LP_RTA_GFP_SPIN_XU_UNOR::constraint_3(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint32_t i = ti.get_id();
  uint32_t request_num_i;
  int32_t p_num = processors.get_processor_num();

  foreach(resources.get_resources(), l_q) {
      uint32_t q = l_q->get_resource_id();
      if (!ti.is_request_exist(q))
        continue;
      for (uint32_t u = 1; u <= p_num; u++) {
        LinearExpression *exp = new LinearExpression(); 
        request_num_i = ti.get_request_by_id(q).get_num_requests();
        for (uint32_t k = 1; k <= request_num_i; k++) {
          var_id = vars->lookup(i, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(i, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(i, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
          exp->add_term(var_id, 1);
        }

        var_id = vars->lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
        exp->add_term(var_id, 1);

        lp->add_inequality(exp, request_num_i);
      }
  }
}

// 
void LP_RTA_GFP_SPIN_XU_UNOR::constraint_4(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint32_t request_num_j;
  int32_t p_num = processors.get_processor_num();

  foreach(tasks.get_tasks(), tj) {
    uint32_t j = tj->get_id();
    if (tj->get_priority() > ti.get_priority())
      continue;
    foreach(resources.get_resources(), l_q) {
      uint32_t q = l_q->get_resource_id();
      if (!tj->is_request_exist(q))
        continue;
      if (j == ti.get_id())
          request_num_j = tj->get_request_by_id(q).get_num_requests();
        else 
          request_num_j = tj->get_max_request_num(q, ti.get_response_time());
      LinearExpression *exp = new LinearExpression(); 
      for (uint32_t k = 1; k <= request_num_j; k++) {
        // LinearExpression *exp = new LinearExpression(); 

        var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
        exp->add_term(var_id, p_num - 1);

        for (uint32_t u = 1; u <= p_num; u++) {
          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
          exp->add_term(var_id, 1);
        }
        // lp->add_inequality(exp, (p_num - 1) * request_num_j);
      }
      lp->add_inequality(exp, (p_num - 1) * request_num_j);
    }
  }
}

// 
void LP_RTA_GFP_SPIN_XU_UNOR::constraint_5(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();

  foreach(tasks.get_tasks(), tj) {
    uint32_t j = tj->get_id();
    if (tj->get_priority() <= ti.get_priority())
      continue;
    foreach(resources.get_resources(), l_q) {
      uint32_t q = l_q->get_resource_id();
      if (!tj->is_request_exist(q))
        continue;
      request_num_j = tj->get_max_request_num(q, ti.get_response_time());
      LinearExpression *exp = new LinearExpression(); 
      for (uint32_t k = 1; k <= request_num_j; k++) {
        // LinearExpression *exp = new LinearExpression(); 

        var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
        exp->add_term(var_id, p_num);

        for (uint32_t u = 1; u <= p_num; u++) {
          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
          exp->add_term(var_id, 1);
        }
        // lp->add_inequality(exp, p_num * request_num_j);
      }
      lp->add_inequality(exp, p_num * request_num_j);
    }
  }
}

// 
void LP_RTA_GFP_SPIN_XU_UNOR::constraint_6(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();

  foreach(resources.get_resources(), l_q) {
      uint32_t q = l_q->get_resource_id();
      int32_t sum_n = 0;
      int64_t sum = 0, sum_l = 0;
      LinearExpression *exp = new LinearExpression(); 
      vector<uint64_t> temp;

      foreach(tasks.get_tasks(), tj) {
        uint32_t j = tj->get_id();
        if (!tj->is_request_exist(q))
          continue;
        int32_t L_j_q = tj->get_request_by_id(q).get_max_length();
        if (j == ti.get_id())
          request_num_j = tj->get_request_by_id(q).get_num_requests();
        else 
          request_num_j = tj->get_max_request_num(q, ti.get_response_time());
        sum_n += request_num_j;
        sum += request_num_j * L_j_q;
        if (tj->get_priority() > ti.get_priority()) {
          sum_l += request_num_j * L_j_q;
        }
        for (uint32_t k = 1; k <= request_num_j; k++) {
          temp.push_back(L_j_q);
          var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
          exp->add_term(var_id, L_j_q);

          for (uint32_t u = 1; u <= p_num; u++) {
            var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
            exp->add_term(var_id, L_j_q);

            var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
            exp->add_term(var_id, L_j_q);
          }
        }
      }
      std::sort(temp.begin(), temp.end());

      int32_t a_i_q = min(sum_n, p_num);
      int32_t b_i_q = sum_n - a_i_q;
      uint64_t A_sum = 0;
      int64_t A_i_q = 0;
      for (uint p = 0; p < a_i_q; p++) {
        A_sum += temp[p];
        A_i_q += (a_i_q - p - 1) * temp[a_i_q - p];
      }
      // cout << "p_num:" <<  p_num << endl;
      // cout << "sum_n:" <<  sum_n << endl;
      // cout << "sum_l:" <<  sum_l << endl;
      // cout << "sum:" <<  sum << endl;

      // cout << "a_i_q:" <<  a_i_q << endl;
      // cout << "A_sum:" <<  A_sum << endl;
      // cout << "A_i_q:" <<  A_i_q << endl;
      double coef = (p_num - 1) * (sum - A_sum) + A_i_q + sum_l;

      // assert(coef >=0);

      lp->add_inequality(exp, coef);
  }
}



/** Class LP_RTA_GFP_SPIN_XU_FIFO */

LP_RTA_GFP_SPIN_XU_FIFO::LP_RTA_GFP_SPIN_XU_FIFO()
    : RTA_DAG_GFP_JOSE() {}

LP_RTA_GFP_SPIN_XU_FIFO::LP_RTA_GFP_SPIN_XU_FIFO(DAG_TaskSet tasks, ProcessorSet processors,
                                 ResourceSet resources)
    : RTA_DAG_GFP_JOSE(tasks, processors, resources) {}

LP_RTA_GFP_SPIN_XU_FIFO::~LP_RTA_GFP_SPIN_XU_FIFO() {}


bool LP_RTA_GFP_SPIN_XU_FIFO::is_schedulable() {
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

      if (response_time <= ti->get_deadline()) {
        ti->set_response_time(response_time);
      } else {
        return false;
      }
    }
  } while (update);

  // foreach(tasks.get_tasks(), ti) {
  //   uint64_t TRD = resource_delay((*ti))/processors.get_processor_num();
  //   cout << "TRD:" << TRD << endl;
  // }

  return true;
}

// bool LP_RTA_GFP_SPIN_XU_FIFO::is_schedulable() {
//   bool update = false;

//   foreach(tasks.get_tasks(), ti) {
//     ti->set_response_time(ti->get_critical_path_length());
//   }

//   foreach(tasks.get_tasks(), ti) {
//     ulong response_time = get_response_time(*ti);

//     if (response_time <= ti->get_deadline()) {
//       ti->set_response_time(response_time);
//     } else {
//       return false;
//     }
//   }

//   return true;
// }

uint64_t LP_RTA_GFP_SPIN_XU_FIFO::resource_delay(const DAG_Task& ti) {
  uint64_t max_delay;
  uint32_t p_num = processors.get_processor_num();
  GFP_SPIN_XUMapper vars;
  LinearProgram delay;
  LinearExpression* obj = new LinearExpression();
  objective(ti, &vars, obj);
  delay.set_objective(obj);
  declare_variable_bounds(ti, &delay, &vars);
  vars.seal();
  add_constraints(ti, &delay, &vars);

  GLPKSolution* rb_solution =
      new GLPKSolution(delay, vars.get_num_vars());
  // cout << "max_var_num:" << vars.get_num_vars() << endl;

  if (rb_solution->is_solved()) {
    int64_t var_id; 
    // foreach(resources.get_resources(), l_q) {
    //   uint q = l_q->get_resource_id();
    //   if (!ti.is_request_exist(q))
    //     continue;

    //   LinearExpression* exp = new LinearExpression();
    //   var_id = vars.lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
    //   exp->add_term(var_id,1);
    //   cout << "Y[" << q << "]" << rb_solution->evaluate(*exp) << endl; 
    //   delete exp;
    // }

    // LinearExpression* exp = new LinearExpression();
    // foreach(tasks.get_tasks(), tj) {
    //   uint j = tj->get_id();
    //   uint32_t request_num_j;
    //   foreach(resources.get_resources(), l_q) {
    //     uint q = l_q->get_resource_id();
    //     if (!tj->is_request_exist(q))
    //       continue;
    //     int32_t L_j_q = tj->get_request_by_id(q).get_max_length();
    //     if (j == ti.get_id()) {
    //       request_num_j = tj->get_request_by_id(q).get_num_requests();
    //     } else {
    //       request_num_j = tj->get_max_request_num(q, ti.get_response_time());
    //     }
    //     for (uint k = 1; k <= request_num_j; k++) {
    //       var_id = vars.lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
    //       exp->add_term(var_id, L_j_q);
    //     }
    //   }
    // }
    // cout << "BKP:" << rb_solution->evaluate(*exp) << endl; 

    double result = rb_solution->evaluate(*(delay.get_objective()));
    delete rb_solution;
    max_delay = result;
  } else {
    cout << "[LP_RTA_GFP_SPIN_XU_FIFO]unsolved." << endl;
    // cout << "LP solver encounters a problem at T[" << ti.get_id() << "] l[" << res_id << "] X[" << x << "]" << endl;
    // tasks.display();
    rb_solution->show_error();
    if (GLP_FEAS == rb_solution->get_status()) {
      max_delay = rb_solution->evaluate(*(delay.get_objective()));
    } else {
      delete rb_solution;
      exit(1);
      return ti.get_deadline() * p_num;;
    }

    // continue;
    // exit(1);
    // return ti.get_deadline() * p_num;
    // return MAX_LONG;
  }

#if GLPK_MEM_USAGE_CHECK == 1
  int peak;
  glp_mem_usage(NULL, &peak, NULL, NULL);
  cout << "Peak memory usage:" << peak << endl;
#endif

  return max_delay;
}

// ulong LP_RTA_GFP_SPIN_XU_FIFO::get_response_time(const DAG_Task& ti) {
//   uint64_t C_i = ti.get_wcet();
//   uint64_t L_i = ti.get_critical_path_length();
//   uint64_t D_i = ti.get_deadline();
//   // uint64_t B_i = total_resource_delay(ti);
//   uint32_t p_num = processors.get_processor_num();
//   uint64_t test_start = L_i + (C_i - L_i)/p_num;
//   // cout << "total resource delay:" << B_i << endl;
//   // cout << "test start:" << test_start << endl;
//   uint64_t response_time = L_i + (C_i - L_i)/p_num;

//   while (response_time <= D_i) {
//     uint64_t update = response_time;
//     uint64_t interf = 0;
//     foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
//       // interf += workload_bound((*tx), D_i);
//       interf += workload_bound((*tx), response_time);
//     }
//     update += interf/p_num + total_resource_delay(ti);

//     if (update > response_time) {
//       response_time = update;
//       ti.set_response_time(response_time);
//     } else {
//       return response_time;
//     }

//   }

//   // uint64_t interf = 0;
//   // foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
//   //   interf += workload_bound((*tx), D_i);
//   //   // interf += workload_bound((*tx), response_time);
//   // }
//   // response_time += interf/p_num;

//   // if (response_time <= D_i) {
//   //   uint64_t B_i = resource_delay(ti)/p_num;
//   //   cout << "[UNOR]TRD:" << B_i << endl;
//   //   response_time += B_i;
//   // }

//   return D_i + 100;
// }

ulong LP_RTA_GFP_SPIN_XU_FIFO::get_response_time(const DAG_Task& ti) {
  uint64_t C_i = ti.get_wcet();
  uint64_t L_i = ti.get_critical_path_length();
  uint64_t D_i = ti.get_deadline();
  // uint64_t B_i = total_resource_delay(ti);
  uint32_t p_num = processors.get_processor_num();
  uint64_t test_start = L_i + (C_i - L_i)/p_num  + resource_delay(ti)/p_num;
  // cout << "total resource delay:" << B_i << endl;
  // cout << "test start:" << test_start << endl;
  uint64_t response_time = test_start;

  while (response_time <= D_i) {
    uint64_t update = test_start;
    uint64_t interf = 0;
    foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
      // interf += workload_bound((*tx), D_i);
      interf += workload_bound((*tx), response_time);
    }
    update += interf/p_num;

    if (update > response_time) {
      response_time = update;
      // ti.set_response_time(response_time);
    } else {
      return response_time;
    }

  }

  return D_i + 100;
}

// ulong LP_RTA_GFP_SPIN_XU_FIFO::get_response_time(const DAG_Task& ti) {
//   uint64_t C_i = ti.get_wcet();
//   uint64_t L_i = ti.get_critical_path_length();
//   uint64_t D_i = ti.get_deadline();
//   // uint64_t B_i = total_resource_delay(ti);
//   uint32_t p_num = processors.get_processor_num();
//   // uint64_t test_start = L_i + B_i + (C_i - L_i)/p_num;
//   // cout << "total resource delay:" << B_i << endl;
//   // cout << "test start:" << test_start << endl;
//   uint64_t response_time = L_i + (C_i - L_i)/p_num;

//   uint64_t interf = 0;
//   foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
//     interf += workload_bound((*tx), D_i);
//     // interf += workload_bound((*tx), response_time);
//   }
//   response_time += interf/p_num;

//   // cout << "[FIFO]R_nb:" << response_time << endl;

//   if (response_time <= D_i) {
//     uint64_t B_i = resource_delay(ti)/p_num;
//     cout << "[FIFO]TRD:" << B_i << endl;
//     response_time += B_i;
//   }

//   return response_time;
// }

uint64_t LP_RTA_GFP_SPIN_XU_FIFO::workload_bound(const DAG_Task& tx, int64_t interval) {
  int64_t L_x = tx.get_critical_path_length();
  int64_t T_x = tx.get_period();
  int64_t W_x = tx.get_wcet();
  int64_t delta_C = interval - max((int64_t)0, (interval - L_x)/ T_x) * T_x;
  int64_t bound = max((int64_t)0, (interval - delta_C)/ T_x) * W_x + (int64_t)get_WC(tx, delta_C);
  return bound;
}

void LP_RTA_GFP_SPIN_XU_FIFO::declare_variable_bounds(const DAG_Task& ti, LinearProgram* lp,
                                      GFP_SPIN_XUMapper* vars) {
  int64_t var_id; 
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();

  foreach(resources.get_resources(), l_q) {
    uint q = l_q->get_resource_id();
    if (!ti.is_request_exist(q))
      continue;
    var_id = vars->lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
    lp->declare_variable_integer(var_id);
    lp->declare_variable_bounds(var_id, true, 0, true, ti.get_request_by_id(q).get_num_requests());
  }

  // foreach(resources.get_resources(), l_q) {
  //   uint q = l_q->get_resource_id();
  //   if (!ti.is_request_exist(q))
  //     continue;
  //   LinearExpression* exp = new LinearExpression();
  //   var_id = vars->lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
  //   exp->add_term(var_id);
  //   lp->add_inequality(exp, ti.get_request_by_id(q).get_num_requests());
  // }

  foreach(tasks.get_tasks(), tj) {
    uint j = tj->get_id();
    foreach(resources.get_resources(), l_q) {
      uint q = l_q->get_resource_id();
      if (!tj->is_request_exist(q))
        continue;
      int32_t L_j_q = tj->get_request_by_id(q).get_max_length();
      if (j == ti.get_id()) {
        request_num_j = tj->get_request_by_id(q).get_num_requests();
      } else {
        request_num_j = tj->get_max_request_num(q, ti.get_response_time());
      }
      for (uint k = 1; k <= request_num_j; k++) {
        var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
        lp->declare_variable_bounds(var_id, true, 0, true, 1);

        for (uint u = 1; u <= p_num; u++) {
          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          lp->declare_variable_bounds(var_id, true, 0, true, 1);

          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
          lp->declare_variable_bounds(var_id, true, 0, true, 1);
        }
      }
    }
  }
}

void LP_RTA_GFP_SPIN_XU_FIFO::objective(const DAG_Task& ti, GFP_SPIN_XUMapper* vars,
                        LinearExpression* obj) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();
  foreach(tasks.get_tasks(), tj) {
    int m_i_j;
    if (tj->get_priority() <= ti.get_priority())
      m_i_j = p_num - 1;
    else
      m_i_j = p_num;
    uint j = tj->get_id();
    foreach(resources.get_resources(), l_q) {
      uint q = l_q->get_resource_id();
      if (!tj->is_request_exist(q))
        continue;
      int32_t L_j_q = tj->get_request_by_id(q).get_max_length();
      if (j == ti.get_id()) {
        request_num_j = tj->get_request_by_id(q).get_num_requests();
      } else {
        request_num_j = tj->get_max_request_num(q, ti.get_response_time());
      }
      for (uint k = 1; k <= request_num_j; k++) {
        var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
        obj->add_term(var_id, m_i_j * L_j_q);

        for (uint u = 1; u <= p_num; u++) {
          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          obj->add_term(var_id, L_j_q);

          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
          obj->add_term(var_id, L_j_q);
        }
      }
    }
  }
}

void LP_RTA_GFP_SPIN_XU_FIFO::add_constraints(const DAG_Task& ti, LinearProgram* lp,
                              GFP_SPIN_XUMapper* vars) {
  constraint_1(ti, lp, vars);
  constraint_2(ti, lp, vars);
  constraint_3(ti, lp, vars);
  constraint_4(ti, lp, vars);
  constraint_5(ti, lp, vars);
  constraint_6(ti, lp, vars);
  constraint_7(ti, lp, vars);
}

void LP_RTA_GFP_SPIN_XU_FIFO::constraint_1(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();


  foreach(tasks.get_tasks(), tj) {
    uint j = tj->get_id();
    foreach(resources.get_resources(), l_q) {
      uint q = l_q->get_resource_id();
      if (!tj->is_request_exist(q))
        continue;
      if (j == ti.get_id())
        request_num_j = tj->get_request_by_id(q).get_num_requests();
      else 
        request_num_j = tj->get_max_request_num(q, ti.get_response_time());

      for (uint k = 1; k <= request_num_j; k++) {

        for (uint u = 1; u <= p_num; u++) {
          LinearExpression *exp = new LinearExpression(); 

          var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
          exp->add_term(var_id, 1);

          lp->add_inequality(exp, 1);
        }
      }
    }
  }
}

// 
void LP_RTA_GFP_SPIN_XU_FIFO::constraint_2(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint32_t request_num_j;

  foreach(resources.get_resources(), l_q) {
      uint32_t q = l_q->get_resource_id();
      if (!ti.is_request_exist(q))
        continue;
      int32_t sum = 0;
      LinearExpression *exp = new LinearExpression(); 

      foreach(tasks.get_tasks(), tj) {
        uint32_t j = tj->get_id();
        if (!tj->is_request_exist(q))
          continue;
        if (j == ti.get_id())
          request_num_j = tj->get_request_by_id(q).get_num_requests();
        else 
          request_num_j = tj->get_max_request_num(q, ti.get_response_time());
        sum += request_num_j;

        for (uint32_t k = 1; k <= request_num_j; k++) {
          var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
          exp->add_term(var_id, 1);
        }
      }

      var_id = vars->lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
      exp->add_term(var_id, -1 * sum);

      lp->add_inequality(exp, 0);
  }
}

// 
void LP_RTA_GFP_SPIN_XU_FIFO::constraint_3(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint32_t i = ti.get_id();
  uint32_t request_num_i;
  int32_t p_num = processors.get_processor_num();

  foreach(resources.get_resources(), l_q) {
      uint32_t q = l_q->get_resource_id();
      if (!ti.is_request_exist(q))
        continue;
      for (uint32_t u = 1; u <= p_num; u++) {
        LinearExpression *exp = new LinearExpression(); 
        request_num_i = ti.get_request_by_id(q).get_num_requests();
        for (uint32_t k = 1; k <= request_num_i; k++) {
          var_id = vars->lookup(i, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(i, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(i, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
          exp->add_term(var_id, 1);
        }

        var_id = vars->lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
        exp->add_term(var_id, 1);

        lp->add_inequality(exp, request_num_i);
      }
  }
}

// 
void LP_RTA_GFP_SPIN_XU_FIFO::constraint_4(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint32_t request_num_j;
  int32_t p_num = processors.get_processor_num();

  foreach(tasks.get_tasks(), tj) {
    uint32_t j = tj->get_id();
    if (tj->get_priority() > ti.get_priority())
      continue;
    foreach(resources.get_resources(), l_q) {
      uint32_t q = l_q->get_resource_id();
      if (!tj->is_request_exist(q))
        continue;
      if (j == ti.get_id())
          request_num_j = tj->get_request_by_id(q).get_num_requests();
        else 
          request_num_j = tj->get_max_request_num(q, ti.get_response_time());
      LinearExpression *exp = new LinearExpression(); 
      for (uint32_t k = 1; k <= request_num_j; k++) {
        // LinearExpression *exp = new LinearExpression(); 

        var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
        exp->add_term(var_id, p_num - 1);

        for (uint32_t u = 1; u <= p_num; u++) {
          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
          exp->add_term(var_id, 1);
        }
        // lp->add_inequality(exp, (p_num - 1) * request_num_j);
      }
      lp->add_inequality(exp, (p_num - 1) * request_num_j);
    }
  }
}

// 
void LP_RTA_GFP_SPIN_XU_FIFO::constraint_5(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();

  foreach(tasks.get_tasks(), tj) {
    uint32_t j = tj->get_id();
    if (tj->get_priority() <= ti.get_priority())
      continue;
    foreach(resources.get_resources(), l_q) {
      uint32_t q = l_q->get_resource_id();
      if (!tj->is_request_exist(q))
        continue;
      request_num_j = tj->get_max_request_num(q, ti.get_response_time());
      LinearExpression *exp = new LinearExpression(); 
      for (uint32_t k = 1; k <= request_num_j; k++) {
        // LinearExpression *exp = new LinearExpression(); 

        var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
        exp->add_term(var_id, p_num);

        for (uint32_t u = 1; u <= p_num; u++) {
          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
          exp->add_term(var_id, 1);
        }
        // lp->add_inequality(exp, p_num * request_num_j);
      }
      lp->add_inequality(exp, p_num * request_num_j);
    }
  }
}

// 
void LP_RTA_GFP_SPIN_XU_FIFO::constraint_6(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();

  foreach(resources.get_resources(), l_q) {
      uint32_t q = l_q->get_resource_id();
      int32_t sum_n = 0;
      int64_t sum = 0, sum_l = 0;
      LinearExpression *exp = new LinearExpression(); 
      vector<uint64_t> temp;

      foreach(tasks.get_tasks(), tj) {
        uint32_t j = tj->get_id();
        if (!tj->is_request_exist(q))
          continue;
        int32_t L_j_q = tj->get_request_by_id(q).get_max_length();
        if (j == ti.get_id())
          request_num_j = tj->get_request_by_id(q).get_num_requests();
        else 
          request_num_j = tj->get_max_request_num(q, ti.get_response_time());
        sum_n += request_num_j;
        sum += request_num_j * L_j_q;
        if (tj->get_priority() > ti.get_priority()) {
          sum_l += request_num_j * L_j_q;
        }
        for (uint32_t k = 1; k <= request_num_j; k++) {
          temp.push_back(L_j_q);
          var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
          exp->add_term(var_id, L_j_q);

          for (uint32_t u = 1; u <= p_num; u++) {
            var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
            exp->add_term(var_id, L_j_q);

            var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
            exp->add_term(var_id, L_j_q);
          }
        }
      }
      std::sort(temp.begin(), temp.end());

      int32_t a_i_q = min(sum_n, p_num);
      int32_t b_i_q = sum_n - a_i_q;
      uint64_t A_sum = 0;
      int64_t A_i_q = 0;
      for (uint p = 0; p < a_i_q; p++) {
        A_sum += temp[p];
        A_i_q += (a_i_q - p - 1) * temp[a_i_q - p];
      }
      // cout << "p_num:" <<  p_num << endl;
      // cout << "sum_n:" <<  sum_n << endl;
      // cout << "sum_l:" <<  sum_l << endl;
      // cout << "sum:" <<  sum << endl;

      // cout << "a_i_q:" <<  a_i_q << endl;
      // cout << "A_sum:" <<  A_sum << endl;
      // cout << "A_i_q:" <<  A_i_q << endl;
      double coef = (p_num - 1) * (sum - A_sum) + A_i_q + sum_l;

      // assert(coef >=0);

      lp->add_inequality(exp, coef);
  }
}

// 
void LP_RTA_GFP_SPIN_XU_FIFO::constraint_7(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();

  foreach(resources.get_resources(), l_q) {
      uint32_t q = l_q->get_resource_id();
      if (!ti.is_request_exist(q))
        continue;
      int32_t sum_n = 0;
      LinearExpression *exp_1 = new LinearExpression();
      LinearExpression *exp_2 = new LinearExpression(); 
      foreach(tasks.get_tasks(), tj) {
        uint32_t j = tj->get_id();
        if (!tj->is_request_exist(q))
          continue;
        if (j == ti.get_id())
          request_num_j = tj->get_request_by_id(q).get_num_requests();
        else 
          request_num_j = tj->get_max_request_num(q, ti.get_response_time());
        sum_n += request_num_j;
        for (uint32_t k = 1; k <= request_num_j; k++) {
          var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
          exp_1->add_term(var_id, 1);
          exp_2->add_term(var_id, 1);
        }
      }

      var_id = vars->lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
      exp_1->add_term(var_id, 1 - p_num);
      lp->add_inequality(exp_1, 0);

      var_id = vars->lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
      exp_2->add_term(var_id, 1);
      lp->add_inequality(exp_2, sum_n);
  }
}


// 
// void LP_RTA_GFP_SPIN_XU_FIFO::constraint_7_2(const DAG_Task& ti, LinearProgram* lp,
//                           GFP_SPIN_XUMapper* vars) {
//   int64_t var_id;
//   uint request_num_j;
//   int32_t p_num = processors.get_processor_num();

//   foreach(resources.get_resources(), l_q) {
//       uint32_t q = l_q->get_resource_id();
//       if (!ti.is_request_exist(q))
//         continue;
//       int32_t sum_n = 0;
//       // LinearExpression *exp_1 = new LinearExpression();
//       LinearExpression *exp_2 = new LinearExpression(); 
//       foreach(tasks.get_tasks(), tj) {
//         uint32_t j = tj->get_id();
//         if (!tj->is_request_exist(q))
//           continue;
//         if (j == ti.get_id())
//           request_num_j = tj->get_request_by_id(q).get_num_requests();
//         else 
//           request_num_j = tj->get_max_request_num(q, ti.get_response_time());
//         sum_n += request_num_j;
//         for (uint32_t k = 1; k <= request_num_j; k++) {
//           var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
//           // exp_1->add_term(var_id, 1);
//           exp_2->add_term(var_id, 1);
//         }
//       }

//       // var_id = vars->lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
//       // exp_1->add_term(var_id, 1 - p_num);
//       // lp->add_inequality(exp_1, 0);

//       var_id = vars->lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
//       exp_2->add_term(var_id, 1);
//       lp->add_inequality(exp_2, sum_n);
//   }
// }

/** Class LP_RTA_GFP_SPIN_XU_PRIO */

LP_RTA_GFP_SPIN_XU_PRIO::LP_RTA_GFP_SPIN_XU_PRIO()
    : RTA_DAG_GFP_JOSE() {}

LP_RTA_GFP_SPIN_XU_PRIO::LP_RTA_GFP_SPIN_XU_PRIO(DAG_TaskSet tasks, ProcessorSet processors,
                                 ResourceSet resources)
    : RTA_DAG_GFP_JOSE(tasks, processors, resources) {}

LP_RTA_GFP_SPIN_XU_PRIO::~LP_RTA_GFP_SPIN_XU_PRIO() {}

bool LP_RTA_GFP_SPIN_XU_PRIO::is_schedulable() {
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

      if (response_time <= ti->get_deadline()) {
        ti->set_response_time(response_time);
      } else {
        return false;
      }
    }
  } while (update);

  // foreach(tasks.get_tasks(), ti) {
  //   uint64_t TRD = resource_delay((*ti))/processors.get_processor_num();
  //   cout << "TRD:" << TRD << endl;
  // }

  return true;
}

uint64_t LP_RTA_GFP_SPIN_XU_PRIO::higher(const DAG_Task& ti, uint32_t res_id,
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

uint64_t LP_RTA_GFP_SPIN_XU_PRIO::lower(const DAG_Task& ti,
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

uint64_t LP_RTA_GFP_SPIN_XU_PRIO::equal(const DAG_Task& ti,
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

uint64_t LP_RTA_GFP_SPIN_XU_PRIO::dpr(const DAG_Task& ti, uint32_t res_id) {
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

uint64_t LP_RTA_GFP_SPIN_XU_PRIO::resource_delay(const DAG_Task& ti) {
  uint64_t max_delay;
  uint32_t p_num = processors.get_processor_num();
  GFP_SPIN_XUMapper vars;
  LinearProgram delay;
  LinearExpression* obj = new LinearExpression();
  objective(ti, &vars, obj);
  delay.set_objective(obj);
  declare_variable_bounds(ti, &delay, &vars);
  vars.seal();
  add_constraints(ti, &delay, &vars);

  GLPKSolution* rb_solution =
      new GLPKSolution(delay, vars.get_num_vars());
  // cout << "max_var_num:" << vars.get_num_vars() << endl;

  if (rb_solution->is_solved()) {
    int64_t var_id; 
    // foreach(resources.get_resources(), l_q) {
    //   uint q = l_q->get_resource_id();
    //   if (!ti.is_request_exist(q))
    //     continue;

    //   LinearExpression* exp = new LinearExpression();
    //   var_id = vars.lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
    //   exp->add_term(var_id,1);
    //   cout << "Y[" << q << "]" << rb_solution->evaluate(*exp) << endl; 
    //   delete exp;
    // }

    // LinearExpression* exp = new LinearExpression();
    // foreach(tasks.get_tasks(), tj) {
    //   uint j = tj->get_id();
    //   uint32_t request_num_j;
    //   foreach(resources.get_resources(), l_q) {
    //     uint q = l_q->get_resource_id();
    //     if (!tj->is_request_exist(q))
    //       continue;
    //     int32_t L_j_q = tj->get_request_by_id(q).get_max_length();
    //     if (j == ti.get_id()) {
    //       request_num_j = tj->get_request_by_id(q).get_num_requests();
    //     } else {
    //       request_num_j = tj->get_max_request_num(q, ti.get_response_time());
    //     }
    //     for (uint k = 1; k <= request_num_j; k++) {
    //       var_id = vars.lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
    //       exp->add_term(var_id, L_j_q);
    //     }
    //   }
    // }
    // cout << "BKP:" << rb_solution->evaluate(*exp) << endl; 
    
    double result = rb_solution->evaluate(*(delay.get_objective()));
    delete rb_solution;
    max_delay = result;
  } else {
    cout << "[LP_RTA_GFP_SPIN_XU_PRIO]unsolved." << endl;
    // cout << "LP solver encounters a problem at T[" << ti.get_id() << "] l[" << res_id << "] X[" << x << "]" << endl;
    // tasks.display();
    rb_solution->show_error();
    if (GLP_FEAS == rb_solution->get_status()) {
      max_delay = rb_solution->evaluate(*(delay.get_objective()));
    } else {
      delete rb_solution;
      exit(1);
      return ti.get_deadline() * p_num;;
    }

    // continue;
    // exit(1);
    // return ti.get_deadline() * p_num;
    // return MAX_LONG;
  }

#if GLPK_MEM_USAGE_CHECK == 1
  int peak;
  glp_mem_usage(NULL, &peak, NULL, NULL);
  cout << "Peak memory usage:" << peak << endl;
#endif

  return max_delay;
}

ulong LP_RTA_GFP_SPIN_XU_PRIO::get_response_time(const DAG_Task& ti) {
  uint64_t C_i = ti.get_wcet();
  uint64_t L_i = ti.get_critical_path_length();
  uint64_t D_i = ti.get_deadline();
  // uint64_t B_i = total_resource_delay(ti);
  uint32_t p_num = processors.get_processor_num();
  uint64_t test_start = L_i + (C_i - L_i)/p_num  + resource_delay(ti)/p_num;
  // cout << "total resource delay:" << B_i << endl;
  // cout << "test start:" << test_start << endl;
  uint64_t response_time = test_start;

  while (response_time <= D_i) {
    uint64_t update = test_start;
    uint64_t interf = 0;
    foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
      // interf += workload_bound((*tx), D_i);
      interf += workload_bound((*tx), response_time);
    }
    update += interf/p_num;

    if (update > response_time) {
      response_time = update;
      // ti.set_response_time(response_time);
    } else {
      return response_time;
    }

  }

  return D_i + 100;
}

uint64_t LP_RTA_GFP_SPIN_XU_PRIO::workload_bound(const DAG_Task& tx, int64_t interval) {
  int64_t L_x = tx.get_critical_path_length();
  int64_t T_x = tx.get_period();
  int64_t W_x = tx.get_wcet();
  int64_t delta_C = interval - max((int64_t)0, (interval - L_x)/ T_x) * T_x;
  int64_t bound = max((int64_t)0, (interval - delta_C)/ T_x) * W_x + (int64_t)get_WC(tx, delta_C);
  return bound;
}

void LP_RTA_GFP_SPIN_XU_PRIO::declare_variable_bounds(const DAG_Task& ti, LinearProgram* lp,
                                      GFP_SPIN_XUMapper* vars) {
  int64_t var_id; 
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();

  foreach(resources.get_resources(), l_q) {
    uint q = l_q->get_resource_id();
    if (!ti.is_request_exist(q))
      continue;
    var_id = vars->lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
    lp->declare_variable_integer(var_id);
    lp->declare_variable_bounds(var_id, true, 0, true, ti.get_request_by_id(q).get_num_requests());
  }

  // foreach(resources.get_resources(), l_q) {
  //   uint q = l_q->get_resource_id();
  //   if (!ti.is_request_exist(q))
  //     continue;
  //   LinearExpression* exp = new LinearExpression();
  //   var_id = vars->lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
  //   exp->add_term(var_id);
  //   lp->add_inequality(exp, ti.get_request_by_id(q).get_num_requests());
  // }

  foreach(tasks.get_tasks(), tj) {
    uint j = tj->get_id();
    foreach(resources.get_resources(), l_q) {
      uint q = l_q->get_resource_id();
      if (!tj->is_request_exist(q))
        continue;
      int32_t L_j_q = tj->get_request_by_id(q).get_max_length();
      if (j == ti.get_id()) {
        request_num_j = tj->get_request_by_id(q).get_num_requests();
      } else {
        request_num_j = tj->get_max_request_num(q, ti.get_response_time());
      }
      for (uint k = 1; k <= request_num_j; k++) {
        var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
        lp->declare_variable_bounds(var_id, true, 0, true, 1);

        for (uint u = 1; u <= p_num; u++) {
          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          lp->declare_variable_bounds(var_id, true, 0, true, 1);

          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
          lp->declare_variable_bounds(var_id, true, 0, true, 1);
        }
      }
    }
  }
}

void LP_RTA_GFP_SPIN_XU_PRIO::objective(const DAG_Task& ti, GFP_SPIN_XUMapper* vars,
                        LinearExpression* obj) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();
  foreach(tasks.get_tasks(), tj) {
    int m_i_j;
    if (tj->get_priority() <= ti.get_priority())
      m_i_j = p_num - 1;
    else
      m_i_j = p_num;
    uint j = tj->get_id();
    foreach(resources.get_resources(), l_q) {
      uint q = l_q->get_resource_id();
      if (!tj->is_request_exist(q))
        continue;
      int32_t L_j_q = tj->get_request_by_id(q).get_max_length();
      if (j == ti.get_id()) {
        request_num_j = tj->get_request_by_id(q).get_num_requests();
      } else {
        request_num_j = tj->get_max_request_num(q, ti.get_response_time());
      }
      for (uint k = 1; k <= request_num_j; k++) {
        var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
        obj->add_term(var_id, m_i_j * L_j_q);

        for (uint u = 1; u <= p_num; u++) {
          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          obj->add_term(var_id, L_j_q);

          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
          obj->add_term(var_id, L_j_q);
        }
      }
    }
  }
}

void LP_RTA_GFP_SPIN_XU_PRIO::add_constraints(const DAG_Task& ti, LinearProgram* lp,
                              GFP_SPIN_XUMapper* vars) {
  constraint_1(ti, lp, vars);
  constraint_2(ti, lp, vars);
  constraint_3(ti, lp, vars);
  constraint_4(ti, lp, vars);
  constraint_5(ti, lp, vars);
  constraint_6(ti, lp, vars);
  // cout << "C12" << endl;
  constraint_12(ti, lp, vars);
  // cout << "C13" << endl;
  constraint_13(ti, lp, vars);
}

void LP_RTA_GFP_SPIN_XU_PRIO::constraint_1(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();


  foreach(tasks.get_tasks(), tj) {
    uint j = tj->get_id();
    foreach(resources.get_resources(), l_q) {
      uint q = l_q->get_resource_id();
      if (!tj->is_request_exist(q))
        continue;
      if (j == ti.get_id())
        request_num_j = tj->get_request_by_id(q).get_num_requests();
      else 
        request_num_j = tj->get_max_request_num(q, ti.get_response_time());

      for (uint k = 1; k <= request_num_j; k++) {

        for (uint u = 1; u <= p_num; u++) {
          LinearExpression *exp = new LinearExpression(); 

          var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
          exp->add_term(var_id, 1);

          lp->add_inequality(exp, 1);
        }
      }
    }
  }
}

// 
void LP_RTA_GFP_SPIN_XU_PRIO::constraint_2(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint32_t request_num_j;

  foreach(resources.get_resources(), l_q) {
      uint32_t q = l_q->get_resource_id();
      if (!ti.is_request_exist(q))
        continue;
      int32_t sum = 0;
      LinearExpression *exp = new LinearExpression(); 

      foreach(tasks.get_tasks(), tj) {
        uint32_t j = tj->get_id();
        if (!tj->is_request_exist(q))
          continue;
        if (j == ti.get_id())
          request_num_j = tj->get_request_by_id(q).get_num_requests();
        else 
          request_num_j = tj->get_max_request_num(q, ti.get_response_time());
        sum += request_num_j;

        for (uint32_t k = 1; k <= request_num_j; k++) {
          var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
          exp->add_term(var_id, 1);
        }
      }

      var_id = vars->lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
      exp->add_term(var_id, -1 * sum);

      lp->add_inequality(exp, 0);
  }
}

// 
void LP_RTA_GFP_SPIN_XU_PRIO::constraint_3(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint32_t i = ti.get_id();
  uint32_t request_num_i;
  int32_t p_num = processors.get_processor_num();

  foreach(resources.get_resources(), l_q) {
      uint32_t q = l_q->get_resource_id();
      if (!ti.is_request_exist(q))
        continue;
      for (uint32_t u = 1; u <= p_num; u++) {
        LinearExpression *exp = new LinearExpression(); 
        request_num_i = ti.get_request_by_id(q).get_num_requests();
        for (uint32_t k = 1; k <= request_num_i; k++) {
          var_id = vars->lookup(i, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(i, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(i, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
          exp->add_term(var_id, 1);
        }

        var_id = vars->lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
        exp->add_term(var_id, 1);

        lp->add_inequality(exp, request_num_i);
      }
  }
}

// 
void LP_RTA_GFP_SPIN_XU_PRIO::constraint_4(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint32_t request_num_j;
  int32_t p_num = processors.get_processor_num();

  foreach(tasks.get_tasks(), tj) {
    uint32_t j = tj->get_id();
    if (tj->get_priority() > ti.get_priority())
      continue;
    foreach(resources.get_resources(), l_q) {
      uint32_t q = l_q->get_resource_id();
      if (!tj->is_request_exist(q))
        continue;
      if (j == ti.get_id())
          request_num_j = tj->get_request_by_id(q).get_num_requests();
        else 
          request_num_j = tj->get_max_request_num(q, ti.get_response_time());
      LinearExpression *exp = new LinearExpression(); 
      for (uint32_t k = 1; k <= request_num_j; k++) {

        var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
        exp->add_term(var_id, p_num - 1);

        for (uint32_t u = 1; u <= p_num; u++) {
          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
          exp->add_term(var_id, 1);
        }
        
      }
      lp->add_inequality(exp, (p_num - 1) * request_num_j);
    }
  }
}

// 
void LP_RTA_GFP_SPIN_XU_PRIO::constraint_5(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();

  foreach(tasks.get_tasks(), tj) {
    uint32_t j = tj->get_id();
    if (tj->get_priority() <= ti.get_priority())
      continue;
    foreach(resources.get_resources(), l_q) {
      uint32_t q = l_q->get_resource_id();
      if (!tj->is_request_exist(q))
        continue;
      request_num_j = tj->get_max_request_num(q, ti.get_response_time());
      LinearExpression *exp = new LinearExpression(); 
      for (uint32_t k = 1; k <= request_num_j; k++) {

        var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
        exp->add_term(var_id, p_num);

        for (uint32_t u = 1; u <= p_num; u++) {
          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
          exp->add_term(var_id, 1);
        } 
      }
      lp->add_inequality(exp, p_num * request_num_j);
    }
  }
}

// 
void LP_RTA_GFP_SPIN_XU_PRIO::constraint_6(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();

  foreach(resources.get_resources(), l_q) {
      uint32_t q = l_q->get_resource_id();
      int32_t sum_n = 0;
      int64_t sum = 0, sum_l = 0;
      LinearExpression *exp = new LinearExpression(); 
      vector<uint64_t> temp;

      foreach(tasks.get_tasks(), tj) {
        uint32_t j = tj->get_id();
        if (!tj->is_request_exist(q))
          continue;
        int32_t L_j_q = tj->get_request_by_id(q).get_max_length();
        if (j == ti.get_id())
          request_num_j = tj->get_request_by_id(q).get_num_requests();
        else 
          request_num_j = tj->get_max_request_num(q, ti.get_response_time());
        sum_n += request_num_j;
        sum += request_num_j * L_j_q;
        if (tj->get_priority() > ti.get_priority()) {
          sum_l += request_num_j * L_j_q;
        }
        for (uint32_t k = 1; k <= request_num_j; k++) {
          temp.push_back(L_j_q);
          var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
          exp->add_term(var_id, L_j_q);

          for (uint32_t u = 1; u <= p_num; u++) {
            var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
            exp->add_term(var_id, L_j_q);

            var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
            exp->add_term(var_id, L_j_q);
          }
        }
      }
      std::sort(temp.begin(), temp.end());

      int32_t a_i_q = min(sum_n, p_num);
      int32_t b_i_q = sum_n - a_i_q;
      uint64_t A_sum = 0;
      int64_t A_i_q = 0;
      for (uint p = 0; p < a_i_q; p++) {
        A_sum += temp[p];
        A_i_q += (a_i_q - p - 1) * temp[a_i_q - p];
      }
      // cout << "p_num:" <<  p_num << endl;
      // cout << "sum_n:" <<  sum_n << endl;
      // cout << "sum_l:" <<  sum_l << endl;
      // cout << "sum:" <<  sum << endl;

      // cout << "a_i_q:" <<  a_i_q << endl;
      // cout << "A_sum:" <<  A_sum << endl;
      // cout << "A_i_q:" <<  A_i_q << endl;
      double coef = (p_num - 1) * (sum - A_sum) + A_i_q + sum_l;

      // assert(coef >=0);

      lp->add_inequality(exp, coef);
  }
}

// 
void LP_RTA_GFP_SPIN_XU_PRIO::constraint_12(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();

  foreach(resources.get_resources(), l_q) {
    uint32_t q = l_q->get_resource_id();
    if (!ti.is_request_exist(q))
      continue;
    LinearExpression *exp = new LinearExpression(); 

    foreach_lower_priority_task(tasks.get_tasks(), ti, tj) {
      uint32_t j = tj->get_id();
      if (!tj->is_request_exist(q))
        continue;
      request_num_j = tj->get_max_request_num(q, ti.get_response_time());
      // cout << "request_num_j:" << request_num_j << endl;
      for (uint32_t k = 1; k <= request_num_j; k++) {
        var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
        exp->add_term(var_id, 1);
      }
    }

    var_id = vars->lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
      exp->add_term(var_id, -1);

    lp->add_inequality(exp, 0);
  }
}

// 
void LP_RTA_GFP_SPIN_XU_PRIO::constraint_13(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();

  foreach(resources.get_resources(), l_q) {
    uint32_t q = l_q->get_resource_id();
    if (!ti.is_request_exist(q))
      continue;
    int32_t sum = 0;
    int64_t delta_sum = 0;
    LinearExpression *exp_1 = new LinearExpression(); 
    LinearExpression *exp_2 = new LinearExpression(); 

    foreach(tasks.get_tasks(), tj) {
      uint32_t j = tj->get_id();
      if (!tj->is_request_exist(q))
        continue;
      
      if (j == ti.get_id())
        request_num_j = tj->get_request_by_id(q).get_num_requests();
      else 
        request_num_j = tj->get_max_request_num(q, ti.get_response_time());
      sum += request_num_j;
      // cout << "request_num_j:" << request_num_j << endl;
      if (tj->get_priority() <= ti.get_priority())
        continue;
      for (uint32_t k = 1; k <= request_num_j; k++) {
        var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
        exp_1->add_term(var_id, 1);
        exp_2->add_term(var_id, 1);
      }
      int32_t delta = ceiling(dpr(ti, q) + tj->get_deadline(), tj->get_period()) * tj->get_request_by_id(q).get_num_requests();
      delta_sum += delta;
    }

    var_id = vars->lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
    exp_1->add_term(var_id, 1);
    lp->add_inequality(exp_1, sum);


    var_id = vars->lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
    exp_2->add_term(var_id, -1 * delta_sum);
    lp->add_inequality(exp_2, 0);
  }
}






/** Class LP_RTA_GFP_SPIN_XU_UNOR_P */

LP_RTA_GFP_SPIN_XU_UNOR_P::LP_RTA_GFP_SPIN_XU_UNOR_P()
    : LP_RTA_GFP_SPIN_XU_UNOR() {}

LP_RTA_GFP_SPIN_XU_UNOR_P::LP_RTA_GFP_SPIN_XU_UNOR_P(DAG_TaskSet tasks, ProcessorSet processors,
                                 ResourceSet resources)
    : LP_RTA_GFP_SPIN_XU_UNOR(tasks, processors, resources) {}

LP_RTA_GFP_SPIN_XU_UNOR_P::~LP_RTA_GFP_SPIN_XU_UNOR_P() {}

bool LP_RTA_GFP_SPIN_XU_UNOR_P::is_schedulable() {
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

      if (response_time <= ti->get_deadline()) {
        ti->set_response_time(response_time);
      } else {
        return false;
      }
    }
  } while (update);

  foreach(tasks.get_tasks(), ti) {
    uint64_t TRD = resource_delay((*ti))/processors.get_processor_num();
    cout << "TRD:" << TRD << endl;
  }

  return true;
}


uint64_t LP_RTA_GFP_SPIN_XU_UNOR_P::resource_delay(const DAG_Task& ti) {
  uint64_t max_delay;
  uint32_t p_num = processors.get_processor_num();
  GFP_SPIN_XUMapper vars;
  LinearProgram delay;
  LinearExpression* obj = new LinearExpression();
  objective(ti, &vars, obj);
  delay.set_objective(obj);
  declare_variable_bounds(ti, &delay, &vars);
  vars.seal();
  add_constraints(ti, &delay, &vars);

  GLPKSolution* rb_solution =
      new GLPKSolution(delay, vars.get_num_vars());
  // cout << "max_var_num:" << vars.get_num_vars() << endl;

  if (rb_solution->is_solved()) {
    // int64_t var_id; 
    // foreach(resources.get_resources(), l_q) {
    //   uint q = l_q->get_resource_id();
    //   if (!ti.is_request_exist(q))
    //     continue;

    //   LinearExpression* exp = new LinearExpression();
    //   var_id = vars.lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
    //   exp->add_term(var_id,1);
    //   cout << "Y[" << q << "]" << rb_solution->evaluate(*exp) << endl; 
    //   delete exp;
    // }

    double result = rb_solution->evaluate(*(delay.get_objective()));
    delete rb_solution;
    max_delay = result;
  } else {
    cout << "[LP_RTA_GFP_SPIN_XU_UNOR_P]unsolved." << endl;
    // cout << "LP solver encounters a problem at T[" << ti.get_id() << "] l[" << res_id << "] X[" << x << "]" << endl;
    // tasks.display();
    rb_solution->show_error();
    if (GLP_FEAS == rb_solution->get_status()) {
      max_delay = rb_solution->evaluate(*(delay.get_objective()));
    } else {
      delete rb_solution;
      exit(1);
      return ti.get_deadline() * p_num;;
    }

    // continue;
    // exit(1);
    // return ti.get_deadline() * p_num;
    // return MAX_LONG;
  }

#if GLPK_MEM_USAGE_CHECK == 1
  int peak;
  glp_mem_usage(NULL, &peak, NULL, NULL);
  cout << "Peak memory usage:" << peak << endl;
#endif

  return max_delay;
}

ulong LP_RTA_GFP_SPIN_XU_UNOR_P::get_response_time(const DAG_Task& ti) {
  uint64_t C_i = ti.get_wcet();
  uint64_t L_i = ti.get_critical_path_length();
  uint64_t D_i = ti.get_deadline();
  // uint64_t B_i = total_resource_delay(ti);
  uint32_t p_num = processors.get_processor_num();
  uint64_t test_start = L_i + (C_i - L_i)/p_num  + resource_delay(ti)/p_num;
  // cout << "total resource delay:" << B_i << endl;
  // cout << "test start:" << test_start << endl;
  uint64_t response_time = test_start;

  while (response_time <= D_i) {
    uint64_t update = test_start;
    uint64_t interf = 0;
    foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
      // interf += workload_bound((*tx), D_i);
      interf += workload_bound((*tx), response_time);
    }
    update += interf/p_num;

    if (update > response_time) {
      response_time = update;
      // ti.set_response_time(response_time);
    } else {
      return response_time;
    }

  }

  return D_i + 100;
}

void LP_RTA_GFP_SPIN_XU_UNOR_P::add_constraints(const DAG_Task& ti, LinearProgram* lp,
                              GFP_SPIN_XUMapper* vars) {
// cout << "C1" << endl;
  constraint_1(ti, lp, vars);
// cout << "C2" << endl;
  constraint_2(ti, lp, vars);
// cout << "C3" << endl;
  constraint_3(ti, lp, vars);
// cout << "C4" << endl;
  constraint_4(ti, lp, vars);
// cout << "C5" << endl;
  constraint_5(ti, lp, vars);
// cout << "C6" << endl;
  // constraint_6(ti, lp, vars);
// cout << "C8" << endl;
  constraint_8(ti, lp, vars);
// cout << "C9" << endl;
  constraint_9(ti, lp, vars);
// cout << "C10" << endl;
  constraint_10(ti, lp, vars);
// cout << "C11" << endl;
  constraint_11(ti, lp, vars);
}



void LP_RTA_GFP_SPIN_XU_UNOR_P::constraint_8(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();

  foreach_lower_priority_task(tasks.get_tasks(), ti, tj) {
    uint j = tj->get_id();
    foreach(resources.get_resources(), l_q) {
      uint q = l_q->get_resource_id();
      if (!tj->is_request_exist(q))
        continue;
      if (j == ti.get_id())
        request_num_j = tj->get_request_by_id(q).get_num_requests();
      else 
        request_num_j = tj->get_max_request_num(q, ti.get_response_time());

      for (uint k = 1; k <= request_num_j; k++) {
        LinearExpression *exp = new LinearExpression(); 

        var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
        exp->add_term(var_id, 1);

        for (uint u = 1; u <= p_num; u++) {
          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          exp->add_term(var_id, 1);
        }

        lp->add_inequality(exp, 1);
      }
    }
  }
}

void LP_RTA_GFP_SPIN_XU_UNOR_P::constraint_9(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();
  

  foreach(resources.get_resources(), l_q) {
    uint q = l_q->get_resource_id();
    if (!ti.is_request_exist(q))
      continue;
    LinearExpression *exp = new LinearExpression(); 
    foreach_lower_priority_task(tasks.get_tasks(), ti, tj) {
      uint j = tj->get_id();
      if (!tj->is_request_exist(q))
        continue;
      if (j == ti.get_id())
        request_num_j = tj->get_request_by_id(q).get_num_requests();
      else 
        request_num_j = tj->get_max_request_num(q, ti.get_response_time());

      for (uint k = 1; k <= request_num_j; k++) {
        for (uint u = 1; u <= p_num; u++) {
          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
          exp->add_term(var_id, 1);
        }
      }
    }
    int32_t sum = 0;
    foreach(tasks.get_tasks(), tj) {
      if ((tj->get_priority() <= ti.get_priority())&&(tj->is_request_exist(q))) {
        sum += tj->get_request_by_id(q).get_num_requests();
      }
    }

    var_id = vars->lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
    exp->add_term(var_id, 1);

    lp->add_inequality(exp, sum);
  }
}

void LP_RTA_GFP_SPIN_XU_UNOR_P::constraint_10(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();
  LinearExpression *exp = new LinearExpression();

  foreach(tasks.get_tasks(), tj) {
    uint j = tj->get_id();
    if (tj->get_priority() > ti.get_priority())
      continue;
    foreach(resources.get_resources(), l_q) {
      uint q = l_q->get_resource_id();
      if (!tj->is_request_exist(q))
        continue;
      if (j == ti.get_id())
        request_num_j = tj->get_request_by_id(q).get_num_requests();
      else 
        request_num_j = tj->get_max_request_num(q, ti.get_response_time());

      for (uint k = 1; k <= request_num_j; k++) {

        for (uint u = 1; u <= p_num; u++) {
          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          exp->add_term(var_id, 1);
        }
      }
    }
  }

  lp->add_equality(exp, 0);
}

void LP_RTA_GFP_SPIN_XU_UNOR_P::constraint_11(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();

  foreach(resources.get_resources(), l_q) {
    int32_t sum = 0;
    uint q = l_q->get_resource_id();
    if (!ti.is_request_exist(q))
      continue;
    LinearExpression *exp = new LinearExpression(); 
    foreach(tasks.get_tasks(), tj) {
      uint j = tj->get_id();
      if (tj->get_priority() > ti.get_priority())
        continue;
      if (!tj->is_request_exist(q))
        continue;
      if (j == ti.get_id())
        request_num_j = tj->get_request_by_id(q).get_num_requests();
      else 
        request_num_j = tj->get_max_request_num(q, ti.get_response_time());

      for (uint k = 1; k <= request_num_j; k++) {
        sum += p_num - 1;

        var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
        exp->add_term(var_id, p_num - 1);

        for (uint u = 1; u <= p_num; u++) {
          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
          exp->add_term(var_id, 1);
        }
      }
    }
    lp->add_inequality(exp, sum);
  }
}





/** Class LP_RTA_GFP_SPIN_XU_FIFO_P */

LP_RTA_GFP_SPIN_XU_FIFO_P::LP_RTA_GFP_SPIN_XU_FIFO_P()
    : LP_RTA_GFP_SPIN_XU_FIFO() {}

LP_RTA_GFP_SPIN_XU_FIFO_P::LP_RTA_GFP_SPIN_XU_FIFO_P(DAG_TaskSet tasks, ProcessorSet processors,
                                 ResourceSet resources)
    : LP_RTA_GFP_SPIN_XU_FIFO(tasks, processors, resources) {}

LP_RTA_GFP_SPIN_XU_FIFO_P::~LP_RTA_GFP_SPIN_XU_FIFO_P() {}

bool LP_RTA_GFP_SPIN_XU_FIFO_P::is_schedulable() {
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

      if (response_time <= ti->get_deadline()) {
        ti->set_response_time(response_time);
      } else {
        return false;
      }
    }
  } while (update);

  return true;
}


uint64_t LP_RTA_GFP_SPIN_XU_FIFO_P::resource_delay(const DAG_Task& ti) {
  uint64_t max_delay;
  uint32_t p_num = processors.get_processor_num();
  GFP_SPIN_XUMapper vars;
  LinearProgram delay;
  LinearExpression* obj = new LinearExpression();
  objective(ti, &vars, obj);
  delay.set_objective(obj);
  declare_variable_bounds(ti, &delay, &vars);
  vars.seal();
  add_constraints(ti, &delay, &vars);

  GLPKSolution* rb_solution =
      new GLPKSolution(delay, vars.get_num_vars());
  // cout << "max_var_num:" << vars.get_num_vars() << endl;

  if (rb_solution->is_solved()) {
    // int64_t var_id; 
    // foreach(resources.get_resources(), l_q) {
    //   uint q = l_q->get_resource_id();
    //   if (!ti.is_request_exist(q))
    //     continue;

    //   LinearExpression* exp = new LinearExpression();
    //   var_id = vars.lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
    //   exp->add_term(var_id,1);
    //   cout << "Y[" << q << "]" << rb_solution->evaluate(*exp) << endl; 
    //   delete exp;
    // }

    double result = rb_solution->evaluate(*(delay.get_objective()));
    delete rb_solution;
    max_delay = result;
  } else {
    cout << "[LP_RTA_GFP_SPIN_XU_FIFO_P]unsolved." << endl;
    // cout << "LP solver encounters a problem at T[" << ti.get_id() << "] l[" << res_id << "] X[" << x << "]" << endl;
    // tasks.display();
    rb_solution->show_error();
    if (GLP_FEAS == rb_solution->get_status()) {
      max_delay = rb_solution->evaluate(*(delay.get_objective()));
    } else {
      delete rb_solution;
      exit(1);
      return ti.get_deadline() * p_num;;
    }

    // continue;
    // exit(1);
    // return ti.get_deadline() * p_num;
    // return MAX_LONG;
  }

#if GLPK_MEM_USAGE_CHECK == 1
  int peak;
  glp_mem_usage(NULL, &peak, NULL, NULL);
  cout << "Peak memory usage:" << peak << endl;
#endif

  return max_delay;
}

ulong LP_RTA_GFP_SPIN_XU_FIFO_P::get_response_time(const DAG_Task& ti) {
  uint64_t C_i = ti.get_wcet();
  uint64_t L_i = ti.get_critical_path_length();
  uint64_t D_i = ti.get_deadline();
  // uint64_t B_i = total_resource_delay(ti);
  uint32_t p_num = processors.get_processor_num();
  uint64_t test_start = L_i + (C_i - L_i)/p_num  + resource_delay(ti)/p_num;
  // cout << "total resource delay:" << B_i << endl;
  // cout << "test start:" << test_start << endl;
  uint64_t response_time = test_start;

  while (response_time <= D_i) {
    uint64_t update = test_start;
    uint64_t interf = 0;
    foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
      // interf += workload_bound((*tx), D_i);
      interf += workload_bound((*tx), response_time);
    }
    update += interf/p_num;

    if (update > response_time) {
      response_time = update;
      // ti.set_response_time(response_time);
    } else {
      return response_time;
    }

  }

  return D_i + 100;
}

void LP_RTA_GFP_SPIN_XU_FIFO_P::add_constraints(const DAG_Task& ti, LinearProgram* lp,
                              GFP_SPIN_XUMapper* vars) {
// cout << "C1" << endl;
  constraint_1(ti, lp, vars);
// cout << "C2" << endl;
  constraint_2(ti, lp, vars);
// cout << "C3" << endl;
  constraint_3(ti, lp, vars);
// cout << "C4" << endl;
  constraint_4(ti, lp, vars);
// cout << "C5" << endl;
  constraint_5(ti, lp, vars);
// cout << "C6" << endl;
  constraint_6(ti, lp, vars);
// cout << "C7" << endl;
  // constraint_7(ti, lp, vars);
// cout << "C8" << endl;
  constraint_8(ti, lp, vars);
// cout << "C9" << endl;
  constraint_9(ti, lp, vars);
// cout << "C10" << endl;
  constraint_10(ti, lp, vars);
// cout << "C11" << endl;
  constraint_11(ti, lp, vars);
}



void LP_RTA_GFP_SPIN_XU_FIFO_P::constraint_8(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();

  foreach_lower_priority_task(tasks.get_tasks(), ti, tj) {
    uint j = tj->get_id();
    foreach(resources.get_resources(), l_q) {
      uint q = l_q->get_resource_id();
      if (!tj->is_request_exist(q))
        continue;
      if (j == ti.get_id())
        request_num_j = tj->get_request_by_id(q).get_num_requests();
      else 
        request_num_j = tj->get_max_request_num(q, ti.get_response_time());

      for (uint k = 1; k <= request_num_j; k++) {
        LinearExpression *exp = new LinearExpression(); 

        var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
        exp->add_term(var_id, 1);

        for (uint u = 1; u <= p_num; u++) {
          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          exp->add_term(var_id, 1);
        }

        lp->add_inequality(exp, 1);
      }
    }
  }
}

void LP_RTA_GFP_SPIN_XU_FIFO_P::constraint_9(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();
  

  foreach(resources.get_resources(), l_q) {
    uint q = l_q->get_resource_id();
    if (!ti.is_request_exist(q))
      continue;
    LinearExpression *exp = new LinearExpression(); 
    foreach_lower_priority_task(tasks.get_tasks(), ti, tj) {
      uint j = tj->get_id();
      if (!tj->is_request_exist(q))
        continue;
      if (j == ti.get_id())
        request_num_j = tj->get_request_by_id(q).get_num_requests();
      else 
        request_num_j = tj->get_max_request_num(q, ti.get_response_time());

      for (uint k = 1; k <= request_num_j; k++) {
        for (uint u = 1; u <= p_num; u++) {
          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
          exp->add_term(var_id, 1);
        }
      }
    }
    int32_t sum = 0;
    foreach(tasks.get_tasks(), tj) {
      if ((tj->get_priority() <= ti.get_priority())&&(tj->is_request_exist(q))) {
        sum += tj->get_request_by_id(q).get_num_requests();
      }
    }

    var_id = vars->lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
    exp->add_term(var_id, 1);

    lp->add_inequality(exp, sum);
  }
}

void LP_RTA_GFP_SPIN_XU_FIFO_P::constraint_10(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();
  LinearExpression *exp = new LinearExpression();

  foreach(tasks.get_tasks(), tj) {
    uint j = tj->get_id();
    if (tj->get_priority() > ti.get_priority())
      continue;
    foreach(resources.get_resources(), l_q) {
      uint q = l_q->get_resource_id();
      if (!tj->is_request_exist(q))
        continue;
      if (j == ti.get_id())
        request_num_j = tj->get_request_by_id(q).get_num_requests();
      else 
        request_num_j = tj->get_max_request_num(q, ti.get_response_time());

      for (uint k = 1; k <= request_num_j; k++) {

        for (uint u = 1; u <= p_num; u++) {
          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          exp->add_term(var_id, 1);
        }
      }
    }
  }

  lp->add_equality(exp, 0);
}

void LP_RTA_GFP_SPIN_XU_FIFO_P::constraint_11(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();

  foreach(resources.get_resources(), l_q) {
    int32_t sum = 0;
    uint q = l_q->get_resource_id();
    if (!ti.is_request_exist(q))
      continue;
    LinearExpression *exp = new LinearExpression(); 
    foreach(tasks.get_tasks(), tj) {
      uint j = tj->get_id();
      if (tj->get_priority() > ti.get_priority())
        continue;
      if (!tj->is_request_exist(q))
        continue;
      if (j == ti.get_id())
        request_num_j = tj->get_request_by_id(q).get_num_requests();
      else 
        request_num_j = tj->get_max_request_num(q, ti.get_response_time());

      for (uint k = 1; k <= request_num_j; k++) {
        sum += p_num - 1;

        var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
        exp->add_term(var_id, p_num - 1);

        for (uint u = 1; u <= p_num; u++) {
          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
          exp->add_term(var_id, 1);
        }
      }
    }
    lp->add_inequality(exp, sum);
  }
}





/** Class LP_RTA_GFP_SPIN_XU_PRIO_P */

LP_RTA_GFP_SPIN_XU_PRIO_P::LP_RTA_GFP_SPIN_XU_PRIO_P()
    : LP_RTA_GFP_SPIN_XU_PRIO() {}

LP_RTA_GFP_SPIN_XU_PRIO_P::LP_RTA_GFP_SPIN_XU_PRIO_P(DAG_TaskSet tasks, ProcessorSet processors,
                                 ResourceSet resources)
    : LP_RTA_GFP_SPIN_XU_PRIO(tasks, processors, resources) {}

LP_RTA_GFP_SPIN_XU_PRIO_P::~LP_RTA_GFP_SPIN_XU_PRIO_P() {}

bool LP_RTA_GFP_SPIN_XU_PRIO_P::is_schedulable() {
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

      if (response_time <= ti->get_deadline()) {
        ti->set_response_time(response_time);
      } else {
        return false;
      }
    }
  } while (update);

  return true;
}


uint64_t LP_RTA_GFP_SPIN_XU_PRIO_P::resource_delay(const DAG_Task& ti) {
  uint64_t max_delay;
  uint32_t p_num = processors.get_processor_num();
  GFP_SPIN_XUMapper vars;
  LinearProgram delay;
  LinearExpression* obj = new LinearExpression();
  objective(ti, &vars, obj);
  delay.set_objective(obj);
  declare_variable_bounds(ti, &delay, &vars);
  vars.seal();
  add_constraints(ti, &delay, &vars);

  GLPKSolution* rb_solution =
      new GLPKSolution(delay, vars.get_num_vars());
  // cout << "max_var_num:" << vars.get_num_vars() << endl;

  if (rb_solution->is_solved()) {
    // int64_t var_id; 
    // foreach(resources.get_resources(), l_q) {
    //   uint q = l_q->get_resource_id();
    //   if (!ti.is_request_exist(q))
    //     continue;

    //   LinearExpression* exp = new LinearExpression();
    //   var_id = vars.lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
    //   exp->add_term(var_id,1);
    //   cout << "Y[" << q << "]" << rb_solution->evaluate(*exp) << endl; 
    //   delete exp;
    // }

    double result = rb_solution->evaluate(*(delay.get_objective()));
    delete rb_solution;
    max_delay = result;
  } else {
    cout << "[LP_RTA_GFP_SPIN_XU_PRIO_P]unsolved." << endl;
    // cout << "LP solver encounters a problem at T[" << ti.get_id() << "] l[" << res_id << "] X[" << x << "]" << endl;
    // tasks.display();
    rb_solution->show_error();
    if (GLP_FEAS == rb_solution->get_status()) {
      max_delay = rb_solution->evaluate(*(delay.get_objective()));
    } else {
      delete rb_solution;
      exit(1);
      return ti.get_deadline() * p_num;;
    }

    // continue;
    // exit(1);
    // return ti.get_deadline() * p_num;
    // return MAX_LONG;
  }

#if GLPK_MEM_USAGE_CHECK == 1
  int peak;
  glp_mem_usage(NULL, &peak, NULL, NULL);
  cout << "Peak memory usage:" << peak << endl;
#endif

  return max_delay;
}

ulong LP_RTA_GFP_SPIN_XU_PRIO_P::get_response_time(const DAG_Task& ti) {
  uint64_t C_i = ti.get_wcet();
  uint64_t L_i = ti.get_critical_path_length();
  uint64_t D_i = ti.get_deadline();
  // uint64_t B_i = total_resource_delay(ti);
  uint32_t p_num = processors.get_processor_num();
  uint64_t test_start = L_i + (C_i - L_i)/p_num  + resource_delay(ti)/p_num;
  // cout << "total resource delay:" << B_i << endl;
  // cout << "test start:" << test_start << endl;
  uint64_t response_time = test_start;

  while (response_time <= D_i) {
    uint64_t update = test_start;
    uint64_t interf = 0;
    foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
      // interf += workload_bound((*tx), D_i);
      interf += workload_bound((*tx), response_time);
    }
    update += interf/p_num;

    if (update > response_time) {
      response_time = update;
      // ti.set_response_time(response_time);
    } else {
      return response_time;
    }

  }

  return D_i + 100;
}

void LP_RTA_GFP_SPIN_XU_PRIO_P::add_constraints(const DAG_Task& ti, LinearProgram* lp,
                              GFP_SPIN_XUMapper* vars) {
// cout << "C1" << endl;
  constraint_1(ti, lp, vars);
// cout << "C2" << endl;
  constraint_2(ti, lp, vars);
// cout << "C3" << endl;
  constraint_3(ti, lp, vars);
// cout << "C4" << endl;
  constraint_4(ti, lp, vars);
// cout << "C5" << endl;
  constraint_5(ti, lp, vars);
// cout << "C6" << endl;
  constraint_6(ti, lp, vars);
// cout << "C7" << endl;
  // constraint_7(ti, lp, vars);
// cout << "C8" << endl;
  constraint_8(ti, lp, vars);
// cout << "C9" << endl;
  constraint_9(ti, lp, vars);
// cout << "C10" << endl;
  constraint_10(ti, lp, vars);
// cout << "C11" << endl;
  constraint_11(ti, lp, vars);
// cout << "C12" << endl;
  constraint_12(ti, lp, vars);
// cout << "C13" << endl;
  constraint_13(ti, lp, vars);
}



void LP_RTA_GFP_SPIN_XU_PRIO_P::constraint_8(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();

  foreach_lower_priority_task(tasks.get_tasks(), ti, tj) {
    uint j = tj->get_id();
    foreach(resources.get_resources(), l_q) {
      uint q = l_q->get_resource_id();
      if (!tj->is_request_exist(q))
        continue;
      if (j == ti.get_id())
        request_num_j = tj->get_request_by_id(q).get_num_requests();
      else 
        request_num_j = tj->get_max_request_num(q, ti.get_response_time());

      for (uint k = 1; k <= request_num_j; k++) {
        LinearExpression *exp = new LinearExpression(); 

        var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
        exp->add_term(var_id, 1);

        for (uint u = 1; u <= p_num; u++) {
          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          exp->add_term(var_id, 1);
        }

        lp->add_inequality(exp, 1);
      }
    }
  }
}

void LP_RTA_GFP_SPIN_XU_PRIO_P::constraint_9(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();
  

  foreach(resources.get_resources(), l_q) {
    uint q = l_q->get_resource_id();
    if (!ti.is_request_exist(q))
      continue;
    LinearExpression *exp = new LinearExpression(); 
    foreach_lower_priority_task(tasks.get_tasks(), ti, tj) {
      uint j = tj->get_id();
      if (!tj->is_request_exist(q))
        continue;
      if (j == ti.get_id())
        request_num_j = tj->get_request_by_id(q).get_num_requests();
      else 
        request_num_j = tj->get_max_request_num(q, ti.get_response_time());

      for (uint k = 1; k <= request_num_j; k++) {
        for (uint u = 1; u <= p_num; u++) {
          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
          exp->add_term(var_id, 1);
        }
      }
    }
    int32_t sum = 0;
    foreach(tasks.get_tasks(), tj) {
      if ((tj->get_priority() <= ti.get_priority())&&(tj->is_request_exist(q))) {
        sum += tj->get_request_by_id(q).get_num_requests();
      }
    }

    var_id = vars->lookup(0, q, 0, 0, GFP_SPIN_XUMapper::REQUEST_NUM);
    exp->add_term(var_id, 1);

    lp->add_inequality(exp, sum);
  }
}

void LP_RTA_GFP_SPIN_XU_PRIO_P::constraint_10(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();
  LinearExpression *exp = new LinearExpression();

  foreach(tasks.get_tasks(), tj) {
    uint j = tj->get_id();
    if (tj->get_priority() > ti.get_priority())
      continue;
    foreach(resources.get_resources(), l_q) {
      uint q = l_q->get_resource_id();
      if (!tj->is_request_exist(q))
        continue;
      if (j == ti.get_id())
        request_num_j = tj->get_request_by_id(q).get_num_requests();
      else 
        request_num_j = tj->get_max_request_num(q, ti.get_response_time());

      for (uint k = 1; k <= request_num_j; k++) {

        for (uint u = 1; u <= p_num; u++) {
          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          exp->add_term(var_id, 1);
        }
      }
    }
  }

  lp->add_equality(exp, 0);
}

void LP_RTA_GFP_SPIN_XU_PRIO_P::constraint_11(const DAG_Task& ti, LinearProgram* lp,
                          GFP_SPIN_XUMapper* vars) {
  int64_t var_id;
  uint request_num_j;
  int32_t p_num = processors.get_processor_num();

  foreach(resources.get_resources(), l_q) {
    int32_t sum = 0;
    uint q = l_q->get_resource_id();
    if (!ti.is_request_exist(q))
      continue;
    LinearExpression *exp = new LinearExpression(); 
    foreach(tasks.get_tasks(), tj) {
      uint j = tj->get_id();
      if (tj->get_priority() > ti.get_priority())
        continue;
      if (!tj->is_request_exist(q))
        continue;
      if (j == ti.get_id())
        request_num_j = tj->get_request_by_id(q).get_num_requests();
      else 
        request_num_j = tj->get_max_request_num(q, ti.get_response_time());

      for (uint k = 1; k <= request_num_j; k++) {
        sum += p_num - 1;

        var_id = vars->lookup(j, q, k, 0, GFP_SPIN_XUMapper::BLOCKING_KP);
        exp->add_term(var_id, p_num - 1);

        for (uint u = 1; u <= p_num; u++) {
          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_NPD);
          exp->add_term(var_id, 1);

          var_id = vars->lookup(j, q, k, u, GFP_SPIN_XUMapper::BLOCKING_DD);
          exp->add_term(var_id, 1);
        }
      }
    }
    lp->add_inequality(exp, sum);
  }
}



































/** Class LP_RTA_GFP_SPIN_UNOR */

// LP_RTA_GFP_SPIN_UNOR::LP_RTA_GFP_SPIN_UNOR()
//     : RTA_DAG_GFP_JOSE() {}

// LP_RTA_GFP_SPIN_UNOR::LP_RTA_GFP_SPIN_UNOR(DAG_TaskSet tasks, ProcessorSet processors,
//                                  ResourceSet resources)
//     : RTA_DAG_GFP_JOSE(tasks, processors, resources) {}

// LP_RTA_GFP_SPIN_UNOR::~LP_RTA_GFP_SPIN_UNOR() {}

// bool LP_RTA_GFP_SPIN_UNOR::is_schedulable() {
//   bool update = false;

//   foreach(tasks.get_tasks(), ti) {
//     ti->set_response_time(ti->get_critical_path_length());
//   }

//   foreach(tasks.get_tasks(), ti) {
//     ulong response_time = get_response_time(*ti);

//     if (response_time <= ti->get_deadline()) {
//       ti->set_response_time(response_time);
//     } else {
//       return false;
//     }
//   }

//   // do {
//   //   update = false;
//   //   foreach(tasks.get_tasks(), ti) {
//   //     ulong old_response_time = ti->get_response_time();
//   //     ulong response_time = get_response_time(*ti);

//   //     if (old_response_time != response_time) update = true;

//   //     if (response_time <= ti->get_deadline()) {
//   //       ti->set_response_time(response_time);
//   //     } else {
//   //       return false;
//   //     }
//   //   }
//   // } while (update);

//   return true;
// }

// uint64_t LP_RTA_GFP_SPIN_UNOR::higher(const DAG_Task& ti, uint32_t res_id,
//                                        uint64_t interval) {
//   uint64_t sum = 0;
//   foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
//     if (th->is_request_exist(res_id)) {
//       const Request& request_q = th->get_request_by_id(res_id);
//       uint32_t n = th->get_max_job_num(interval);
//       uint32_t N_h_q = request_q.get_num_requests();
//       uint64_t L_h_q = request_q.get_max_length();
//       sum += n * N_h_q * L_h_q;
//     }
//   }
//   return sum;
// }

// uint64_t LP_RTA_GFP_SPIN_UNOR::lower(const DAG_Task& ti,
//                                       uint32_t res_id) {
//   uint64_t max = 0;
//   foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
//     if (tl->is_request_exist(res_id)) {
//       const Request& request_q = tl->get_request_by_id(res_id);
//       uint64_t L_l_q = request_q.get_max_length();
//       if (L_l_q > max) max = L_l_q;
//     }
//   }
//   return max;
// }

// uint64_t LP_RTA_GFP_SPIN_UNOR::equal(const DAG_Task& ti,
//                                       uint32_t res_id) {
//   if (ti.is_request_exist(res_id)) {
//     const Request& request_q = ti.get_request_by_id(res_id);
//     uint32_t N_i_q = request_q.get_num_requests();
//     uint64_t L_i_q = request_q.get_max_length();
//     uint32_t n_i = ti.get_dcores();
//     uint32_t m_i = min(n_i - 1, N_i_q - 1);
//     return m_i * L_i_q;
//   } else {
//     return 0;
//   }
// }

// uint64_t LP_RTA_GFP_SPIN_UNOR::dpr(const DAG_Task& ti, uint32_t res_id) {
//   uint64_t test_start = lower(ti, res_id) + equal(ti, res_id);
//   uint64_t test_end = ti.get_deadline();
//   uint64_t delay = test_start;

//   while (delay <= test_end) {
//     uint64_t temp = test_start;

//     temp += higher(ti, res_id, delay);

//     if (temp > delay)
//       delay = temp;
//     else if (temp == delay)
//       return delay;
//   }
//   return test_end + 100;
// }


// uint64_t LP_RTA_GFP_SPIN_UNOR::resource_delay(const DAG_Task& ti, uint res_id) {
//   uint32_t request_num = ti.get_request_by_id(res_id).get_num_requests();
//   uint64_t max_delay = 0;

//   for (uint x = 0; x <= request_num; x++) {
//     GFP_SPIN_XUMapper vars;
//     LinearProgram delay;
//     LinearExpression* obj = new LinearExpression();
//     objective(ti, res_id, x, &vars, obj);
//     delay.set_objective(obj);
//     vars.seal();
//     add_constraints(ti, res_id, x, &delay, &vars);

//     GLPKSolution* rb_solution =
//         new GLPKSolution(delay, vars.get_num_vars());

//     if (rb_solution->is_solved()) {
//       double result = rb_solution->evaluate(*(delay.get_objective()));
//       if (max_delay < result) {
//         max_delay = result;
//       }
//       delete rb_solution;
//     } else {
//       cout << "[LP_RTA_GFP_SPIN_UNOR]unsolved." << endl;
//       cout << "LP solver encounters a problem at T[" << ti.get_id() << "] l[" << res_id << "] X[" << x << "]" << endl;
//       // tasks.display();
//       delete rb_solution;
//       continue;
//       // exit(1);
//       // return ti.get_deadline();
//       // return MAX_LONG;
//     }

// #if GLPK_MEM_USAGE_CHECK == 1
//     int peak;
//     glp_mem_usage(NULL, &peak, NULL, NULL);
//     cout << "Peak memory usage:" << peak << endl;
// #endif
//   }
//   return max_delay;
// }


// uint64_t LP_RTA_GFP_SPIN_UNOR::total_resource_delay(const DAG_Task& ti) {
//   uint64_t sum = 0;
//   foreach(ti.get_requests(), r_i_q) {
//     uint res_id = r_i_q->get_resource_id();
//     sum += resource_delay(ti, res_id);
//   }
//   return sum;
// }

// ulong LP_RTA_GFP_SPIN_UNOR::get_response_time(const DAG_Task& ti) {
//   uint64_t C_i = ti.get_wcet();
//   uint64_t L_i = ti.get_critical_path_length();
//   uint64_t D_i = ti.get_deadline();
//   // uint64_t B_i = total_resource_delay(ti);
//   uint32_t p_num = processors.get_processor_num();
//   // uint64_t test_start = L_i + B_i + (C_i - L_i)/p_num;
//   // cout << "total resource delay:" << B_i << endl;
//   // cout << "test start:" << test_start << endl;
//   uint64_t response_time = L_i + (C_i - L_i)/p_num;

//   uint64_t interf = 0;
//   foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
//     interf += workload_bound((*tx), D_i);
//     // interf += workload_bound((*tx), response_time);
//   }
//   response_time += interf/p_num;

//   if (response_time <= D_i) {
//     uint64_t B_i = total_resource_delay(ti);
//     cout << "[UNOR]TRD:" << B_i << endl;
//     response_time += B_i;
//   }

//   return response_time;
// }

// uint64_t LP_RTA_GFP_SPIN_UNOR::workload_bound(const DAG_Task& tx, int64_t interval) {
//   int64_t L_x = tx.get_critical_path_length();
//   int64_t T_x = tx.get_period();
//   int64_t W_x = tx.get_wcet();
//   int64_t delta_C = interval - max((int64_t)0, (interval - L_x)/ T_x) * T_x;
//   int64_t bound = max((int64_t)0, (interval - delta_C)/ T_x) * W_x + (int64_t)get_WC(tx, delta_C);
//   return bound;
// }

// void LP_RTA_GFP_SPIN_UNOR::declare_variable_bounds(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                                       GFP_SPIN_XUMapper* vars) {

// }

// void LP_RTA_GFP_SPIN_UNOR::objective(const DAG_Task& ti, uint res_id, uint res_num, GFP_SPIN_XUMapper* vars,
//                         LinearExpression* obj) {
//   int64_t var_id;
//   uint request_num_x, request_num_y;
//   double coef = 1.0 / processors.get_processor_num();

//   foreach(tasks.get_tasks(), tx) {
//     uint x = tx->get_id();
//     if (!tx->is_request_exist(res_id))
//       continue;
//     if (x == ti.get_id())
//       request_num_x = res_num;
//     else
//       request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
//     int32_t L_x_q = tx->get_request_by_id(res_id).get_max_length();
//     // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
//     for (uint u = 1; u <= request_num_x; u++) {
//       var_id = vars->lookup(x, u, 0, 0, GFP_SPIN_XUMapper::BLOCKING_SPIN);
//       obj->add_term(var_id, L_x_q);

//       foreach(tasks.get_tasks(), ty) {  // for transitive delay
//         uint y = ty->get_id();
//         if (!ty->is_request_exist(res_id))
//           continue;
//         if (y == ti.get_id())
//           request_num_y = res_num;
//         else
//           request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());
//         for (uint v = 1; v <= request_num_y; v++) {
//           double temp = coef * L_x_q;
//           // cout << "temp" << temp << endl;
//           var_id = vars->lookup(x, u, y, v, GFP_SPIN_XUMapper::BLOCKING_TRANS);
//           obj->add_term(var_id, temp);
//         }
//       }

//       if (tx->get_priority() <= ti.get_priority()) {  // to counteract the interference
//         double temp_2 = -1.0 * coef * L_x_q;
//         var_id = vars->lookup(x, u, 0, 0, GFP_SPIN_XUMapper::BLOCKING_SPIN);
//         obj->add_term(var_id, temp_2);
//       }
//     }
//   }
// }

// void LP_RTA_GFP_SPIN_UNOR::add_constraints(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                               GFP_SPIN_XUMapper* vars) {
//   constraint_1(ti, res_id, res_num, lp, vars);
//   constraint_2(ti, res_id, res_num, lp, vars);
//   constraint_3(ti, res_id, res_num, lp, vars);
//   constraint_4(ti, res_id, res_num, lp, vars);
//   constraint_11(ti, res_id, res_num, lp, vars);
//   constraint_12(ti, res_id, res_num, lp, vars);
// }

// void LP_RTA_GFP_SPIN_UNOR::constraint_1(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                           GFP_SPIN_XUMapper* vars) {
//   int64_t var_id;
//   uint request_num_x, request_num_y;

//   foreach(tasks.get_tasks(), tx) {
//     uint x = tx->get_id();
//     if (!tx->is_request_exist(res_id))
//       continue;
//     if (x == ti.get_id())
//       request_num_x = res_num;
//     else
//       request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
//     // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
//     for (uint u = 1; u <= request_num_x; u++) {
//       foreach(tasks.get_tasks(), ty) {
//         uint y = ty->get_id();
//         if (!ty->is_request_exist(res_id))
//           continue;
//         if (y == ti.get_id())
//           request_num_y = res_num;
//         else
//           request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());

//         for (uint v = 1; v <= request_num_y; v++) {
//           LinearExpression *exp = new LinearExpression();

//           var_id = vars->lookup(x, u, y, v, GFP_SPIN_XUMapper::BLOCKING_TRANS);
//           exp->add_term(var_id, 1);

//           var_id = vars->lookup(x, u, 0, 0, GFP_SPIN_XUMapper::BLOCKING_SPIN);
//           exp->add_term(var_id, 1);

//           lp->add_inequality(exp, 1);
//         }
//       }
//     }
//   }
// }

// // 
// void LP_RTA_GFP_SPIN_UNOR::constraint_2(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                           GFP_SPIN_XUMapper* vars) {
//   int64_t var_id;
//   uint request_num_x, request_num_y;

//   foreach(tasks.get_tasks(), tx) {
//     uint x = tx->get_id();
//     if (!tx->is_request_exist(res_id))
//       continue;
//     if (x == ti.get_id())
//       request_num_x = res_num;
//     else
//       request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
//     // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
//     for (uint u = 1; u <= request_num_x; u++) {
//       foreach(tasks.get_tasks(), ty) {
//         uint y = ty->get_id();
//         if (!ty->is_request_exist(res_id))
//           continue;
//         if (y == ti.get_id())
//           request_num_y = res_num;
//         else
//           request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());

//         for (uint v = 1; v <= request_num_y; v++) {
//           LinearExpression *exp = new LinearExpression();
//           var_id = vars->lookup(x, u, y, v, GFP_SPIN_XUMapper::BLOCKING_TRANS);
//           exp->add_term(var_id, 1);

//           var_id = vars->lookup(y, v, x, u, GFP_SPIN_XUMapper::BLOCKING_TRANS);
//           exp->add_term(var_id, 1);

//           lp->add_inequality(exp, 1);
//         }
//       }
//     }
//   }
// }

// // 
// void LP_RTA_GFP_SPIN_UNOR::constraint_3(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                           GFP_SPIN_XUMapper* vars) {
//   int64_t var_id;
//   uint request_num_x, request_num_y;

//   foreach(tasks.get_tasks(), tx) {
//     uint x = tx->get_id();
//     if (!tx->is_request_exist(res_id))
//       continue;
//     if (x == ti.get_id())
//       request_num_x = res_num;
//     else
//       request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
//     // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
//     for (uint u = 1; u <= request_num_x; u++) {
//       LinearExpression *exp = new LinearExpression();
//       var_id = vars->lookup(x, u, x, u, GFP_SPIN_XUMapper::BLOCKING_TRANS);
//       exp->add_term(var_id, 1);

//       lp->add_equality(exp, 0);
//     }
//   }
// }

// // 
// void LP_RTA_GFP_SPIN_UNOR::constraint_4(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                           GFP_SPIN_XUMapper* vars) {
//   int64_t var_id;
//   uint request_num_x, request_num_y;
//   uint p_num = processors.get_processor_num();

//   foreach(tasks.get_tasks(), tx) {
//     uint x = tx->get_id();
//     if (!tx->is_request_exist(res_id))
//       continue;
//     if (x == ti.get_id())
//       request_num_x = res_num;
//     else
//       request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
//     // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
//     for (uint u = 1; u <= request_num_x; u++) {
//       LinearExpression *exp = new LinearExpression();

//       foreach(tasks.get_tasks(), ty) {
//         uint y = ty->get_id();
//         if (!ty->is_request_exist(res_id))
//           continue;
//         if (y == ti.get_id())
//           request_num_y = res_num;
//         else
//           request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());
//         for (uint v = 1; v <= request_num_y; v++) {
//           var_id = vars->lookup(x, u, y, v, GFP_SPIN_XUMapper::BLOCKING_TRANS);
//           exp->add_term(var_id, 1);
//         }
//       }

//       var_id = vars->lookup(x, u, 0, 0, GFP_SPIN_XUMapper::BLOCKING_SPIN);
//       exp->add_term(var_id, 1);

//       lp->add_inequality(exp, p_num - 1);
//     }
//   }
// }

// // 
// void LP_RTA_GFP_SPIN_UNOR::constraint_11(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                           GFP_SPIN_XUMapper* vars) {
//   int64_t var_id;
//   uint request_num_x, request_num_y;
//   uint p_num = processors.get_processor_num();

//   foreach(tasks.get_tasks(), ty) {
//     uint y = ty->get_id();
//     if (!ty->is_request_exist(res_id))
//       continue;
//     if (y == ti.get_id())
//       request_num_y = res_num;
//     else
//       request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());
//     for (uint v = 1; v <= request_num_y; v++) {
//       foreach_task_except(tasks.get_tasks(), ti, tx) {
//         uint x = tx->get_id();
//         if (!tx->is_request_exist(res_id))
//           continue;
//         if (x == ti.get_id())
//           request_num_x = res_num;
//         else
//           request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());

//         uint32_t ub = ceiling(dpr((*ty), res_id)+tx->get_deadline(), tx->get_period()) * tx->get_request_by_id(res_id).get_num_requests();
//         LinearExpression *exp = new LinearExpression();

//         for (uint u = 1; u <= request_num_x; u++) {
//           var_id = vars->lookup(x, u, y, v, GFP_SPIN_XUMapper::BLOCKING_TRANS);
//           exp->add_term(var_id, 1);
//         }

//         lp->add_inequality(exp, ub);
//       }
//     }
//   }
// }

// // 
// void LP_RTA_GFP_SPIN_UNOR::constraint_12(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                           GFP_SPIN_XUMapper* vars) {
//   int64_t var_id;
//   uint request_num_x;
//   uint p_num = processors.get_processor_num();
//   uint N_i_q = ti.get_request_by_id(res_id).get_num_requests();

//   foreach_task_except(tasks.get_tasks(), ti, tx) {
//     uint x = tx->get_id();
//     if (!tx->is_request_exist(res_id))
//       continue;
//     if (x == ti.get_id())
//       request_num_x = res_num;
//     else
//       request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
//     uint32_t ub = ceiling(dpr(ti, res_id)+tx->get_deadline(), tx->get_period()) * tx->get_request_by_id(res_id).get_num_requests() * (N_i_q - res_num);
//     LinearExpression *exp = new LinearExpression();

//     for (uint u = 1; u <= request_num_x; u++) {
//       var_id = vars->lookup(x, u, 0, 0, GFP_SPIN_XUMapper::BLOCKING_SPIN);
//       exp->add_term(var_id, 1);
//     }

//     lp->add_inequality(exp, ub);
//   }
// }


/** Class LP_RTA_GFP_SPIN_GENERAL */

// LP_RTA_GFP_SPIN_GENERAL::LP_RTA_GFP_SPIN_GENERAL()
//     : RTA_DAG_GFP_JOSE() {}

// LP_RTA_GFP_SPIN_GENERAL::LP_RTA_GFP_SPIN_GENERAL(DAG_TaskSet tasks, ProcessorSet processors,
//                                  ResourceSet resources)
//     : RTA_DAG_GFP_JOSE(tasks, processors, resources) {}

// LP_RTA_GFP_SPIN_GENERAL::~LP_RTA_GFP_SPIN_GENERAL() {}

// bool LP_RTA_GFP_SPIN_GENERAL::is_schedulable() {
//   bool update = false;

//   foreach(tasks.get_tasks(), ti) {
//     ti->set_response_time(ti->get_critical_path_length());
//   }

//   foreach(tasks.get_tasks(), ti) {
//     ulong response_time = get_response_time(*ti);

//     if (response_time <= ti->get_deadline()) {
//       ti->set_response_time(response_time);
//     } else {
//       return false;
//     }
//   }

//   // do {
//   //   update = false;
//   //   foreach(tasks.get_tasks(), ti) {
//   //     ulong old_response_time = ti->get_response_time();
//   //     ulong response_time = get_response_time(*ti);

//   //     if (old_response_time != response_time) update = true;

//   //     if (response_time <= ti->get_deadline()) {
//   //       ti->set_response_time(response_time);
//   //     } else {
//   //       return false;
//   //     }
//   //   }
//   // } while (update);

//   return true;
// }


// uint64_t LP_RTA_GFP_SPIN_GENERAL::resource_delay(const DAG_Task& ti, uint res_id) {
//   uint32_t request_num = ti.get_request_by_id(res_id).get_num_requests();
//   uint64_t max_delay = 0;

//   for (uint x = 0; x <= request_num; x++) {
//     GFP_SPIN_XUMapper vars;
//     LinearProgram delay;
//     LinearExpression* obj = new LinearExpression();
//     objective(ti, res_id, x, &vars, obj);
//     delay.set_objective(obj);
//     vars.seal();
//     add_constraints(ti, res_id, x, &delay, &vars);

//     GLPKSolution* rb_solution =
//         new GLPKSolution(delay, vars.get_num_vars());

//     if (rb_solution->is_solved()) {
//       double result = rb_solution->evaluate(*(delay.get_objective()));
//       if (max_delay < result) {
//         max_delay = result;
//       }
//       delete rb_solution;
//     } else {
//       cout << "[LP_RTA_GFP_SPIN_GENERAL]unsolved." << endl;
//       cout << "LP solver encounters a problem at T[" << ti.get_id() << "] l[" << res_id << "] X[" << x << "]" << endl;
//       // tasks.display();
//       delete rb_solution;
//       continue;
//       // exit(1);
//       // return ti.get_deadline();
//       // return MAX_LONG;
//     }

// #if GLPK_MEM_USAGE_CHECK == 1
//     int peak;
//     glp_mem_usage(NULL, &peak, NULL, NULL);
//     cout << "Peak memory usage:" << peak << endl;
// #endif
//   }
//   return max_delay;
// }


// uint64_t LP_RTA_GFP_SPIN_GENERAL::total_resource_delay(const DAG_Task& ti) {
//   uint64_t sum = 0;
//   foreach(ti.get_requests(), r_i_q) {
//     uint res_id = r_i_q->get_resource_id();
//     sum += resource_delay(ti, res_id);
//   }
//   return sum;
// }

// ulong LP_RTA_GFP_SPIN_GENERAL::get_response_time(const DAG_Task& ti) {
//   uint64_t C_i = ti.get_wcet();
//   uint64_t L_i = ti.get_critical_path_length();
//   uint64_t D_i = ti.get_deadline();
//   // uint64_t B_i = total_resource_delay(ti);
//   uint32_t p_num = processors.get_processor_num();
//   // uint64_t test_start = L_i + B_i + (C_i - L_i)/p_num;
//   // cout << "total resource delay:" << B_i << endl;
//   // cout << "test start:" << test_start << endl;
//   uint64_t response_time = L_i + (C_i - L_i)/p_num;

//   uint64_t interf = 0;
//   foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
//     interf += workload_bound((*tx), D_i);
//     // interf += workload_bound((*tx), response_time);
//   }
//   response_time += interf/p_num;

//   if (response_time <= D_i) {
//     uint64_t B_i = total_resource_delay(ti);
//     cout << "[GENERAL]TRD:" << B_i << endl;
//     response_time += B_i;
//   }

//   return response_time;
// }

// uint64_t LP_RTA_GFP_SPIN_GENERAL::workload_bound(const DAG_Task& tx, int64_t interval) {
//   int64_t L_x = tx.get_critical_path_length();
//   int64_t T_x = tx.get_period();
//   int64_t W_x = tx.get_wcet();
//   int64_t delta_C = interval - max((int64_t)0, (interval - L_x)/ T_x) * T_x;
//   int64_t bound = max((int64_t)0, (interval - delta_C)/ T_x) * W_x + (int64_t)get_WC(tx, delta_C);
//   return bound;
// }

// void LP_RTA_GFP_SPIN_GENERAL::declare_variable_bounds(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                                       GFP_SPIN_XUMapper* vars) {

// }

// void LP_RTA_GFP_SPIN_GENERAL::objective(const DAG_Task& ti, uint res_id, uint res_num, GFP_SPIN_XUMapper* vars,
//                         LinearExpression* obj) {
//   int64_t var_id;
//   uint request_num_x, request_num_y;
//   double coef = 1.0 / processors.get_processor_num();

//   foreach(tasks.get_tasks(), tx) {
//     uint x = tx->get_id();
//     if (!tx->is_request_exist(res_id))
//       continue;
//     if (x == ti.get_id())
//       request_num_x = res_num;
//     else
//       request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
//     int32_t L_x_q = tx->get_request_by_id(res_id).get_max_length();
//     // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
//     for (uint u = 1; u <= request_num_x; u++) {
//       var_id = vars->lookup(x, u, 0, 0, GFP_SPIN_XUMapper::BLOCKING_SPIN);
//       obj->add_term(var_id, L_x_q);

//       foreach(tasks.get_tasks(), ty) {  // for transitive delay
//         uint y = ty->get_id();
//         if (!ty->is_request_exist(res_id))
//           continue;
//         if (y == ti.get_id())
//           request_num_y = res_num;
//         else
//           request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());
//         for (uint v = 1; v <= request_num_y; v++) {
//           double temp = coef * L_x_q;
//           // cout << "temp" << temp << endl;
//           var_id = vars->lookup(x, u, y, v, GFP_SPIN_XUMapper::BLOCKING_TRANS);
//           obj->add_term(var_id, temp);
//         }
//       }

//       if (tx->get_priority() <= ti.get_priority()) {  // to counteract the interference
//         double temp_2 = -1.0 * coef * L_x_q;
//         var_id = vars->lookup(x, u, 0, 0, GFP_SPIN_XUMapper::BLOCKING_SPIN);
//         obj->add_term(var_id, temp_2);
//       }
//     }
//   }
// }

// void LP_RTA_GFP_SPIN_GENERAL::add_constraints(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                               GFP_SPIN_XUMapper* vars) {
//   constraint_1(ti, res_id, res_num, lp, vars);
//   constraint_2(ti, res_id, res_num, lp, vars);
//   constraint_3(ti, res_id, res_num, lp, vars);
//   constraint_4(ti, res_id, res_num, lp, vars);
//   // constraint_5(ti, res_id, res_num, lp, vars);
//   // constraint_6(ti, res_id, res_num, lp, vars);
// }

// void LP_RTA_GFP_SPIN_GENERAL::constraint_1(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                           GFP_SPIN_XUMapper* vars) {
//   int64_t var_id;
//   uint request_num_x, request_num_y;

//   foreach(tasks.get_tasks(), tx) {
//     uint x = tx->get_id();
//     if (!tx->is_request_exist(res_id))
//       continue;
//     if (x == ti.get_id())
//       request_num_x = res_num;
//     else
//       request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
//     // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
//     for (uint u = 1; u <= request_num_x; u++) {
//       foreach(tasks.get_tasks(), ty) {
//         uint y = ty->get_id();
//         if (!ty->is_request_exist(res_id))
//           continue;
//         if (y == ti.get_id())
//           request_num_y = res_num;
//         else
//           request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());

//         for (uint v = 1; v <= request_num_y; v++) {
//           LinearExpression *exp = new LinearExpression();

//           var_id = vars->lookup(x, u, y, v, GFP_SPIN_XUMapper::BLOCKING_TRANS);
//           exp->add_term(var_id, 1);

//           var_id = vars->lookup(x, u, 0, 0, GFP_SPIN_XUMapper::BLOCKING_SPIN);
//           exp->add_term(var_id, 1);

//           lp->add_inequality(exp, 1);
//         }
//       }
//     }
//   }
// }

// // 
// void LP_RTA_GFP_SPIN_GENERAL::constraint_2(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                           GFP_SPIN_XUMapper* vars) {
//   int64_t var_id;
//   uint request_num_x, request_num_y;

//   foreach(tasks.get_tasks(), tx) {
//     uint x = tx->get_id();
//     if (!tx->is_request_exist(res_id))
//       continue;
//     if (x == ti.get_id())
//       request_num_x = res_num;
//     else
//       request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
//     // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
//     for (uint u = 1; u <= request_num_x; u++) {
//       foreach(tasks.get_tasks(), ty) {
//         uint y = ty->get_id();
//         if (!ty->is_request_exist(res_id))
//           continue;
//         if (y == ti.get_id())
//           request_num_y = res_num;
//         else
//           request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());

//         for (uint v = 1; v <= request_num_y; v++) {
//           LinearExpression *exp = new LinearExpression();
//           var_id = vars->lookup(x, u, y, v, GFP_SPIN_XUMapper::BLOCKING_TRANS);
//           exp->add_term(var_id, 1);

//           var_id = vars->lookup(y, v, x, u, GFP_SPIN_XUMapper::BLOCKING_TRANS);
//           exp->add_term(var_id, 1);

//           lp->add_inequality(exp, 1);
//         }
//       }
//     }
//   }
// }

// // 
// void LP_RTA_GFP_SPIN_GENERAL::constraint_3(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                           GFP_SPIN_XUMapper* vars) {
//   int64_t var_id;
//   uint request_num_x;

//   foreach(tasks.get_tasks(), tx) {
//     uint x = tx->get_id();
//     if (!tx->is_request_exist(res_id))
//       continue;
//     if (x == ti.get_id())
//       request_num_x = res_num;
//     else
//       request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
//     // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
//     for (uint u = 1; u <= request_num_x; u++) {
//       LinearExpression *exp = new LinearExpression();
//       var_id = vars->lookup(x, u, x, u, GFP_SPIN_XUMapper::BLOCKING_TRANS);
//       exp->add_term(var_id, 1);

//       lp->add_equality(exp, 0);
//     }
//   }
// }

// // 
// void LP_RTA_GFP_SPIN_GENERAL::constraint_4(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                           GFP_SPIN_XUMapper* vars) {
//   int64_t var_id;
//   uint request_num_x, request_num_y;
//   uint p_num = processors.get_processor_num();

//   foreach(tasks.get_tasks(), tx) {
//     uint x = tx->get_id();
//     if (!tx->is_request_exist(res_id))
//       continue;
//     if (x == ti.get_id())
//       request_num_x = res_num;
//     else
//       request_num_x = tx->get_max_request_num(res_id, ti.get_deadline());
//     // cout << "res_id:" << res_id << "ti.get_response_time:" << ti.get_response_time() <<"request num:" << request_num << endl;
//     for (uint u = 1; u <= request_num_x; u++) {
//       LinearExpression *exp = new LinearExpression();

//       foreach(tasks.get_tasks(), ty) {
//         uint y = ty->get_id();
//         if (!ty->is_request_exist(res_id))
//           continue;
//         if (y == ti.get_id())
//           request_num_y = res_num;
//         else
//           request_num_y = ty->get_max_request_num(res_id, ti.get_deadline());
//         for (uint v = 1; v <= request_num_y; v++) {
//           var_id = vars->lookup(x, u, y, v, GFP_SPIN_XUMapper::BLOCKING_TRANS);
//           exp->add_term(var_id, 1);
//         }
//       }

//       var_id = vars->lookup(x, u, 0, 0, GFP_SPIN_XUMapper::BLOCKING_SPIN);
//       exp->add_term(var_id, 1);

//       lp->add_inequality(exp, p_num - 1);
//     }
//   }
// }
