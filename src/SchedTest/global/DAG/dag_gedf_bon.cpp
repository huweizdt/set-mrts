// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <dag_gedf_bon.h>
#include <iteration-helper.h>
#include <math-helper.h>

DAG_GEDF_BON() : GlobalSched(false, SB, EDF, NONE, "", "DAG") { e = 0.1; }

DAG_GEDF_BON(DAG_TaskSet tasks, ProcessorSet processors, ResourceSet resources,
             double e = 0.1)
    : GlobalSched(false, SB, EDF, NONE, "", "DAG") {
  this->tasks = tasks;
  this->processors = processors;
  this->resources = resources;
  // this->tasks.RM_Order();
}

set<Work> DAG_GEDF_BON::precise_workload(const DAG_Task& dag_task, ulong T) {
  set<Work> workload_set_total;
  workload_set_total.clear();
  ulong deadline = dag_task.get_deadline();
  ulong period = dag_task.get_period();
  ulong vol = dag_task.get_vol();
  if (T < deadline) return workload_set_total;
  ulong current_t = T;
  vector<ulong> f_times;
  vector<Exec_Range> jobs;

  for (uint i = 0; i < dag_task.get_vnode_num(); i++) {
    Exec_Range er;
    er.r_t = 0;
    er.f_t = 0;
    jobs.push_back(er);
  }

  for (uint i = 0; i < dag_task.get_vnode_num(); i++) {
    VNode& vnode = dag_task.get_vnode_by_id(i);
    ulong latest_f_time = 0;
    for (uint j = 0; j < vnode.precedences.size(); j++) {
      uint prenode = vnode.precedences[j]->tail;

      if (latest_f_time < jobs[prenode].f_t) latest_f_time = jobs[prenode].f_t;
    }
    jobs[i].r_t = latest_f_time;
    jobs[i].f_t = latest_f_time + vnode.wcet;
  }

  set<ulong> time_set;
  time_set.insert(0);
  for (uint i = 0; i < jobs.size(); i++) {
    time_set.insert(jobs[i].f_t);
  }
  time_set.insert(deadline);

  // sort(time_set.begin(), time_set.end());
  set<Work> workload_set;
  Work work;
  work.time = 0;
  work.workload = 0;
  workload_set.insert(work);

  for (uint i = 1; i < time_set.size(); i++) {
    ulong t = set_member<ulong>(time_set, i);
    ulong exec_t = t - set_member<ulong>(time_set, i - 1);
    ulong workload = 0;
    for (uint j = 0; j < jobs.size(); j++) {
      if ((jobs[j].r_t < t) && (jobs[j].f_t >= t)) {
        workload += exec_t;
      }
    }
    work.time = t;
    work.workload = workload;
    // work.workload = workload + set_member(workload_set, workload_set.size() -
    // 1).workload;
    workload_set.insert(work);
  }

  uint num = T / period;
  uint l = T % period;
  ulong release;

  work.time = 0;
  work.workload = 0;
  workload_set_total.insert(work);
  if ((l >= deadline) || (0 == l)) {
    if (l != 0) {
      num++;
      release = l - deadline;
    } else {
      release = period - deadline;
    }

    for (uint i = 0; i < num; i++) {
      for (uint j = 1; j < workload_set.size(); j++) {
        work.time =
            set_member<Work>(workload_set, j).time + release + i * period;
        work.workload = set_member<Work>(workload_set, j).workload;
        // work.workload = set_member<Work>(workload_set, j).workload + i * vol;
        workload_set_total.insert(work);
      }
    }
  } else {
    ulong cutting = deadline - l;
    release = period - deadline + l;
    uint begin;
    ulong cutting_workload;
    ulong cutting_time;
    for (uint i = 0; i < workload_set.size() - 1; i++) {
      if ((set_member<Work>(workload_set, i).time <= cutting) &&
          (set_member<Work>(workload_set, i + 1).time > cutting)) {
        begin = i + 1;
        cutting_workload;
        cutting_time = cutting - set_member<Work>(workload_set, i).time;
        cutting_workload =
            (cutting_time * (set_member<Work>(workload_set, i + 1).workload)) /
            (set_member<Work>(workload_set, i + 1).time -
             set_member<Work>(workload_set, i).time);
        break;
      }
    }

    work.time = set_member<Work>(workload_set, begin).time - cutting;
    work.workload =
        set_member<Work>(workload_set, begin).workload - cutting_workload;
    workload_set_total.insert(work);
    begin++;

    for (uint i = begin; i < workload_set.size(); i++) {
      work.time = set_member<Work>(workload_set, i).time - cutting;
      work.workload = set_member<Work>(workload_set, i).workload;
      workload_set_total.insert(work);
    }

    ulong carry_in_workload =
        set_member<Work>(workload_set_total, workload_set_total.size() - 1)
            .workload;

    for (uint i = 0; i < num; i++) {
      for (uint j = 0; j < workload_set.size(); j++) {
        work.time =
            set_member<Work>(workload_set, j).time + release + i * period;
        work.workload = set_member<Work>(workload_set, j).workload;
        // work.workload = set_member<Work>(workload_set, j).workload +
        // carry_in_workload + i * vol;
        workload_set_total.insert(work);
      }
    }
  }

  return workload_set_total;
}

ulong DAG_GEDF_BON::approximate_workload(const DAG_Task& dag_task, ulong T) {
  return (T - dag_task.get_deadline()) * dag_task.get_vol() /
         dag_task.get_period();
}

double DAG_GEDF_BON::approximation() {
  vector<set<Work>> piecewises;
  vector<double> last_slopes;
  ulong splitted_time;
  double last_slope = 0;
  double max_slope = 0;

  for (uint i = 0; i < tasks.get_taskset_size(); i++) {
    DAG_Task& dag_task_i = tasks.get_task_by_id(i);
    ulong Ti = dag_task_i.get_period();
    ulong Di = dag_task_i.get_deadline();
    double slope_i = dag_task_i.get_vol();
    slope_i /= Ti;
    splitted_time = Ti / e + (1 + 1 / e) * Di;
    piecewises.push_back(precise_workload(dag_task_i, splitted_time));
    last_slopes.push_back(slope_i);
  }
  for (uint i = 0; i < last_slopes.size(); i++) last_slope += last_slopes[i];
  set<Work> workload_sum;
  Work work;
  work.time = 0;
  work.workload = 0;
  workload_sum.insert(work);
  ulong latest_time = 0;
  ulong current_time = MAX_LONG;
  ulong current_workload = 0;
  bool empty;
  for (uint i = 0; i < piecewises.size(); i++) {  // find latest time
    if (piecewises[i].size() > 0) {
      Work temp = set_member(piecewises[i], piecewises[i].size() - 1);
      if (temp.time > latest_time) {
        latest_time = temp.time;
      }
    }
  }
  for (uint i = 0; i < piecewises.size(); i++) {
    if (piecewises[i].size() > 0) {
      Work temp = set_member(piecewises[i], piecewises[i].size() - 1);
      if (temp.time < latest_time) {
        work.time = latest_time;
        work.time = temp.workload + (latest_time - temp.time) * last_slopes[i];
        piecewises[i].insert(work);
      }
    }
  }

  for (uint i = 0; i < piecewises.size(); i++) {
    for (uint j = 0; j < piecewises[i].size(); j++) {
      Work temp = set_member(piecewises[i], j);
    }
  }
  do {
    empty = true;
    current_time = MAX_LONG;
    for (uint i = 0; i < piecewises.size(); i++) {  // find earliest time
      if (piecewises[i].size() > 0) {
        empty = false;
        Work temp = set_member(piecewises[i], 0);
        if (temp.time < current_time) {
          current_time = temp.time;
        }
      }
    }

    for (uint i = 0; i < piecewises.size(); i++) {  // merge same instant
      if (piecewises[i].size() > 0) {
        empty = false;
        Work temp = set_member(piecewises[i], 0);
        if (temp.time == current_time) {
          current_workload += temp.workload;
          piecewises[i].erase(temp);
        }
      }
    }
    work.time = current_time;
    work.workload = current_workload;
    if (!empty) workload_sum.insert(work);
  } while (!empty);

  for (uint i = 1; i < workload_sum.size(); i++) {
    Work w = set_member(workload_sum, i);
    double s = w.workload;
    s /= w.time;
    if (s > max_slope) max_slope = s;
  }
  if (last_slope > max_slope) max_slope = last_slope;

  return max_slope;
}

bool DAG_GEDF_BON::is_schedulable() {
  double max_slope = approximation();
  double speed_up = e + 2 - 1.0 / m;
  if (max_slope * speed_up < m)
    return true;
  else
    return false;
}
