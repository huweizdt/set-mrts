// Copyright [2020] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_PARTITIONED_LP_LP_RTA_PFP_GPU_H_
#define INCLUDE_SCHEDTEST_PARTITIONED_LP_LP_RTA_PFP_GPU_H_

/*
**
**
** 
*/

#include <p_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>
#include <varmapper.h>
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
|(63-34)Reserved |(31-30) Blocking type|(29-20) Task          |(19-10) GPU_id        |(9-0) GPU_request     |
|________________|_____________________|______________________|______________________|______________________|
*/
class GPUMapper : public VarMapperBase {
 public:
  enum var_type {
    DELAY_DIRECT,      // 0x000
    DELAY_OTHER,       // 0x001
  };

 protected:
  static uint64_t encode_request(uint64_t task_id, uint64_t gpu_id, uint64_t grequest_id, uint64_t type);
  static uint64_t get_type(uint64_t var);
  static uint64_t get_task(uint64_t var);
  static uint64_t get_gpu_id(uint64_t var);
  static uint64_t get_grequest_id(uint64_t var);

 public:
  explicit GPUMapper(uint start_var = 0);
  uint lookup(uint task_id, uint gpu_id, uint grequest_id, var_type type);
  string key2str(uint64_t key, uint var) const;
};

class LP_RTA_PFP_GPU : public PartitionedSched {
 protected:
  TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  uint64_t gpu_response_time(Task* ti);
  uint64_t interference(const Task& ti, uint64_t interval);
  uint64_t response_time(Task* ti);
  bool alloc_schedulable();
  uint64_t get_max_wait_time(const Task& ti, const Request& rq);

 public:
  LP_RTA_PFP_GPU();
  LP_RTA_PFP_GPU(TaskSet tasks, ProcessorSet processors,
                  ResourceSet resources);
  ~LP_RTA_PFP_GPU();
  bool is_schedulable();
};

typedef struct {
  uint t_id;
  uint gthreads;  // G_Threads per block
} G_Threads;

int gthread_decrease(G_Threads t1, G_Threads t2);

class LP_RTA_PFP_GPU_USS : public LP_RTA_PFP_GPU {
 private:
  vector<vector<uint32_t>> streams;
 protected:
  vector<vector<uint32_t>> stream_partition();
  uint64_t get_gpurt(Task* ti, uint64_t interval);
  uint64_t gpu_response_time(Task* ti);
  uint64_t interference(const Task& ti, uint64_t interval);
  uint64_t response_time(Task* ti);
  bool alloc_schedulable();

  void objective(const Task& ti, LinearProgram* lp, GPUMapper* vars, LinearExpression* obj, uint64_t interval);

  void add_constraints(const Task& ti, LinearProgram* lp, GPUMapper* vars, uint64_t interval);

  void constraint_1(const Task& ti, LinearProgram* lp, GPUMapper* vars, uint64_t interval);

  void constraint_2(const Task& ti, LinearProgram* lp, GPUMapper* vars, uint64_t interval);

  void constraint_3(const Task& ti, LinearProgram* lp, GPUMapper* vars, uint64_t interval);
                          
 public:
  LP_RTA_PFP_GPU_USS();
  LP_RTA_PFP_GPU_USS(TaskSet tasks, ProcessorSet processors,
                  ResourceSet resources);
  ~LP_RTA_PFP_GPU_USS();
  bool is_schedulable();
};



class LP_RTA_PFP_GPU_USS_v2 : public LP_RTA_PFP_GPU_USS {
 protected:
  uint threshold;
  vector<vector<uint32_t>> streams;
  uint64_t get_gpurt(Task* ti);
  uint64_t gpu_response_time(Task* ti);
  uint64_t response_time(Task* ti);
  bool alloc_schedulable();
                          
 public:
  LP_RTA_PFP_GPU_USS_v2();
  LP_RTA_PFP_GPU_USS_v2(TaskSet tasks, ProcessorSet processors,
                  ResourceSet resources);
  ~LP_RTA_PFP_GPU_USS_v2();
  bool is_schedulable();
};


class LP_RTA_PFP_GPU_prio : public LP_RTA_PFP_GPU_USS_v2 {
 protected:
  uint64_t get_gpurt(Task* ti);
  uint64_t gpu_response_time(Task* ti);
  uint64_t response_time(Task* ti);
                          
 public:
  LP_RTA_PFP_GPU_prio();
  LP_RTA_PFP_GPU_prio(TaskSet tasks, ProcessorSet processors,
                  ResourceSet resources);
  ~LP_RTA_PFP_GPU_prio();
  bool is_schedulable();
};





#endif  // INCLUDE_SCHEDTEST_PARTITIONED_LP_LP_RTA_PFP_GPU_H_
