// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_DAG_ILP_RTA_FED_H2LP_v2_V2_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_DAG_ILP_RTA_FED_H2LP_v2_V2_H_

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

/* b_{x,q,u,v}
|_________________|_____________________|______________________|______________________|______________________|
|                 |                     |                      |                      |                      |
|(63-39) Reserved |(39-30) x            |(29-26) q             |(25-10) u             |(9-0) v               |
|_________________|_____________________|______________________|______________________|______________________|
*/

class H2LPMapper_v2 : public VarMapperBase {
 protected:
  static uint64_t encode_request(uint64_t part_1 = 0, uint64_t part_2 = 0,
                                 uint64_t part_3 = 0, uint64_t part_4 = 0);
  static uint64_t get_part_1(uint64_t var);
  static uint64_t get_part_2(uint64_t var);
  static uint64_t get_part_3(uint64_t var);
  static uint64_t get_part_4(uint64_t var);

 public:
  explicit H2LPMapper_v2(uint start_var = 0);
  uint lookup(uint part_1, uint part_, uint part_3, uint part_4);
  string key2str(uint64_t key, uint var) const;
  string var2str(unsigned int var) const;
};

class ILP_RTA_FED_H2LP_v2 : public PartitionedSched {
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


  void total_blocking(const DAG_Task& ti, H2LPMapper_v2* vars, 
                      LinearExpression* exp, int32_t res_id, double coef = 1);
  void blocking_on_cp(const DAG_Task& ti, H2LPMapper_v2* vars, LinearExpression* exp, 
                      int32_t res_id, int32_t request_num, double coef = 1);
  void blocking_off_cp(const DAG_Task& ti, H2LPMapper_v2* vars, LinearExpression* exp, 
                       int32_t res_id, int32_t request_num, double coef = 1);

  void declare_variable_bounds(const DAG_Task& ti, LinearProgram* lp, 
                                       H2LPMapper_v2* vars, int32_t res_id);

  void objective_heavy(const DAG_Task& ti, H2LPMapper_v2* vars, LinearExpression* obj, int32_t res_id, int32_t request_num);

  void objective_light(const DAG_Task& ti, H2LPMapper_v2* vars, LinearExpression* obj, int32_t res_id);

  void add_constraints(const DAG_Task& ti, LinearProgram* lp,
                               H2LPMapper_v2* vars, int32_t res_id);

  void constraint_1(const DAG_Task& ti, LinearProgram* lp,
                            H2LPMapper_v2* vars, int32_t res_id);

  void constraint_2(const DAG_Task& ti, LinearProgram* lp,
                            H2LPMapper_v2* vars, int32_t res_id);

  void constraint_3(const DAG_Task& ti, LinearProgram* lp,
                            H2LPMapper_v2* vars, int32_t res_id, int32_t p_num_lu);

  bool alloc_schedulable();
 public:
  ILP_RTA_FED_H2LP_v2();
  ILP_RTA_FED_H2LP_v2(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~ILP_RTA_FED_H2LP_v2();
  bool is_schedulable();
};


// class ILP_RTA_FED_H2LP_v2_FF : public ILP_RTA_FED_H2LP_v2 {
//  public:
//   ILP_RTA_FED_H2LP_v2_FF();
//   ILP_RTA_FED_H2LP_v2_FF(DAG_TaskSet tasks, ProcessorSet processors,
//                       ResourceSet resources);
//   ~ILP_RTA_FED_H2LP_v2_FF();
//   bool is_schedulable();
// };


// class ILP_RTA_FED_H2LP_v2_HEAVY : public ILP_RTA_FED_H2LP_v2 {
//  public:
//   ILP_RTA_FED_H2LP_v2_HEAVY();
//   ILP_RTA_FED_H2LP_v2_HEAVY(DAG_TaskSet tasks, ProcessorSet processors,
//                       ResourceSet resources);
//   ~ILP_RTA_FED_H2LP_v2_HEAVY();
//   bool is_schedulable();
// };

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_DAG_ILP_RTA_FED_H2LP_v2_H_
