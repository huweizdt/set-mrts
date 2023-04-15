// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_DAG_LP_RTA_FED_SPIN_FIFO_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_DAG_LP_RTA_FED_SPIN_FIFO_H_

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
|(63-42) Reserved |(41-40) var type     |(39-30) Task          |(29-15) blocking-r    |(14-0) blocked-r      |
|_________________|_____________________|______________________|______________________|______________________|
*/

class SPINMapper : public VarMapperBase {
 public:
  enum var_type {
    BLOCKING_SPIN,
    BLOCKING_TRANS
  };

 protected:
  static uint64_t encode_request(uint64_t task_id, uint64_t res_id,
                                 uint64_t req_id, uint64_t type);
  static uint64_t get_type(uint64_t var);
  static uint64_t get_task(uint64_t var);
  static uint64_t get_req_1(uint64_t var);
  static uint64_t get_req_2(uint64_t var);

 public:
  explicit SPINMapper(uint start_var = 0);
  uint64_t lookup(uint64_t task_id, uint64_t res_id, uint64_t req_id, var_type type);
  string key2str(uint64_t key, uint64_t var) const;
  string var2str(unsigned int var) const;
};

class LP_RTA_FED_SPIN_FIFO : public PartitionedSched {
 protected:
  DAG_TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  uint64_t resource_delay(const DAG_Task& ti, uint32_t res_id);
  uint64_t total_resource_delay(const DAG_Task& ti);

  uint64_t get_response_time(const DAG_Task& ti);

  void objective(const DAG_Task& ti, uint res_id, uint res_num, SPINMapper* vars, LinearExpression* obj);

  void add_constraints(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                               SPINMapper* vars);

  void constraint_1(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                            SPINMapper* vars);

  void constraint_2(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                            SPINMapper* vars);

  void constraint_3(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                            SPINMapper* vars);

  void constraint_4(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                            SPINMapper* vars);

  void constraint_5(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                            SPINMapper* vars);

  void constraint_6(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                            SPINMapper* vars);

  void constraint_7(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                            SPINMapper* vars);

  void constraint_8(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                            SPINMapper* vars);

  bool alloc_schedulable();
 public:
  LP_RTA_FED_SPIN_FIFO();
  LP_RTA_FED_SPIN_FIFO(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~LP_RTA_FED_SPIN_FIFO();
  bool is_schedulable();
};

/*
class LP_RTA_FED_SPIN_FIFO_FF : public LP_RTA_FED_SPIN_FIFO {
 public:
  LP_RTA_FED_SPIN_FIFO_FF();
  LP_RTA_FED_SPIN_FIFO_FF(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~LP_RTA_FED_SPIN_FIFO_FF();
  bool is_schedulable();
};

*/

// linear optimization only for intra-task blocking
class LP_RTA_FED_SPIN_FIFO_v2 : public LP_RTA_FED_SPIN_FIFO {
 protected:
  uint64_t inter_task_blocking(const DAG_Task& ti, uint32_t res_id, uint32_t request_num);
  uint64_t resource_delay(const DAG_Task& ti, uint32_t res_id);
  uint64_t total_resource_delay(const DAG_Task& ti);

  uint64_t get_response_time(const DAG_Task& ti);

  void objective(const DAG_Task& ti, uint res_id, uint res_num, SPINMapper* vars, LinearExpression* obj);

  void add_constraints(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                               SPINMapper* vars);

  void constraint_1(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                            SPINMapper* vars);

  void constraint_5(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                            SPINMapper* vars);

  void constraint_6(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                            SPINMapper* vars);

  void constraint_7(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                            SPINMapper* vars);

  void constraint_8(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                            SPINMapper* vars);
 public:
  LP_RTA_FED_SPIN_FIFO_v2();
  LP_RTA_FED_SPIN_FIFO_v2(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~LP_RTA_FED_SPIN_FIFO_v2();
  bool is_schedulable();
};


class LP_RTA_FED_SPIN_FIFO_v3 : public LP_RTA_FED_SPIN_FIFO {
 protected:
  uint64_t resource_delay(const DAG_Task& ti, uint32_t res_id);
  uint64_t total_resource_delay(const DAG_Task& ti);
  uint64_t get_response_time(const DAG_Task& ti);
  void objective(const DAG_Task& ti, uint res_id, uint res_num, SPINMapper* vars, LinearExpression* obj);
 public:
  LP_RTA_FED_SPIN_FIFO_v3();
  LP_RTA_FED_SPIN_FIFO_v3(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~LP_RTA_FED_SPIN_FIFO_v3();
  bool is_schedulable();
};



class LP_RTA_FED_SPIN_PRIO : public PartitionedSched {
 protected:
  DAG_TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

                                       
  uint64_t higher(const DAG_Task& ti, uint32_t res_id, uint64_t interval);
  uint64_t lower(const DAG_Task& ti, uint32_t res_id);
  uint64_t equal(const DAG_Task& ti, uint32_t res_id);
  uint64_t dpr(const DAG_Task& ti, uint32_t res_id);

  uint64_t resource_delay(const DAG_Task& ti, uint32_t res_id);
  uint64_t total_resource_delay(const DAG_Task& ti);

  uint64_t get_response_time(const DAG_Task& ti);

  void objective(const DAG_Task& ti, uint res_id, uint res_num, SPINMapper* vars, LinearExpression* obj);

  void add_constraints(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                               SPINMapper* vars);

  void constraint_1(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                            SPINMapper* vars);

  void constraint_2(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                            SPINMapper* vars);

  void constraint_3(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                            SPINMapper* vars);

  void constraint_3_p(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                            SPINMapper* vars);

  void constraint_4(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                            SPINMapper* vars);

  void constraint_4_p(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                            SPINMapper* vars);

  void constraint_5(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                            SPINMapper* vars);

  void constraint_6(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                            SPINMapper* vars);

  void constraint_7(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                            SPINMapper* vars);

  void constraint_8(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
                            SPINMapper* vars);

  bool alloc_schedulable();
 public:
  LP_RTA_FED_SPIN_PRIO();
  LP_RTA_FED_SPIN_PRIO(DAG_TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~LP_RTA_FED_SPIN_PRIO();
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_DAG_LP_RTA_FED_SPIN_FIFO_H_
