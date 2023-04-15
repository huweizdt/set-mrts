// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_GLOBAL_DAG_LP_RTA_DAG_GFP_PIP_H_
#define INCLUDE_SCHEDTEST_GLOBAL_DAG_LP_RTA_DAG_GFP_PIP_H_

/*
** LinearProgramming approach for global fix-priority scheduling under PIP
*locking protocol
**
** RTSS 2015 Maolin Yang et al. [Global Real-Time Semaphore Protocols: A Survey,
*Unified Analysis, and Comparison]
*/

#include <g_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>
#include <varmapper.h>
#include <string>

class DAG_Task;
class DAG_TaskSet;
class Request;
class Resource;
class ResourceSet;
class ProcessorSet;
class LinearExpression;
class LinearProgram;

/*
|________________|_____________________|______________________|______________________|______________________|
|                |                     |                      | | |
|(63-34)Reserved |(32-30) var type     |(29-20) Task          |(19-10) Resource
|(9-0) Request         |
|________________|_____________________|______________________|______________________|______________________|
*/
class DAG_PIPMapper : public VarMapperBase {
 public:
  enum var_type {
    BLOCKING_DIRECT,     // 0x000
    BLOCKING_INDIRECT,   // 0x001
    BLOCKING_PREEMPT,    // 0x010
    BLOCKING_OTHER,      // 0x011
    INTERF_REGULAR,      // 0x100
    INTERF_CO_BOOSTING,  // 0x101
    INTERF_STALLING,     // 0x110
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
  explicit DAG_PIPMapper(uint start_var = 0);
  uint lookup(uint task_id, uint res_id, uint req_id, var_type type);
  string key2str(uint64_t key, uint var) const;
};

class LP_RTA_DAG_GFP_PIP : public GlobalSched {
 protected:
  DAG_TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  ulong response_time(const DAG_Task& ti);
  ulong workload_bound(const DAG_Task& tx, ulong Ri);
  ulong holding_bound(const DAG_Task& ti, const DAG_Task& tx, uint resource_id);
  ulong wait_time_bound(const DAG_Task& ti, uint resource_id);
  void lp_pip_directed_blocking(const DAG_Task& ti, const DAG_Task& tx,
                                DAG_PIPMapper* vars, LinearExpression* exp,
                                double coef = 1);
  void lp_pip_indirected_blocking(const DAG_Task& ti, const DAG_Task& tx,
                                  DAG_PIPMapper* vars, LinearExpression* exp,
                                  double coef = 1);
  void lp_pip_preemption_blocking(const DAG_Task& ti, const DAG_Task& tx,
                                  DAG_PIPMapper* vars, LinearExpression* exp,
                                  double coef = 1);
  void lp_pip_OD(const DAG_Task& ti, DAG_PIPMapper* vars, LinearExpression* exp,
                 double coef = 1);
  void lp_pip_declare_variable_bounds(const DAG_Task& ti, LinearProgram* lp,
                                      DAG_PIPMapper* vars);
  void lp_pip_objective(const DAG_Task& ti, LinearProgram* lp,
                        DAG_PIPMapper* vars, LinearExpression* obj);
  void lp_pip_add_constraints(const DAG_Task& ti, LinearProgram* lp,
                              DAG_PIPMapper* vars);
  // Constraint 1 [Maolin 2015 RTSS]
  void lp_pip_constraint_1(const DAG_Task& ti, LinearProgram* lp,
                           DAG_PIPMapper* vars);
  // Constraint 2 [Maolin 2015 RTSS]
  void lp_pip_constraint_2(const DAG_Task& ti, LinearProgram* lp,
                           DAG_PIPMapper* vars);
  // Constraint 3 [Maolin 2015 RTSS]
  void lp_pip_constraint_3(const DAG_Task& ti, LinearProgram* lp,
                           DAG_PIPMapper* vars);
  // Constraint 4 [Maolin 2015 RTSS]
  void lp_pip_constraint_4(const DAG_Task& ti, LinearProgram* lp,
                           DAG_PIPMapper* vars);
  // Constraint 5 [Maolin 2015 RTSS]
  void lp_pip_constraint_5(const DAG_Task& ti, LinearProgram* lp,
                           DAG_PIPMapper* vars);
  // Constraint 6 [Maolin 2015 RTSS]
  void lp_pip_constraint_6(const DAG_Task& ti, LinearProgram* lp,
                           DAG_PIPMapper* vars);
  // Constraint 7 [Maolin 2015 RTSS]
  void lp_pip_constraint_7(const DAG_Task& ti, LinearProgram* lp,
                           DAG_PIPMapper* vars);
  // Constraint 9 [Maolin 2015 RTSS]
  void lp_pip_constraint_8(const DAG_Task& ti, LinearProgram* lp,
                           DAG_PIPMapper* vars);
  // Constraint 10 [Maolin 2015 RTSS]
  void lp_pip_constraint_9(const DAG_Task& ti, LinearProgram* lp,
                           DAG_PIPMapper* vars);
  // Constraint 11 [Maolin 2015 RTSS]
  void lp_pip_constraint_10(const DAG_Task& ti, LinearProgram* lp,
                            DAG_PIPMapper* vars);
  // Constraint 12 [Maolin 2015 RTSS]
  void lp_pip_constraint_11(const DAG_Task& ti, LinearProgram* lp,
                            DAG_PIPMapper* vars);

 public:
  LP_RTA_DAG_GFP_PIP();
  LP_RTA_DAG_GFP_PIP(DAG_TaskSet tasks, ProcessorSet processors,
                     ResourceSet resources);
  ~LP_RTA_DAG_GFP_PIP();
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_GLOBAL_DAG_LP_RTA_DAG_GFP_PIP_H_