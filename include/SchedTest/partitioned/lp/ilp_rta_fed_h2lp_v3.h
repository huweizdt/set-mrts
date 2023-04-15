// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_DAG_ILP_RTA_FED_H2LP_V3_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_DAG_ILP_RTA_FED_H2LP_V3_H_

/**
 * 
 * 
 */


#include <p_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>
#include <types.h>
#include <varmapper.h>
#include <string>
#include <set>

/*
|_________________|_____________________|______________________|______________________|______________________|
|                 |                     |                      |                      |                      |
|(63-23) Reserved |(38-36) var type     |(35-26) Task          |(25-16) Resource      |(15-0) Request        |
|_________________|_____________________|______________________|______________________|______________________|
*/

class H2LPMapper_v3 : public VarMapperBase {
 public:
  enum var_type {
    REQUEST_NUM,   
    BLOCKING_SPIN,         
    BLOCKING_DIRECT,     
    BLOCKING_INDIRECT,   
    BLOCKING_PREEMPT,  
    INTERF    
  };

 protected:
  static uint64_t encode_request(uint64_t task_id, uint64_t res_id,
                                 uint64_t req_id, uint64_t type);
  static uint64_t get_type(uint64_t var);
  static uint64_t get_task(uint64_t var);
  static uint64_t get_res_id(uint64_t var);
  static uint64_t get_req_id(uint64_t var);

 public:
  explicit H2LPMapper_v3(uint start_var = 0);
  uint64_t lookup(uint64_t task_id, uint64_t res_id, uint64_t req_id, var_type type);
  string key2str(uint64_t key, uint64_t var) const;
  string var2str(unsigned int var) const;
};

class ILP_RTA_FED_H2LP_v3 : public PartitionedSched {
 protected:
  DAG_TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;
  uint p_num_lu;

  uint64_t blocking_spin(const DAG_Task& ti, uint32_t res_id);
  uint64_t blocking_spin_total(const DAG_Task& ti);

  uint64_t workload_bound(const DAG_Task& ti, const DAG_Task& tx);

  uint64_t get_response_time(const DAG_Task& ti);

  void objective(const DAG_Task& ti, H2LPMapper_v3* vars, LinearExpression* obj);

  void declare_variable_bounds(const DAG_Task& ti, LinearProgram* lp,
                                       H2LPMapper_v3* vars);

  void lp_h2lp_add_constraints(const DAG_Task& ti, LinearProgram* lp,
                               H2LPMapper_v3* vars);

  void spining_blocking(const DAG_Task& ti,
                        H2LPMapper_v3* vars, LinearExpression* exp,
                        double coef = 1);

  void direct_blocking(const DAG_Task& ti,
                         H2LPMapper_v3* vars, LinearExpression* exp,
                         double coef = 1);
  void indirect_blocking(const DAG_Task& ti,
                            H2LPMapper_v3* vars, LinearExpression* exp, 
                            double coef = 1);
  void preemption_blocking(const DAG_Task& ti,
                            H2LPMapper_v3* vars,
                            LinearExpression* exp, double coef = 1);

  void constraint_1(const DAG_Task& ti, LinearProgram* lp,
                            H2LPMapper_v3* vars);

  void constraint_2(const DAG_Task& ti, LinearProgram* lp,
                            H2LPMapper_v3* vars);

  void constraint_3(const DAG_Task& ti, LinearProgram* lp,
                            H2LPMapper_v3* vars);

  void constraint_4(const DAG_Task& ti, LinearProgram* lp,
                            H2LPMapper_v3* vars);

  void constraint_5(const DAG_Task& ti, LinearProgram* lp,
                            H2LPMapper_v3* vars);

  void constraint_6(const DAG_Task& ti, LinearProgram* lp,
                            H2LPMapper_v3* vars);

  void constraint_7(const DAG_Task& ti, LinearProgram* lp,
                            H2LPMapper_v3* vars);

  void constraint_8(const DAG_Task& ti, LinearProgram* lp,
                            H2LPMapper_v3* vars);

  void constraint_9(const DAG_Task& ti, LinearProgram* lp,
                            H2LPMapper_v3* vars);

  void constraint_10(const DAG_Task& ti, LinearProgram* lp,
                            H2LPMapper_v3* vars);

  void constraint_11(const DAG_Task& ti, LinearProgram* lp,
                            H2LPMapper_v3* vars);

  void constraint_12(const DAG_Task& ti, LinearProgram* lp,
                            H2LPMapper_v3* vars);

  void constraint_13(const DAG_Task& ti, LinearProgram* lp,
                            H2LPMapper_v3* vars);

  bool alloc_schedulable();
 public:
  ILP_RTA_FED_H2LP_v3();
  ILP_RTA_FED_H2LP_v3(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~ILP_RTA_FED_H2LP_v3();
  bool is_schedulable();
};

/*
class ILP_RTA_FED_H2LP_v3_FF : public ILP_RTA_FED_H2LP_v3 {
 public:
  ILP_RTA_FED_H2LP_v3_FF();
  ILP_RTA_FED_H2LP_v3_FF(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~ILP_RTA_FED_H2LP_v3_FF();
  bool is_schedulable();
};

*/

class ILP_RTA_FED_H2LP_v3_HEAVY : public ILP_RTA_FED_H2LP_v3 {
 public:
  ILP_RTA_FED_H2LP_v3_HEAVY();
  ILP_RTA_FED_H2LP_v3_HEAVY(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~ILP_RTA_FED_H2LP_v3_HEAVY();
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_DAG_ILP_RTA_FED_H2LP_V3_H_
