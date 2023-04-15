// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_DAG_ILP_RTA_FED_H2LP_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_DAG_ILP_RTA_FED_H2LP_H_

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
|_________________|_____________________|______________________|______________________|
|                 |                     |                      |                      |
|(63-23) Reserved |(22-20) var type     |(19-10) Resource      |(9-0) Request         |
|_________________|_____________________|______________________|______________________|
*/

class H2LPMapper : public VarMapperBase {
 public:
  enum var_type {
    REQUEST_NUM,        
    BLOCKING_DIRECT,     
    BLOCKING_INDIRECT,   
    BLOCKING_PREEMPT,    
    BLOCKING_OTHER,      
    INTERF_INTRA,        
    // INTERF_INTER,        
    INTERF_OTHER         
  };

 protected:
  static uint64_t encode_request(uint64_t res_id,
                                 uint64_t req_id, uint64_t type);
  static uint64_t get_type(uint64_t var);
  static uint64_t get_res_id(uint64_t var);
  static uint64_t get_req_id(uint64_t var);

 public:
  explicit H2LPMapper(uint start_var = 0);
  uint lookup(uint res_id, uint req_id, var_type type);
  string key2str(uint64_t key, uint var) const;
  string var2str(unsigned int var) const;
};

class ILP_RTA_FED_H2LP : public PartitionedSched {
 protected:
  DAG_TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;
  uint p_num_lu;

  // Lemma 1
  uint64_t blocking_spin(const DAG_Task& ti, uint32_t res_id);
  uint64_t blocking_spin_total(const DAG_Task& ti);

  // Lemma 2
  uint64_t blocking_susp(const DAG_Task& ti, uint32_t res_id, int32_t Y);

  // Lemma 3
  uint64_t blocking_global(const DAG_Task& ti);

  // Lemma 4
  uint64_t blocking_local(const DAG_Task& ti);

  uint64_t interference(const DAG_Task& ti, uint64_t interval);

  // Theorem 1
  uint64_t get_response_time_HUT(const DAG_Task& ti);
  uint64_t get_response_time_HUT_2(const DAG_Task& ti);

  // Theorem 2
  uint64_t get_response_time_LUT(const DAG_Task& ti);


  void lp_h2lp_directed_blocking(const DAG_Task& ti,
                                 H2LPMapper* vars, LinearExpression* exp,
                                 double coef = 1);
  void lp_h2lp_indirected_blocking(const DAG_Task& ti,
                                   H2LPMapper* vars, LinearExpression* exp, 
                                   double coef = 1);
  void lp_h2lp_preemption_blocking(const DAG_Task& ti,
                                   H2LPMapper* vars,
                                   LinearExpression* exp, double coef = 1);

  void lp_h2lp_declare_variable_bounds(const DAG_Task& ti, LinearProgram* lp,
                                       H2LPMapper* vars);

  void lp_h2lp_declare_variable_bounds_2(const DAG_Task& ti, LinearProgram* lp,
                                       H2LPMapper* vars);

  void lp_h2lp_objective(const DAG_Task& ti, H2LPMapper* vars, LinearExpression* obj);

  void lp_h2lp_add_constraints(const DAG_Task& ti, LinearProgram* lp,
                               H2LPMapper* vars);

  void lp_h2lp_constraint_1(const DAG_Task& ti, LinearProgram* lp,
                            H2LPMapper* vars);

  void lp_h2lp_constraint_2(const DAG_Task& ti, LinearProgram* lp,
                            H2LPMapper* vars);

  void lp_h2lp_constraint_3(const DAG_Task& ti, LinearProgram* lp,
                            H2LPMapper* vars);

  void lp_h2lp_constraint_4(const DAG_Task& ti, LinearProgram* lp,
                            H2LPMapper* vars);

  void lp_h2lp_constraint_5(const DAG_Task& ti, LinearProgram* lp,
                            H2LPMapper* vars);

  void lp_h2lp_constraint_6(const DAG_Task& ti, LinearProgram* lp,
                            H2LPMapper* vars);

  bool alloc_schedulable();
 public:
  ILP_RTA_FED_H2LP();
  ILP_RTA_FED_H2LP(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~ILP_RTA_FED_H2LP();
  bool is_schedulable();
};

class ILP_RTA_FED_H2LP_FF : public ILP_RTA_FED_H2LP {
 public:
  ILP_RTA_FED_H2LP_FF();
  ILP_RTA_FED_H2LP_FF(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~ILP_RTA_FED_H2LP_FF();
  bool is_schedulable();
};


class ILP_RTA_FED_H2LP_HEAVY : public ILP_RTA_FED_H2LP {
 public:
  ILP_RTA_FED_H2LP_HEAVY();
  ILP_RTA_FED_H2LP_HEAVY(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~ILP_RTA_FED_H2LP_HEAVY();
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_DAG_ILP_RTA_FED_H2LP_H_
