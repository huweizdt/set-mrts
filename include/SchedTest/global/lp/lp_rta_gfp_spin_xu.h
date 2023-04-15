// Copyright [2021] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_GLOBAL_LP_LP_RTA_GFP_SPIN_XU_H_
#define INCLUDE_SCHEDTEST_GLOBAL_LP_LP_RTA_GFP_SPIN_XU_H_

/*
*
*   
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
|________________|_____________________|______________________|______________________|______________________|______________________|
|                |                     |                      |                      |                      |                      |
|(63-34)Reserved |(38-35) var type     |(34-25) Task          |(24-20) resource      |(19-10) request       |(9-0) processor       |
|________________|_____________________|______________________|______________________|______________________|______________________|
*/
class GFP_SPIN_XUMapper : public VarMapperBase {
 public:
  enum var_type {
    BLOCKING_KP,
    BLOCKING_NPD,
    BLOCKING_DD,
    REQUEST_NUM
  };

 protected:
  static uint64_t encode_request(uint64_t task, uint64_t resource,
                                 uint64_t request, uint64_t processor, 
                                 uint64_t type);
  static uint64_t get_type(uint64_t var);
  static uint64_t get_task(uint64_t var);
  static uint64_t get_resource(uint64_t var);
  static uint64_t get_request(uint64_t var);
  static uint64_t get_processor(uint64_t var);

 public:
  explicit GFP_SPIN_XUMapper(uint start_var = 0);
  uint lookup(uint task, uint resource, uint request, uint processor, var_type type);
  string key2str(uint64_t key, uint var) const;
};

class LP_RTA_GFP_SPIN_XU_UNOR : public RTA_DAG_GFP_JOSE {
 protected:
  uint64_t resource_delay(const DAG_Task& ti);
  ulong get_response_time(const DAG_Task& ti);
  uint64_t workload_bound(const DAG_Task& tx, int64_t interval);

  void declare_variable_bounds(const DAG_Task& ti, LinearProgram* lp,
                                       GFP_SPIN_XUMapper* vars);
  void objective(const DAG_Task& ti, GFP_SPIN_XUMapper* vars,
                         LinearExpression* obj);
  void add_constraints(const DAG_Task& ti, LinearProgram* lp,
                               GFP_SPIN_XUMapper* vars);
  // 
  void constraint_1(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_2(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_3(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_4(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_5(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_6(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);

 public:
  LP_RTA_GFP_SPIN_XU_UNOR();
  LP_RTA_GFP_SPIN_XU_UNOR(DAG_TaskSet tasks, ProcessorSet processors,
                  ResourceSet resources);
  ~LP_RTA_GFP_SPIN_XU_UNOR();
  bool is_schedulable();
};

class LP_RTA_GFP_SPIN_XU_FIFO : public RTA_DAG_GFP_JOSE {
 protected:
  uint64_t resource_delay(const DAG_Task& ti);
  ulong get_response_time(const DAG_Task& ti);
  uint64_t workload_bound(const DAG_Task& tx, int64_t interval);

  void declare_variable_bounds(const DAG_Task& ti, LinearProgram* lp,
                                       GFP_SPIN_XUMapper* vars);
  void objective(const DAG_Task& ti, GFP_SPIN_XUMapper* vars,
                         LinearExpression* obj);
  void add_constraints(const DAG_Task& ti, LinearProgram* lp,
                               GFP_SPIN_XUMapper* vars);
  // 
  void constraint_1(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_2(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_3(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_4(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_5(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_6(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_7(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);

 public:
  LP_RTA_GFP_SPIN_XU_FIFO();
  LP_RTA_GFP_SPIN_XU_FIFO(DAG_TaskSet tasks, ProcessorSet processors,
                  ResourceSet resources);
  ~LP_RTA_GFP_SPIN_XU_FIFO();
  bool is_schedulable();
};

class LP_RTA_GFP_SPIN_XU_PRIO : public RTA_DAG_GFP_JOSE {
 protected:

  uint64_t higher(const DAG_Task& ti, uint32_t res_id, uint64_t interval);
  uint64_t lower(const DAG_Task& ti, uint32_t res_id);
  uint64_t equal(const DAG_Task& ti, uint32_t res_id);
  uint64_t dpr(const DAG_Task& ti, uint32_t res_id);

  uint64_t resource_delay(const DAG_Task& ti);
  ulong get_response_time(const DAG_Task& ti);
  uint64_t workload_bound(const DAG_Task& tx, int64_t interval);

  void declare_variable_bounds(const DAG_Task& ti, LinearProgram* lp,
                                       GFP_SPIN_XUMapper* vars);
  void objective(const DAG_Task& ti, GFP_SPIN_XUMapper* vars,
                         LinearExpression* obj);
  void add_constraints(const DAG_Task& ti, LinearProgram* lp,
                               GFP_SPIN_XUMapper* vars);
  // 
  void constraint_1(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_2(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_3(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_4(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_5(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_6(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_12(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_13(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);

 public:
  LP_RTA_GFP_SPIN_XU_PRIO();
  LP_RTA_GFP_SPIN_XU_PRIO(DAG_TaskSet tasks, ProcessorSet processors,
                  ResourceSet resources);
  ~LP_RTA_GFP_SPIN_XU_PRIO();
  bool is_schedulable();
};

class LP_RTA_GFP_SPIN_XU_UNOR_P : public LP_RTA_GFP_SPIN_XU_UNOR {
 protected:
  uint64_t resource_delay(const DAG_Task& ti);
  ulong get_response_time(const DAG_Task& ti);

  void add_constraints(const DAG_Task& ti, LinearProgram* lp,
                               GFP_SPIN_XUMapper* vars);
  // 
  void constraint_8(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_9(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_10(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_11(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);



 public:
  LP_RTA_GFP_SPIN_XU_UNOR_P();
  LP_RTA_GFP_SPIN_XU_UNOR_P(DAG_TaskSet tasks, ProcessorSet processors,
                  ResourceSet resources);
  ~LP_RTA_GFP_SPIN_XU_UNOR_P();
  bool is_schedulable();
};


class LP_RTA_GFP_SPIN_XU_FIFO_P : public LP_RTA_GFP_SPIN_XU_FIFO {
 protected:
  uint64_t resource_delay(const DAG_Task& ti);
  ulong get_response_time(const DAG_Task& ti);

  void add_constraints(const DAG_Task& ti, LinearProgram* lp,
                               GFP_SPIN_XUMapper* vars);
  // 
  void constraint_8(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_9(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_10(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_11(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);



 public:
  LP_RTA_GFP_SPIN_XU_FIFO_P();
  LP_RTA_GFP_SPIN_XU_FIFO_P(DAG_TaskSet tasks, ProcessorSet processors,
                  ResourceSet resources);
  ~LP_RTA_GFP_SPIN_XU_FIFO_P();
  bool is_schedulable();
};


class LP_RTA_GFP_SPIN_XU_PRIO_P : public LP_RTA_GFP_SPIN_XU_PRIO {
 protected:
  uint64_t resource_delay(const DAG_Task& ti);
  ulong get_response_time(const DAG_Task& ti);

  void add_constraints(const DAG_Task& ti, LinearProgram* lp,
                               GFP_SPIN_XUMapper* vars);
  // 
  void constraint_8(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_9(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_10(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);
  // 
  void constraint_11(const DAG_Task& ti, LinearProgram* lp,
                            GFP_SPIN_XUMapper* vars);



 public:
  LP_RTA_GFP_SPIN_XU_PRIO_P();
  LP_RTA_GFP_SPIN_XU_PRIO_P(DAG_TaskSet tasks, ProcessorSet processors,
                  ResourceSet resources);
  ~LP_RTA_GFP_SPIN_XU_PRIO_P();
  bool is_schedulable();
};





// class LP_RTA_GFP_SPIN_UNOR : public RTA_DAG_GFP_JOSE {
//  protected:

//   uint64_t higher(const DAG_Task& ti, uint32_t res_id, uint64_t interval);
//   uint64_t lower(const DAG_Task& ti, uint32_t res_id);
//   uint64_t equal(const DAG_Task& ti, uint32_t res_id);
//   uint64_t dpr(const DAG_Task& ti, uint32_t res_id);

//   uint64_t resource_delay(const DAG_Task& ti, uint res_id);
//   uint64_t total_resource_delay(const DAG_Task& ti);

//   ulong get_response_time(const DAG_Task& ti);
//   uint64_t workload_bound(const DAG_Task& tx, int64_t interval);

//   void declare_variable_bounds(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                                        GFP_SPIN_XUMapper* vars);
//   void objective(const DAG_Task& ti, uint res_id, uint res_num, GFP_SPIN_XUMapper* vars,
//                          LinearExpression* obj);
//   void add_constraints(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                                GFP_SPIN_XUMapper* vars);
//   // 
//   void constraint_1(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                             GFP_SPIN_XUMapper* vars);
//   // 
//   void constraint_2(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                             GFP_SPIN_XUMapper* vars);
//   // 
//   void constraint_3(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                             GFP_SPIN_XUMapper* vars);
//   // 
//   void constraint_4(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                             GFP_SPIN_XUMapper* vars);
//   // 
//   void constraint_11(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                             GFP_SPIN_XUMapper* vars);
//   // 
//   void constraint_12(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                             GFP_SPIN_XUMapper* vars);

//  public:
//   LP_RTA_GFP_SPIN_UNOR();
//   LP_RTA_GFP_SPIN_UNOR(DAG_TaskSet tasks, ProcessorSet processors,
//                   ResourceSet resources);
//   ~LP_RTA_GFP_SPIN_UNOR();
//   bool is_schedulable();
// };

// class LP_RTA_GFP_SPIN_GENERAL : public RTA_DAG_GFP_JOSE {
//  protected:
//   uint64_t resource_delay(const DAG_Task& ti, uint res_id);
//   uint64_t total_resource_delay(const DAG_Task& ti);
//   ulong get_response_time(const DAG_Task& ti);
//   uint64_t workload_bound(const DAG_Task& tx, int64_t interval);

//   void declare_variable_bounds(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                                        GFP_SPIN_XUMapper* vars);
//   void objective(const DAG_Task& ti, uint res_id, uint res_num, GFP_SPIN_XUMapper* vars,
//                          LinearExpression* obj);
//   void add_constraints(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                                GFP_SPIN_XUMapper* vars);
//   // 
//   void constraint_1(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                             GFP_SPIN_XUMapper* vars);
//   // 
//   void constraint_2(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                             GFP_SPIN_XUMapper* vars);
//   // 
//   void constraint_3(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                             GFP_SPIN_XUMapper* vars);
//   // 
//   void constraint_4(const DAG_Task& ti, uint res_id, uint res_num, LinearProgram* lp,
//                             GFP_SPIN_XUMapper* vars);

//  public:
//   LP_RTA_GFP_SPIN_GENERAL();
//   LP_RTA_GFP_SPIN_GENERAL(DAG_TaskSet tasks, ProcessorSet processors,
//                   ResourceSet resources);
//   ~LP_RTA_GFP_SPIN_GENERAL();
//   bool is_schedulable();
// };

#endif  // INCLUDE_SCHEDTEST_GLOBAL_LP_LP_RTA_GFP_SPIN_XU_H_
