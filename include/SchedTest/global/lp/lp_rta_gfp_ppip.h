// Copyright [2021] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_GLOBAL_LP_LP_RTA_GFP_PPIP_H_
#define INCLUDE_SCHEDTEST_GLOBAL_LP_LP_RTA_GFP_PPIP_H_

/*
*
*  Blocking analysis of suspension-based protocols for parallel real-time tasks 
*  under global fixed-priority scheduling [P-PIP]
*  
*/

#include <g_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>
#include <varmapper.h>
#include <rta_dag_gfp_jose.h>
#include <string>


class Task;
class TaskSet;
class Request;
class Resource;
class ResourceSet;
class ProcessorSet;
class LinearExpression;
class LinearProgram;

/*
|________________|_____________________|______________________|______________________|______________________|
|                |                     |                      |                      |                      |
|(63-34)Reserved |(32-30) var type     |(29-20) Task          |(19-10) Resource      |(9-0) Request         |
|________________|_____________________|______________________|______________________|______________________|
*/
class PPIPMapper : public VarMapperBase {
 public:
  enum var_type {
    REQUEST_NUM,
    BLOCKING_DIRECT,     // 0x000
    BLOCKING_INDIRECT,   // 0x001
    BLOCKING_PREEMPT,    // 0x010
    BLOCKING_OTHER,      // 0x011
    INTERF_REGULAR,      // 0x100
    INTERF_OTHER         // 0x111
  };

 protected:
  static uint64_t encode_request(uint64_t task_id, uint64_t res_id,
                                 uint64_t req_id, uint64_t type);
  static uint64_t get_type(uint64_t var);
  static uint64_t get_task(uint64_t var);
  static uint64_t get_res_id(uint64_t var);
  static uint64_t get_req_id(uint64_t var);

 public:
  explicit PPIPMapper(uint start_var = 0);
  uint lookup(uint task_id, uint res_id, uint req_id, var_type type);
  string key2str(uint64_t key, uint var) const;
};

class LP_RTA_GFP_PPIP : public RTA_DAG_GFP_JOSE {
 protected:
  ulong get_response_time(const DAG_Task& ti);
  uint64_t workload_bound(const DAG_Task& tx, int64_t interval);
  uint32_t instance_number(const DAG_Task& ti, int64_t interval);
  uint64_t resource_holding(const DAG_Task& ti, const DAG_Task& tx, uint32_t r_id);
  uint64_t delay_per_request(const DAG_Task& ti, uint32_t r_id);

  void directed_blocking(const DAG_Task& ti, const DAG_Task& tx,
                                 PPIPMapper* vars, LinearExpression* exp,
                                 double coef = 1);
  void indirected_blocking(const DAG_Task& ti, const DAG_Task& tx,
                                   PPIPMapper* vars,
                                   LinearExpression* exp, double coef = 1);
  void preemption_blocking(const DAG_Task& ti, const DAG_Task& tx,
                                   PPIPMapper* vars,
                                   LinearExpression* exp, double coef = 1);
  void declare_variable_bounds(const DAG_Task& ti, LinearProgram* lp,
                                       PPIPMapper* vars);
  void objective(const DAG_Task& ti, LinearProgram* lp, PPIPMapper* vars,
                         LinearExpression* obj);
  void add_constraints(const DAG_Task& ti, LinearProgram* lp,
                               PPIPMapper* vars);
  // Constraint 1&2
  void constraint_1(const DAG_Task& ti, LinearProgram* lp,
                            PPIPMapper* vars);
  // Constraint 3
  void constraint_2(const DAG_Task& ti, LinearProgram* lp,
                            PPIPMapper* vars);
  // Constraint 4
  void constraint_3(const DAG_Task& ti, LinearProgram* lp,
                            PPIPMapper* vars);
  // Constraint 5
  void constraint_4(const DAG_Task& ti, LinearProgram* lp,
                            PPIPMapper* vars);
  // Constraint 7
  void constraint_5(const DAG_Task& ti, LinearProgram* lp,
                            PPIPMapper* vars);
  // Constraint 8
  void constraint_6(const DAG_Task& ti, LinearProgram* lp,
                            PPIPMapper* vars);
  // Constraint 10
  void constraint_7(const DAG_Task& ti, LinearProgram* lp,
                            PPIPMapper* vars);
  // Constraint 11
  void constraint_8(const DAG_Task& ti, LinearProgram* lp,
                            PPIPMapper* vars);

 public:
  LP_RTA_GFP_PPIP();
  LP_RTA_GFP_PPIP(DAG_TaskSet tasks, ProcessorSet processors,
                  ResourceSet resources);
  ~LP_RTA_GFP_PPIP();
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_GLOBAL_LP_LP_RTA_GFP_PPIP_H_
