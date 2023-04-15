// Copyright [2019] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <lp.h>
#include <math-helper.h>
#include <processors.h>
#include <resources.h>
#include <rta_dag_gedf_hoo.h>
#include <solution.h>
#include <tasks.h>
#include <iostream>
#include <sstream>
#include <exception>

using std::min;
using std::exception;
using std::ostringstream;

RTA_DAG_GEDF_HOO::RTA_DAG_GEDF_HOO()
    : GlobalSched(false, RTA, FIX_PRIORITY, FMLP, "", "") {}

RTA_DAG_GEDF_HOO::RTA_DAG_GEDF_HOO(DAG_TaskSet tasks, ProcessorSet processors,
                                   ResourceSet resources)
    : GlobalSched(false, RTA, FIX_PRIORITY, FMLP, "", "") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;

  this->resources.update(&(this->tasks));
  this->processors.update(&(this->tasks), &(this->resources));

  this->tasks.init();
  this->tasks.RM_Order();
  this->processors.init();
}

RTA_DAG_GEDF_HOO::~RTA_DAG_GEDF_HOO() {}

uint32_t RTA_DAG_GEDF_HOO::instance_number(const DAG_Task& ti,
                                            uint64_t interval) {
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  return ceiling(interval + r, p);
}

uint64_t RTA_DAG_GEDF_HOO::total_workload(const DAG_Task& ti,
                                           uint64_t interval) {
  uint32_t p_num = processors.get_processor_num();
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  uint64_t workload = ceiling(interval + r - e/p_num, p) * e;
  // cout << "    workload 1:" << workload << endl;
  return workload;
}

uint64_t RTA_DAG_GEDF_HOO::total_workload_2(const DAG_Task& ti,
                                           uint64_t interval) {
  // uint64_t cpl = ti.get_critical_path_length();
  uint32_t p_num = processors.get_processor_num();
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t d = ti.get_deadline();
  uint64_t r = ti.get_response_time();
  uint64_t workload = ceiling(interval + r - d, p) * e;
  // cout << "    workload 2:" << workload << endl;
  return workload;
}


uint64_t RTA_DAG_GEDF_HOO::intra_interference(const DAG_Task& ti) {
  return ti.get_wcet() - ti.get_critical_path_length();
}

uint64_t RTA_DAG_GEDF_HOO::inter_interference(const DAG_Task& ti,
                                               uint64_t interval, 
                                               vector<Slack> slacks) {
  uint64_t inter_interf = 0;
  uint64_t cil;
  try {
    foreach_task_except(tasks.get_tasks(), ti, tx) {
      cil = ti.get_deadline() - (ti.get_deadline()/tx->get_period())*tx->get_period();
      // cout << "inter_I from task " << tx->get_id() << ":" << endl;
      inter_interf += (ti.get_deadline()/tx->get_period())*tx->get_wcet() + CI_workload((*tx), cil, slacks[tx->get_id()].slack);
    }
  } catch (exception& e) {
    cout << e.what() << endl;
  }
  return inter_interf;
}

typedef struct CIW {
  uint32_t node_id;
  uint64_t s;
  uint64_t f;
  uint64_t interf_w;
}CIW;

uint64_t RTA_DAG_GEDF_HOO::CI_workload(const DAG_Task& tx, uint64_t interval, uint64_t slack) {
  uint64_t sum = 0;
  vector<struct CIW> ciw;
  uint64_t dx = tx.get_deadline();
  foreach(tx.get_vnodes(), vnode) {
    CIW tmp;
    tmp.node_id = vnode->job_id;
    tmp.s = dx - slack;
    tmp.f = dx - slack;
    tmp.interf_w = 0;
    ciw.push_back(tmp);
  }
  foreach(tx.get_vnodes(), vnode) {
    foreach(tx.get_vnodes(), pnode) {
      if (tx.is_connected((*vnode), (*pnode))) {
        if (ciw[pnode->job_id].s < ciw[vnode->job_id].f)
          ciw[vnode->job_id].f = ciw[pnode->job_id].s;
      }
    }
    ciw[vnode->job_id].s = ciw[vnode->job_id].f - vnode->wcet;
  }

  foreach(tx.get_vnodes(), vnode) {
    if (dx - interval <= ciw[vnode->job_id].s) {
      ciw[vnode->job_id].interf_w = vnode->wcet;
    } else if ((dx - interval > ciw[vnode->job_id].s) && (dx - interval < ciw[vnode->job_id].f)) {
      ciw[vnode->job_id].interf_w = ciw[vnode->job_id].f - (dx - interval);
    } else {
      ciw[vnode->job_id].interf_w = 0;
    }
  }

  foreach(ciw, node) {
    sum += node->interf_w;
  }
  return sum;
}

uint64_t RTA_DAG_GEDF_HOO::response_time(const DAG_Task& ti, uint64_t slack) {
  uint32_t t_id = ti.get_id();
  uint64_t CPL = ti.get_critical_path_length();
  uint64_t intra_interf = intra_interference(ti);
  uint p_num = processors.get_processor_num();

  uint64_t test_start = CPL + intra_interf / p_num;

  vector<Slack> slacks;
  foreach(tasks.get_tasks(), task) {
    Slack tmp;
    tmp.t_id = task->get_id();
    tmp.slack = 0;
    slacks.push_back(tmp);
  }

  uint64_t response = test_start;
  uint64_t demand = 0;

  uint64_t inter_interf = inter_interference(ti, ti.get_deadline(), slacks);
  demand = test_start + inter_interf / p_num;

  return demand;
}


bool RTA_DAG_GEDF_HOO::is_schedulable() {
  uint64_t response_bound;

  foreach(tasks.get_tasks(), ti) {
    if (ti->get_critical_path_length() > ti->get_deadline())
      return false;
    // cout << "Task:" << ti->get_id() << " Priority:" << ti->get_priority() <<
    // " Deadline:" << ti->get_deadline() << endl;
    response_bound = response_time((*ti));
    // cout << "response time:" << response_bound << endl;
    if (response_bound > ti->get_deadline()) {
      // tasks.display();
      return false;
    }
    ti->set_response_time(response_bound);
  }
  // tasks.display();
  return true;
}

//////////////with slack-based iterative scheduling test///////////////////

RTA_DAG_GEDF_HOO_v2::RTA_DAG_GEDF_HOO_v2() {}
RTA_DAG_GEDF_HOO_v2::RTA_DAG_GEDF_HOO_v2(DAG_TaskSet tasks, ProcessorSet processors,
                   ResourceSet resources): RTA_DAG_GEDF_HOO(tasks, processors, resources) {

                   }
RTA_DAG_GEDF_HOO_v2::~RTA_DAG_GEDF_HOO_v2() {}


bool RTA_DAG_GEDF_HOO_v2::is_schedulable() {
  bool updated = true;
  bool feasible;
  uint p_num = processors.get_processor_num();
  uint32_t Nround = 0;
  vector<Slack> slacks;
  foreach(tasks.get_tasks(), task) {
    Slack tmp;
    tmp.t_id = task->get_id();
    tmp.slack = 0;
    slacks.push_back(tmp);
  }

  do {
    feasible = true;
    updated = false;
    foreach(tasks.get_tasks(), task) {
      uint64_t intra_I = intra_interference((*task));
      uint64_t inter_I = inter_interference((*task), task->get_deadline(), slacks);
      int64_t newBound = task->get_deadline() - task->get_critical_path_length() - ceiling(inter_I + intra_I, p_num);
      if (0 > newBound)
        feasible = false;
      else {
        if (slacks[task->get_id()].slack < newBound) {
          slacks[task->get_id()].slack = newBound;
          updated = true;
        }
      }
    }
    Nround++;
    if (feasible)
      return true;
  } while(updated && Nround < 2);

  return false;
}
