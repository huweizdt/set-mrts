// Copyright [2018] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <math-helper.h>
#include <math.h>
#include <rta_pfp_dag.h>

using std::min;

RTA_PFP_DAG::RTA_PFP_DAG()
    : PartitionedSched(false, RTA, FIX_PRIORITY, NONE, "",
                       "DAG") {}

RTA_PFP_DAG::RTA_PFP_DAG(TaskSet tasks, ProcessorSet processors,
                         ResourceSet resources)
    : PartitionedSched(false, RTA, FIX_PRIORITY, NONE, "",
                       "DAG") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;

  this->resources.update(&(this->tasks));
  this->processors.update(&(this->tasks), &(this->resources));

  this->processors.init();
}

RTA_PFP_DAG::~RTA_PFP_DAG() {}


bool RTA_PFP_DAG::is_schedulable() {

}

ulong RTA_PFP_DAG::interf_intra(DAG_Task &ti, uint n_id) {
  VNode v = ti.get_vnode_by_id(n_id);
  foreach(ti.get_vnodes(), u) {
    
  }
}

ulong RTA_PFP_DAG::interf_inter(DAG_Task &ti, uint n_id) {

}

ulong RTA_PFP_DAG::response_time_node(DAG_Task &ti, uint n_id) {

}

ulong RTA_PFP_DAG::response_time_task(DAG_Task &ti) {

}
