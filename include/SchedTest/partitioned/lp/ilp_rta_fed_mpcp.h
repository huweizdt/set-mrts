// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_DAG_ILP_RTA_FED_MPCP_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_DAG_ILP_RTA_FED_MPCP_H_

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
|(63-23) Reserved |(39-36) var type     |(35-26) Task          |(25-16) Resource      |(15-0) Request        |
|_________________|_____________________|______________________|______________________|______________________|
*/

class CMPCPMapper : public VarMapperBase {
 public:
  enum var_type {
    REQUEST_NUM,           
    BLOCKING_DIRECT,     
    BLOCKING_INDIRECT,
    BLOCKING_TOKEN
  };

 protected:
  static uint64_t encode_request(uint64_t task_id, uint64_t res_id,
                                 uint64_t req_id, uint64_t type);
  static uint64_t get_type(uint64_t var);
  static uint64_t get_task(uint64_t var);
  static uint64_t get_res_id(uint64_t var);
  static uint64_t get_req_id(uint64_t var);

 public:
  explicit CMPCPMapper(uint start_var = 0);
  uint64_t lookup(uint64_t task_id, uint64_t res_id, uint64_t req_id, var_type type);
  string key2str(uint64_t key, uint64_t var) const;
  string var2str(unsigned int var) const;
};

class ILP_RTA_FED_MPCP : public PartitionedSched {
 protected:
  DAG_TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;
  uint p_num_lu;

  uint32_t MPCPCeiling(const DAG_Task& tx, uint32_t r_id);

  uint64_t resource_holding(const DAG_Task& tx, uint32_t r_id);

  uint64_t delay_per_request(const DAG_Task& ti, uint32_t r_id);

  uint64_t get_response_time(const DAG_Task& ti);

  uint64_t get_total_delay(const DAG_Task& ti);

  void objective(const DAG_Task& ti, CMPCPMapper* vars, LinearExpression* obj);

  void declare_variable_bounds(const DAG_Task& ti, LinearProgram* lp,
                                       CMPCPMapper* vars);

  void add_constraints(const DAG_Task& ti, LinearProgram* lp,
                               CMPCPMapper* vars);

  void constraint_1(const DAG_Task& ti, LinearProgram* lp,
                            CMPCPMapper* vars);

  void constraint_2(const DAG_Task& ti, LinearProgram* lp,
                            CMPCPMapper* vars);

  void constraint_3(const DAG_Task& ti, LinearProgram* lp,
                            CMPCPMapper* vars);

  void constraint_4(const DAG_Task& ti, LinearProgram* lp,
                            CMPCPMapper* vars);

  void constraint_5(const DAG_Task& ti, LinearProgram* lp,
                            CMPCPMapper* vars);

  void constraint_6(const DAG_Task& ti, LinearProgram* lp,
                            CMPCPMapper* vars);

  void constraint_7(const DAG_Task& ti, LinearProgram* lp,
                            CMPCPMapper* vars);

  void constraint_8(const DAG_Task& ti, LinearProgram* lp,
                            CMPCPMapper* vars);

  bool alloc_schedulable();
 public:
  ILP_RTA_FED_MPCP();
  ILP_RTA_FED_MPCP(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~ILP_RTA_FED_MPCP();
  bool is_schedulable();
};



/*
class ILP_RTA_FED_MPCP_FF : public ILP_RTA_FED_MPCP {
 public:
  ILP_RTA_FED_MPCP_FF();
  ILP_RTA_FED_MPCP_FF(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~ILP_RTA_FED_MPCP_FF();
  bool is_schedulable();
};

*/


class ILP_RTA_FED_CMPCP : public ILP_RTA_FED_MPCP {
 protected:
  uint64_t resource_holding(const DAG_Task& tx, uint32_t r_id);

  uint64_t delay_per_request(const DAG_Task& ti, uint32_t r_id);

  uint64_t get_response_time(const DAG_Task& ti);

  uint64_t get_total_delay(const DAG_Task& ti);

  void objective(const DAG_Task& ti, CMPCPMapper* vars, LinearExpression* obj);

  void declare_variable_bounds(const DAG_Task& ti, LinearProgram* lp,
                                       CMPCPMapper* vars);

  void add_constraints(const DAG_Task& ti, LinearProgram* lp,
                               CMPCPMapper* vars);

  void constraint_1(const DAG_Task& ti, LinearProgram* lp,
                            CMPCPMapper* vars);

  void constraint_2(const DAG_Task& ti, LinearProgram* lp,
                            CMPCPMapper* vars);

  void constraint_3(const DAG_Task& ti, LinearProgram* lp,
                            CMPCPMapper* vars);

  void constraint_4(const DAG_Task& ti, LinearProgram* lp,
                            CMPCPMapper* vars);

  void constraint_5(const DAG_Task& ti, LinearProgram* lp,
                            CMPCPMapper* vars);

  void constraint_6(const DAG_Task& ti, LinearProgram* lp,
                            CMPCPMapper* vars);

  void constraint_7(const DAG_Task& ti, LinearProgram* lp,
                            CMPCPMapper* vars);

  void constraint_8(const DAG_Task& ti, LinearProgram* lp,
                            CMPCPMapper* vars);

  void constraint_9(const DAG_Task& ti, LinearProgram* lp,
                            CMPCPMapper* vars);

 public:
  ILP_RTA_FED_CMPCP();
  ILP_RTA_FED_CMPCP(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~ILP_RTA_FED_CMPCP();
  
  bool is_schedulable();
};


// class ILP_RTA_FED_MPCP_UTIL : public ILP_RTA_FED_MPCP {
//  public:
//   ILP_RTA_FED_MPCP_UTIL();
//   ILP_RTA_FED_MPCP_UTIL(DAG_TaskSet tasks, ProcessorSet processors,
//                       ResourceSet resources);
//   ~ILP_RTA_FED_MPCP_UTIL();
//   bool is_schedulable();
// };

// class ILP_RTA_FED_MPCP_HEAVY : public ILP_RTA_FED_MPCP {
//  public:
//   ILP_RTA_FED_MPCP_HEAVY();
//   ILP_RTA_FED_MPCP_HEAVY(DAG_TaskSet tasks, ProcessorSet processors,
//                       ResourceSet resources);
//   ~ILP_RTA_FED_MPCP_HEAVY();
//   bool is_schedulable();
// };

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_DAG_ILP_RTA_FED_MPCP_H_
