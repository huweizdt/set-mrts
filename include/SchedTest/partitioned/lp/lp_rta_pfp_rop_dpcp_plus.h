// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_LP_LP_RTA_PFP_ROP_DPCP_PLUS_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_LP_LP_RTA_PFP_ROP_DPCP_PLUS_H_

#include <p_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>
#include <varmapper.h>
#include <lp_rta_pfp_dpcp.h>
#include <string>

class Task;
class TaskSet;
class Request;
class Resource;
class ResourceSet;
class ProcessorSet;
class LinearExpression;
class LinearProgram;

class LP_RTA_PFP_ROP_DPCP_PLUS : public PartitionedSched {
 protected:
  TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  // ROP
  ulong blocking_bound(const Task& ti, uint r_id);
  ulong request_bound(const Task& ti, uint r_id);
  ulong formula_30(const Task& ti, uint p_id, ulong interval);
  ulong angent_exec_bound(const Task& ti, uint p_id, ulong interval);
  ulong NCS_workload(const Task& ti, ulong interval);  // Non-Critical-Section
  ulong CS_workload(const Task& ti, uint resource_id,
                    ulong interval);  // Critical-Section
  ulong response_time_AP(const Task& ti);
  ulong response_time_SP(const Task& ti);
  bool worst_fit_for_resources(uint active_processor_num);
  bool is_first_fit_for_tasks_schedulable_1(uint start_processor);
  bool is_first_fit_for_tasks_schedulable_2(uint start_processor);

  // DPCP
  ulong local_blocking(Task* ti);
  ulong remote_blocking(Task* ti);
  ulong total_blocking(Task* ti);
  ulong interference(const Task& ti, ulong interval);
  ulong response_time(Task* ti);
  ulong get_max_wait_time(const Task& ti, const Request& rq);
  void lp_dpcp_objective(const Task& ti, LinearProgram* lp, DPCPMapper* vars,
                         LinearExpression* local_obj,
                         LinearExpression* remote_obj);
  void lp_dpcp_add_constraints(const Task& ti, LinearProgram* lp,
                               DPCPMapper* vars);
  // Constraint 1 [BrandenBurg 2013 RTAS] Xd(x,q,v) + Xi(x,q,v) + Xp(x,q,v) <= 1
  void lp_dpcp_constraint_1(const Task& ti, LinearProgram* lp,
                            DPCPMapper* vars);
  // Constraint 2 [BrandenBurg 2013 RTAS] for any remote resource lq and task tx
  // except ti Xp(x,q,v) = 0
  void lp_dpcp_constraint_2(const Task& ti, LinearProgram* lp,
                            DPCPMapper* vars);
  // Constraint 3 [BrandenBurg 2013 RTAS]
  void lp_dpcp_constraint_3(const Task& ti, LinearProgram* lp,
                            DPCPMapper* vars);
  // Constraint 6 [BrandenBurg 2013 RTAS]
  void lp_dpcp_constraint_4(const Task& ti, LinearProgram* lp,
                            DPCPMapper* vars);
  // Constraint 7 [BrandenBurg 2013 RTAS]
  void lp_dpcp_constraint_5(const Task& ti, LinearProgram* lp,
                            DPCPMapper* vars);
  // Constraint 8 [BrandenBurg 2013 RTAS]
  void lp_dpcp_constraint_6(const Task& ti, LinearProgram* lp,
                            DPCPMapper* vars);

  // Others
  bool alloc_schedulable();
  bool alloc_schedulable(Task* ti);

 public:
  LP_RTA_PFP_ROP_DPCP_PLUS();
  LP_RTA_PFP_ROP_DPCP_PLUS(TaskSet tasks, ProcessorSet processors,
                      ResourceSet resources);
  ~LP_RTA_PFP_ROP_DPCP_PLUS();
  bool is_schedulable();
};

#endif  // INCLUDE_SCHEDTEST_PARTITIONED_LP_LP_RTA_PFP_ROP_DPCP_PLUS_H_
