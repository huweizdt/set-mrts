// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_LP_ILP_RTA_PFP_ROP_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_LP_ILP_RTA_PFP_ROP_H_

#include <p_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>
#include <types.h>
#include <varmapper.h>
#include <string>

/*
|________________|_______________|_____________|_____________|_____________|____________|
|                |               |             |             |             |            |
|(63-45)Reserved |(44-40)var type|(39-30)part1 |(29-20)part2 |(19-10)part3 |(9-0)part4  |
|________________|_______________|_____________|_____________|_____________|____________|
*/

class ILPROPMapper : public VarMapperBase {
 public:
  enum var_type {
    SAME_LOCALITY,                 // S_p_q
    NOT_ALONE,                     // Z_i
    PREEMPT_NUM,                   // P_i_x
    PREEMPT_NUM_REQUEST,           // P_i_x_v
    PREEMPT_NUM_CS,                // P^CS_i_x
    TBT_PREEMPT_NUM,               // H_i_x
    RBT_PREEMPT_NUM,               // H_i_x_v resource v is requested by task x
    RBR_PREEMPT_NUM,               // H_i_x_u_v
    RESPONSE_TIME,                 // R_i
    RESPONSE_TIME_CS,              // R^CS_i_u
    BLOCKING_TIME,                 // B_i
    REQUEST_BLOCKING_TIME,         // b_i_r
    INTERFERENCE_TIME_R,           // ICS_i
    INTERFERENCE_TIME_R_RESOURCE,  // ICS_i_u
    INTERFERENCE_TIME_R_REQUEST,   // ICS_i_x_u_v
    INTERFERENCE_TIME_C,           // INCS_i
    INTERFERENCE_TIME_C_TASK,      // INCS_i_x
    INTERFERENCE_TIME_C_RESOURCE,  // INCS_i_y_u
  };

 protected:
  static uint64_t encode_request(uint64_t type, uint64_t part_1 = 0,
                                 uint64_t part_2 = 0, uint64_t part_3 = 0,
                                 uint64_t part_4 = 0);
  static uint64_t get_type(uint64_t var);
  static uint64_t get_part_1(uint64_t var);
  static uint64_t get_part_2(uint64_t var);
  static uint64_t get_part_3(uint64_t var);
  static uint64_t get_part_4(uint64_t var);

 public:
  explicit ILPROPMapper(uint start_var = 0);
  uint lookup(uint type, uint part_1 = 0, uint part_2 = 0, uint part_3 = 0,
              uint part_4 = 0);
  string var2str(unsigned int var) const;
  string key2str(uint64_t key) const;
};

/** Class ILP_RTA_PFP_ROP */

class ILP_RTA_PFP_ROP : public PartitionedSched {
 protected:
  TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  void construct_exp(ILPROPMapper* vars, LinearExpression* exp, uint type,
                     uint part_1 = 0, uint part_2 = 0, uint part_3 = 0,
                     uint part_4 = 0);
  void set_objective(LinearProgram* lp, ILPROPMapper* vars);
  void add_constraints(LinearProgram* lp, ILPROPMapper* vars);

  // Constraints
  // C1
  void constraint_1(LinearProgram* lp, ILPROPMapper* vars);
  // C2
  void constraint_2(LinearProgram* lp, ILPROPMapper* vars);
  // C3
  void constraint_3(LinearProgram* lp, ILPROPMapper* vars);
  // C4
  void constraint_4(LinearProgram* lp, ILPROPMapper* vars);
  // C5
  void constraint_5(LinearProgram* lp, ILPROPMapper* vars);
  // C6
  void constraint_6(LinearProgram* lp, ILPROPMapper* vars);
  // C7
  void constraint_7(LinearProgram* lp, ILPROPMapper* vars);
  // C7-1
  void constraint_7_1(LinearProgram* lp, ILPROPMapper* vars);
  // C8
  void constraint_8(LinearProgram* lp, ILPROPMapper* vars);
  // C8_1
  void constraint_8_1(LinearProgram* lp, ILPROPMapper* vars);
  // C8_2
  void constraint_8_2(LinearProgram* lp, ILPROPMapper* vars);
  // C8_3
  void constraint_8_3(LinearProgram* lp, ILPROPMapper* vars);
  // C8_4
  void constraint_8_4(LinearProgram* lp, ILPROPMapper* vars);
  // C8_5
  void constraint_8_5(LinearProgram* lp, ILPROPMapper* vars);
  // C9
  void constraint_9(LinearProgram* lp, ILPROPMapper* vars);
  // C10
  void constraint_10(LinearProgram* lp, ILPROPMapper* vars);
  // C11
  void constraint_11(LinearProgram* lp, ILPROPMapper* vars);
  // C12
  void constraint_12(LinearProgram* lp, ILPROPMapper* vars);
  // C13
  void constraint_13(LinearProgram* lp, ILPROPMapper* vars);
  // C14
  void constraint_14(LinearProgram* lp, ILPROPMapper* vars);
  // C15
  void constraint_15(LinearProgram* lp, ILPROPMapper* vars);
  // C16
  void constraint_16(LinearProgram* lp, ILPROPMapper* vars);
  // C17
  void constraint_17(LinearProgram* lp, ILPROPMapper* vars);
  // C17_1
  void constraint_17_1(LinearProgram* lp, ILPROPMapper* vars);
  // C18
  void constraint_18(LinearProgram* lp, ILPROPMapper* vars);
  // C19
  void constraint_19(LinearProgram* lp, ILPROPMapper* vars);
  // C20
  void constraint_20(LinearProgram* lp, ILPROPMapper* vars);
  // TEST
  void constraint_for_specific_alloc(LinearProgram* lp, ILPROPMapper* vars);

  void specific_partition();

  bool alloc_schedulable();

 public:
  ILP_RTA_PFP_ROP();
  ILP_RTA_PFP_ROP(TaskSet tasks, ProcessorSet processors,
                  ResourceSet resources);
  ~ILP_RTA_PFP_ROP();
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_LP_ILP_RTA_PFP_ROP_H_
