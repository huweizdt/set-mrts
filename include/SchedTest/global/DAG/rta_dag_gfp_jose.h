// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#ifndef INCLUDE_SCHEDTEST_GLOBAL_DAG_RTA_DAG_GFP_JOSE_H_
#define INCLUDE_SCHEDTEST_GLOBAL_DAG_RTA_DAG_GFP_JOSE_H_

/*
**
**
**
*/

#include <digraph.h>
#include <g_sched.h>
#include <processors.h>
#include <resources.h>
#include <tasks.h>
#include <set>
#include <vector>

#define PAR 0
#define SER 1
#define EXT 2

typedef set<uint64_t> F_Times;

typedef struct workload_block {
  int64_t width;
  int32_t height;
} work_block;

typedef vector<work_block> workload_dist;

typedef struct workload_CIO {
  uint task_id;
  workload_dist WD_CI;
  workload_dist WD_CO;
} workload_CIO;

typedef struct ld_edge {
  uint id;
  uint type;
  uint tail;
  uint head;
} ld_edge;

typedef struct ld_vertex {
  uint id;
  vector<ld_edge> predecessors;
  vector<ld_edge> successors;
} ld_vertex;

typedef struct {
  vector<ld_vertex> vertices;
  vector<ld_edge> edges;
} LDigraph;

typedef struct dt_node{  // node of a decomposition tree
  int64_t node_id;
  int type; // 0: parallel composition 1: series composition 2: external node (vertex)
  int64_t job_id;
  int64_t wcet;
  int64_t left;
  int64_t right;
} dt_node;

typedef struct DTree {
  uint root;
  vector<dt_node> dt_nodes;
} DTree;

class RTA_DAG_GFP_JOSE : public GlobalSched {
 protected:
  DAG_TaskSet tasks;
  ProcessorSet processors;
  ResourceSet resources;

  vector<workload_CIO> workload_CIOs;


  // improved carry-in workload
  workload_dist get_WD_CI(const DAG_Task& ti); // get workload distribution
  uint32_t AS(const DAG_Task& ti, uint64_t t);
  uint64_t get_CI(const DAG_Task& ti, workload_dist WD, int64_t interval);

  // improved carry-out workload
  workload_dist get_WD_CO(const DAG_Task& ti);
  DAG_Task get_NFJ_task(const DAG_Task& ti);
  LDigraph get_LDigraph(const DAG_Task& NFJ_task);
  DTree get_decomposition_tree(const DAG_Task& ti, LDigraph ldg);
  DTree get_decomposition_tree(const DAG_Task& ti);  // from NFJ_tasks
  bool is_parallel_composition(const DAG_Task& ti, DTree dtree, dt_node T1, dt_node T2);
  vector<int64_t> par(DTree *dtree, int64_t node_id);
  void dt_delete(DTree *dtree, uint node_id);
  uint64_t get_CO(const DAG_Task& ti, workload_dist WD, int64_t interval);

  uint64_t get_WC(const DAG_Task& ti, int64_t interval);

  uint64_t intra_interference(const DAG_Task& ti);

  uint64_t inter_interference(const DAG_Task& ti, int64_t interval);

  uint64_t response_time(const DAG_Task& ti);

 public:
  RTA_DAG_GFP_JOSE();
  RTA_DAG_GFP_JOSE(DAG_TaskSet tasks, ProcessorSet processors,
                   ResourceSet resources);
  ~RTA_DAG_GFP_JOSE();
  bool is_schedulable();
};

// Non-LP approach for FMLP
class RTA_DAG_GFP_JOSE_FMLP : public RTA_DAG_GFP_JOSE {
 protected:
  uint32_t instance_number(const DAG_Task& ti, int64_t interval);
  uint64_t intra_blocking(const DAG_Task& ti, uint32_t resource_id,
                          uint32_t request_num);
  uint64_t inter_blocking(const DAG_Task& ti, uint32_t resource_id,
                          uint32_t request_num);
  uint64_t CS_workload(const DAG_Task& ti, uint32_t resource_id,
                       uint64_t interval);  // Critical-Section
  uint64_t inter_interference(const DAG_Task& ti, int64_t interval);
  uint64_t response_time(const DAG_Task& ti);
 public:
  RTA_DAG_GFP_JOSE_FMLP();
  RTA_DAG_GFP_JOSE_FMLP(DAG_TaskSet tasks, ProcessorSet processors,
                   ResourceSet resources);
  ~RTA_DAG_GFP_JOSE_FMLP();
  bool is_schedulable();
};

// Non-LP approach for PIP
class RTA_DAG_GFP_JOSE_PIP : public RTA_DAG_GFP_JOSE {
 protected:
  uint32_t instance_number(const DAG_Task& ti, int64_t interval);
  uint64_t intra_blocking(const DAG_Task& ti, uint32_t resource_id,
                          uint32_t request_num);
  uint64_t inter_blocking(const DAG_Task& ti, uint32_t resource_id,
                          uint32_t request_num);
  uint64_t CS_workload(const DAG_Task& ti, uint32_t resource_id,
                       uint64_t interval);  // Critical-Section
  uint64_t inter_interference(const DAG_Task& ti, int64_t interval);
  uint64_t response_time(const DAG_Task& ti);
 public:
  RTA_DAG_GFP_JOSE_PIP();
  RTA_DAG_GFP_JOSE_PIP(DAG_TaskSet tasks, ProcessorSet processors,
                   ResourceSet resources);
  ~RTA_DAG_GFP_JOSE_PIP();
  bool is_schedulable();
};

void display_dt_node(dt_node dtn);
void display_dtree(DTree dtree);
void display_LDigraph(LDigraph ldgraph);
ld_vertex* get_ld_vertex(LDigraph *ldgraph, uint v_id);
set<uint> get_predecessors(ld_vertex vertex);
set<uint> get_successors(ld_vertex vertex);
void delete_ld_edge(LDigraph *ldgraph, uint e_id);

#endif  // INCLUDE_SCHEDTEST_GLOBAL_DAG_RTA_DAG_GFP_JOSE_H_
