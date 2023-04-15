// Copyright [2020] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <assert.h>
#include <iteration-helper.h>
#include <lp.h>
#include <math-helper.h>
#include <processors.h>
#include <resources.h>
#include <rta_dag_gfp_jose.h>
#include <solution.h>
#include <tasks.h>
#include <iostream>
#include <sstream>
#include <exception>


using std::max;
using std::min;
using std::exception;
using std::ostringstream;

RTA_DAG_GFP_JOSE::RTA_DAG_GFP_JOSE()
    : GlobalSched(false, RTA, FIX_PRIORITY, FMLP, "", "") {}

RTA_DAG_GFP_JOSE::RTA_DAG_GFP_JOSE(DAG_TaskSet tasks, ProcessorSet processors,
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
  foreach(this->tasks.get_tasks(), ti) {
    ti->calculate_RCT();
    workload_dist WD_CI = get_WD_CI((*ti));
    workload_dist WD_CO = get_WD_CO((*ti));
    workload_CIO temp = {
      .task_id = ti->get_id(),
      .WD_CI = WD_CI,
      .WD_CO = WD_CO,
    };
    workload_CIOs.push_back(temp);
    // ti->display();
    // ti->update_parallel_degree();
    // cout << "parallel degree by dilworth's theorem:" << ti->get_parallel_degree() << endl;
    // cout << "parallel degree of the NFJ-DAG:" << temp.WD_CO[0].height << endl;
    // if (ti->get_parallel_degree() != temp.WD_CO[0].height)
    //   exit(0);
  }
}

RTA_DAG_GFP_JOSE::~RTA_DAG_GFP_JOSE() {}

workload_dist RTA_DAG_GFP_JOSE::get_WD_CI(const DAG_Task& ti) {
  uint64_t f_time;
  F_Times f_times;
  vector<uint64_t> f_times_v;
  f_times.insert(0);
  uint i;
  workload_dist WD;
  foreach(ti.get_vnodes(), vnode) {
    f_time = ti.get_RCT(vnode->job_id);
    f_times.insert(f_time);
  }
  for (std::set<uint64_t>::iterator it=f_times.begin(); it!=f_times.end(); ++it) {
    f_times_v.push_back((*it));
  }
  for (i = 0; i < f_times.size() - 1; i++) {
    work_block wb;
    wb.width = f_times_v[i+1] - f_times_v[i];
    wb.height = AS(ti, f_times_v[i]);
    WD.push_back(wb);
  }
  return WD;
}

uint32_t RTA_DAG_GFP_JOSE::AS(const DAG_Task& ti, uint64_t t) {
  uint32_t parallel_degree = 0;
  uint64_t rct, wcet;
  foreach(ti.get_vnodes(), vnode) {
    rct = ti.get_RCT(vnode->job_id);
    wcet = vnode->wcet;
    if (t >= (rct - wcet) && t < rct)
      parallel_degree++;
  }
  return parallel_degree;
}


uint64_t RTA_DAG_GFP_JOSE::get_CI(const DAG_Task& ti, workload_dist WD, int64_t interval) {
  int64_t sum = 0;
  int64_t temp, subsequence;
  int64_t T_i = ti.get_period();
  int64_t R_i = ti.get_response_time();
  uint i, j;
  for (i = 0; i < WD.size(); i++) {
    subsequence = 0;
    for (j = i + 1; j < WD.size(); j++) {
      subsequence += WD[j].width;
    }
    temp = max(min(interval - T_i + R_i - subsequence, WD[i].width), (int64_t)0);
    sum += WD[i].height * temp;
  }
  return min(sum, max((int64_t)0, interval - T_i + R_i) * processors.get_processor_num());
}


workload_dist RTA_DAG_GFP_JOSE::get_WD_CO(const DAG_Task& ti) {
  DAG_Task NFJ_task = get_NFJ_task(ti);
  LDigraph ldg = get_LDigraph(NFJ_task);
  DTree dtree = get_decomposition_tree(ti, ldg);
  workload_dist WD;
  vector<int64_t> P;
  uint64_t min;
  uint64_t height, width;
  work_block wb;
  dt_node node;

  while (0 < dtree.dt_nodes.size()) {
    min = 0xffffffff;
    P = par(&dtree, dtree.root);

    foreach(P, p) {
      node = dtree.dt_nodes[(*p)];
      VNode vnode = ti.get_vnode_by_id(node.job_id);

    if (0 > node.wcet)
      exit(0);

    if (node.wcet < min)
        min = node.wcet;
    }
    width = min;
    wb.width = width;
    wb.height = P.size();
    WD.push_back(wb);
    foreach(P, p) {
      dtree.dt_nodes[(*p)].wcet -= width;
      if (0 == dtree.dt_nodes[(*p)].wcet) {
        dt_delete(&dtree, (*p));
        foreach(P, temp) {
          if ((*temp) > (*p))
            (*temp)--;
        }
      }
    }
  }
  return WD;
}

DAG_Task RTA_DAG_GFP_JOSE::get_NFJ_task(const DAG_Task& ti) {
  VNode tail, head;
  DAG_Task NFJ_task = ti;
  NFJ_task.unFloyd();


  int i;
  NFJ_task.refresh_relationship();
  bool deleted;

  do {
    deleted = false;
    foreach(NFJ_task.get_arcnodes(), arcnode) {
      tail = NFJ_task.get_vnode_by_id(arcnode->tail);
      head = NFJ_task.get_vnode_by_id(arcnode->head);
      if (1 < tail.follow_ups.size() && 1 < head.precedences.size()) { // source -> sink
        NFJ_task.delete_arc(arcnode->tail, arcnode->head);
        deleted = true;
        break;
      }
    }
  } while (deleted);
  
  return NFJ_task;
}


LDigraph RTA_DAG_GFP_JOSE::get_LDigraph(const DAG_Task& NFJ_task) {
  LDigraph ldg;

  foreach(NFJ_task.get_vnodes(), node) {
    ld_edge edge;
    edge.id = node->job_id;
    edge.type = EXT;
    edge.tail = MAX_INT;
    edge.head = MAX_INT;
    ldg.edges.push_back(edge);
  }

  ld_vertex source;
  source.id = 0;
  ldg.vertices.push_back(source);

  for (uint i = 0; i < NFJ_task.get_vnodes().size(); i++) {
    for (uint j = 0; j < NFJ_task.get_vnodes().size(); j++) {
      if (NFJ_task.is_arc_exist(i, j)) {
        if (MAX_INT != ldg.edges[i].head) {
          ldg.edges[j].tail = ldg.edges[i].head;
        }
        else if (MAX_INT != ldg.edges[j].tail) {
          ldg.edges[i].head = ldg.edges[j].tail;
        }
        else {
          ld_vertex vertex;
          vertex.id = ldg.vertices.size();
          ldg.edges[i].head = vertex.id;
          ldg.edges[j].tail = vertex.id;
          ldg.vertices.push_back(vertex);
        }
      }
    }
  }

  ld_vertex sink;
  sink.id = ldg.vertices.size();
  for (uint i = 0; i < ldg.edges.size(); i++) {
    if (MAX_INT == ldg.edges[i].tail) {
      ldg.edges[i].tail = 0;
    }
    if (MAX_INT == ldg.edges[i].head) {
      ldg.edges[i].head = sink.id;
    }
  }
  ldg.vertices.push_back(sink);

  foreach(ldg.vertices, vertex) {
    foreach(ldg.edges, edge) {
      if (vertex->id == edge->head) {
        vertex->predecessors.push_back((*edge));
      } else if (vertex->id == edge->tail) {
        vertex->successors.push_back((*edge));
      }
    }
  }

  return ldg;
}

DTree RTA_DAG_GFP_JOSE::get_decomposition_tree(const DAG_Task& ti, LDigraph ldg) {
  DTree dtree;
  vector<uint> unsatisfied;
  dt_node T1, T2, temp;

  for (uint i = 1; i < ldg.vertices.size() - 1; i++) {
    unsatisfied.push_back(i);
  }


  foreach(ldg.edges, edge) {
    dt_node temp = {
      .node_id = (int64_t)dtree.dt_nodes.size(),
      .type = EXT,
      .job_id = edge->id,
      .wcet = (int64_t)ti.get_vnode_by_id(edge->id).wcet,
      .left = -1,
      .right = -1,
    };
    dtree.dt_nodes.push_back(temp);
  }

  while (0 < unsatisfied.size()) {
    uint curr_vid = unsatisfied.front();
    unsatisfied.erase(unsatisfied.begin());


    ld_vertex *curr_vertex = get_ld_vertex(&ldg, curr_vid);

    while (1 < curr_vertex->predecessors.size() && 2 > get_predecessors(*curr_vertex).size()) { // one predecessor with multiple edges
      ld_edge e1, e2, p;
      set<uint> predecessors = get_predecessors(*curr_vertex);

      e1 = curr_vertex->predecessors[0];
      delete_ld_edge(&ldg, e1.id);
      e2 = curr_vertex->predecessors[0];
      delete_ld_edge(&ldg, e2.id);

      p.id = dtree.dt_nodes.size();
      p.type = PAR;
      p.head = curr_vid;
      p.tail = (get_ld_vertex(&ldg, e1.tail)->successors.size() < get_ld_vertex(&ldg, e2.tail)->successors.size()) ? e1.tail : e2.tail;
      ldg.edges.push_back(p);
      get_ld_vertex(&ldg, p.tail)->successors.push_back(p);
      get_ld_vertex(&ldg, p.head)->predecessors.push_back(p);
      dt_node temp = {
        .node_id = (int64_t)dtree.dt_nodes.size(),
        .type = PAR,
        .job_id = MAX_INT,
        .wcet = 0,
        .left = e1.id,
        .right = e2.id,
      };
      dtree.dt_nodes.push_back(temp);
    }

    while (1 < curr_vertex->successors.size() && 2 > get_successors(*curr_vertex).size()) { // one successor with multiple edges
      ld_edge e1, e2, p;
      set<uint> successors = get_successors(*curr_vertex);

      e1 = curr_vertex->successors[0];
      delete_ld_edge(&ldg, e1.id);
      e2 = curr_vertex->successors[0];
      delete_ld_edge(&ldg, e2.id);

      p.id = dtree.dt_nodes.size();
      p.type = PAR;
      p.head = (get_ld_vertex(&ldg, e1.head)->predecessors.size() < get_ld_vertex(&ldg, e2.head)->predecessors.size()) ? e1.head : e2.head;
      p.tail = curr_vid;
      ldg.edges.push_back(p);
      get_ld_vertex(&ldg, p.tail)->successors.push_back(p);
      get_ld_vertex(&ldg, p.head)->predecessors.push_back(p);
      dt_node temp = {
        .node_id = (int64_t)dtree.dt_nodes.size(),
        .type = PAR,
        .job_id = MAX_INT,
        .wcet = 0,
        .left = e1.id,
        .right = e2.id,
      };
      dtree.dt_nodes.push_back(temp);
    }

    if (1 == curr_vertex->predecessors.size() && 1 == curr_vertex->successors.size()) {
      ld_edge e1, e2, s;
      ld_vertex *predecessor = get_ld_vertex(&ldg, curr_vertex->predecessors[0].tail);
      ld_vertex *successor = get_ld_vertex(&ldg, curr_vertex->successors[0].head);

      e1 = curr_vertex->predecessors[0];
      delete_ld_edge(&ldg, e1.id);
      e2 = curr_vertex->successors[0];
      delete_ld_edge(&ldg, e2.id);

      s.id = dtree.dt_nodes.size();
      s.type = SER;
      s.head = successor->id;
      s.tail = predecessor->id;
      ldg.edges.push_back(s);
      predecessor->successors.push_back(s);
      successor->predecessors.push_back(s);
      dt_node temp = {
        .node_id = (int64_t)dtree.dt_nodes.size(),
        .type = SER,
        .job_id = MAX_INT,
        .wcet = 0,
        .left = e1.id,
        .right = e2.id,
      };
      dtree.dt_nodes.push_back(temp);

      bool p_unsatisfied = false, s_unsatisfied = false;
      foreach(unsatisfied, vertex) {
        if ((*vertex) == predecessor->id) {
          p_unsatisfied = true;
        }
        if ((*vertex) == successor->id) {
          s_unsatisfied = true;
        }
      }
      
      if (!p_unsatisfied && 0 != predecessor->predecessors.size()) {
        // add predecessor to unsatisfied set
        unsatisfied.push_back(predecessor->id);
      }
      
      if (!s_unsatisfied && 0 != successor->successors.size()) {
        // add successor to unsatisfied set
        unsatisfied.push_back(successor->id);
      }
    }
  }

  for (uint i = 1; i < ldg.vertices.size() - 1; i++) {
    if (0 != ldg.vertices[i].predecessors.size() || 0 != ldg.vertices[i].successors.size()) {
      unsatisfied.push_back(i);
    }
  }

  if (0 != unsatisfied.size()) {
    while (0 < unsatisfied.size()) {

      uint curr_vid = unsatisfied.front();
      unsatisfied.erase(unsatisfied.begin());


      ld_vertex *curr_vertex = get_ld_vertex(&ldg, curr_vid);

      while (1 < curr_vertex->predecessors.size()) {  
        ld_edge e1, e2, p;
        set<uint> predecessors = get_predecessors(*curr_vertex);

        e1 = curr_vertex->predecessors[0];
        delete_ld_edge(&ldg, e1.id);
        e2 = curr_vertex->predecessors[0];
        delete_ld_edge(&ldg, e2.id);

        p.id = dtree.dt_nodes.size();
        p.type = PAR;
        p.head = curr_vid;
        p.tail = (get_ld_vertex(&ldg, e1.tail)->successors.size() < get_ld_vertex(&ldg, e2.tail)->successors.size()) ? e1.tail : e2.tail;
        ldg.edges.push_back(p);
        get_ld_vertex(&ldg, p.tail)->successors.push_back(p);
        get_ld_vertex(&ldg, p.head)->predecessors.push_back(p);
        dt_node temp = {
          .node_id = (int64_t)dtree.dt_nodes.size(),
          .type = PAR,
          .job_id = MAX_INT,
          .wcet = 0,
          .left = e1.id,
          .right = e2.id,
        };
        dtree.dt_nodes.push_back(temp);
      }

      while (1 < curr_vertex->successors.size()) {  
        ld_edge e1, e2, p;
        set<uint> successors = get_successors(*curr_vertex);

        e1 = curr_vertex->successors[0];
        delete_ld_edge(&ldg, e1.id);
        e2 = curr_vertex->successors[0];
        delete_ld_edge(&ldg, e2.id);

        p.id = dtree.dt_nodes.size();
        p.type = PAR;
        p.head = (get_ld_vertex(&ldg, e1.head)->predecessors.size() < get_ld_vertex(&ldg, e2.head)->predecessors.size()) ? e1.head : e2.head;
        p.tail = curr_vid;
        ldg.edges.push_back(p);
        get_ld_vertex(&ldg, p.tail)->successors.push_back(p);
        get_ld_vertex(&ldg, p.head)->predecessors.push_back(p);
        dt_node temp = {
          .node_id = (int64_t)dtree.dt_nodes.size(),
          .type = PAR,
          .job_id = MAX_INT,
          .wcet = 0,
          .left = e1.id,
          .right = e2.id,
        };
        dtree.dt_nodes.push_back(temp);
      }

      if (1 == curr_vertex->predecessors.size() && 1 == curr_vertex->successors.size()) {
        ld_edge e1, e2, s;
        ld_vertex *predecessor = get_ld_vertex(&ldg, curr_vertex->predecessors[0].tail);
        ld_vertex *successor = get_ld_vertex(&ldg, curr_vertex->successors[0].head);

        e1 = curr_vertex->predecessors[0];
        delete_ld_edge(&ldg, e1.id);
        e2 = curr_vertex->successors[0];
        delete_ld_edge(&ldg, e2.id);

        s.id = dtree.dt_nodes.size();
        s.type = SER;
        s.head = successor->id;
        s.tail = predecessor->id;
        ldg.edges.push_back(s);
        predecessor->successors.push_back(s);
        successor->predecessors.push_back(s);
        // display_LDigraph(ldg);
        dt_node temp = {
          .node_id = (int64_t)dtree.dt_nodes.size(),
          .type = SER,
          .job_id = MAX_INT,
          .wcet = 0,
          .left = e1.id,
          .right = e2.id,
        };
        dtree.dt_nodes.push_back(temp);
        // display_dt_node(temp);

        bool p_unsatisfied = false, s_unsatisfied = false;
        foreach(unsatisfied, vertex) {
          if ((*vertex) == predecessor->id) {
            p_unsatisfied = true;
          }
          if ((*vertex) == successor->id) {
            s_unsatisfied = true;
          }
        }
        
        if (!p_unsatisfied && 0 != predecessor->predecessors.size()) {
          // add predecessor to unsatisfied set
          unsatisfied.push_back(predecessor->id);
        }
        
        if (!s_unsatisfied && 0 != successor->successors.size()) {
          // add successor to unsatisfied set
          unsatisfied.push_back(successor->id);
        }
      }
    }
  }

  for (uint i = 1; i < ldg.vertices.size() - 1; i++) {
    if (0 != ldg.vertices[i].predecessors.size() || 0 != ldg.vertices[i].successors.size()) {
      unsatisfied.push_back(i);
    }
  }

  ld_vertex *source = &ldg.vertices[0];
  ld_vertex *sink = &ldg.vertices[ldg.vertices.size()-1];

  while (1 < source->successors.size()) {
    ld_edge e1, e2, p;

    e1 = source->successors[0];
    delete_ld_edge(&ldg, e1.id);
    e2 = source->successors[0];
    delete_ld_edge(&ldg, e2.id);

    p.id = dtree.dt_nodes.size();
    p.type = PAR;
    p.head = sink->id;
    p.tail = source->id;
    ldg.edges.push_back(p);
    source->successors.push_back(p);
    sink->predecessors.push_back(p);
    dt_node temp = {
      .node_id = (int64_t)dtree.dt_nodes.size(),
      .type = PAR,
      .job_id = MAX_INT,
      .wcet = 0,
      .left = e1.id,
      .right = e2.id,
    };
    dtree.dt_nodes.push_back(temp);
  }
  
  dtree.root = dtree.dt_nodes.size() - 1;
  return dtree;
}

void display_dt_node(dt_node dtn) {
  cout << "job_id:" << dtn.job_id << " node_id: " << dtn.node_id << endl;
  cout << "type: " << dtn.type << endl;
  cout << "left: " << dtn.left << " right: " << dtn.right << endl;
  cout << "--------------------------------" << endl;
}

void display_dtree(DTree dtree) {
  foreach(dtree.dt_nodes, dtn)
    display_dt_node(*dtn);
}

void display_LDigraph(LDigraph ldgraph) {
  foreach(ldgraph.vertices, vertex) {
    cout << "vertex " << vertex->id << ":" << endl;
    cout << "  predecessors: ";
    foreach(vertex->predecessors, p) {
      cout << p->id << " ";
    }
    cout << endl;
    cout << "  successors: ";
    foreach(vertex->successors, p) {
      cout << p->id << " ";
    }
    cout << endl;
  }

  foreach(ldgraph.edges, edge) {
    cout << edge->tail << "--" << edge->id << "-->" << edge->head << endl;
  }
}

ld_vertex* get_ld_vertex(LDigraph *ldgraph, uint v_id) {
  foreach(ldgraph->vertices, vertex) {
    if (v_id == vertex->id)
      return &(*vertex);
  }
  return NULL;
}

set<uint> get_predecessors(ld_vertex vertex) {
  set<uint> predecessors;
  foreach(vertex.predecessors, edge) {
      predecessors.insert(edge->tail);
  }
  return predecessors;
}

set<uint> get_successors(ld_vertex vertex) {
  set<uint> successors;
  foreach(vertex.successors, edge) {
      successors.insert(edge->head);
  }
  return successors;
}

void delete_ld_edge(LDigraph *ldgraph, uint e_id) {
  // display_LDigraph(*ldgraph);
  foreach(ldgraph->edges, edge) {
    if (e_id == edge->id) {
      ld_vertex *head = get_ld_vertex(ldgraph, edge->head);
      ld_vertex *tail = get_ld_vertex(ldgraph, edge->tail);
  // deleting from head
      foreach(head->predecessors, e_head) {
        if (e_id == e_head->id) {
          head->predecessors.erase(e_head);
          break;
        }
      }
  // deleting from tail
      foreach(tail->successors, e_tail) {
        if (e_id == e_tail->id) {
          tail->successors.erase(e_tail);
          break;
        }
      }
  // deleting from graph
      ldgraph->edges.erase(edge);
      break;
    }
  }
  // display_LDigraph(*ldgraph);
}

DTree RTA_DAG_GFP_JOSE::get_decomposition_tree(const DAG_Task& ti) {
  DTree dtree;
  vector<uint> untouched;
  vector<uint> unlinked;
  dt_node T1, T2, temp;
  VNode vnode;
  int i;
  foreach(ti.get_vnodes(), vnode) {
    untouched.push_back(dtree.dt_nodes.size());
    dt_node temp = {
      .node_id = (int64_t)dtree.dt_nodes.size(),
      .type = EXT,
      .job_id = vnode->job_id,
      .wcet = (int64_t)vnode->wcet,
      .left = -1,
      .right = -1,
    };
    dtree.dt_nodes.push_back(temp);
  }

  
  // starting with subjobs without any arc
  for (i = 0; i < ti.get_vnode_num(); i++) {
    vnode = ti.get_vnodes()[i];
    if (0 == vnode.precedences.size() && 0 == vnode.follow_ups.size()) {
      unlinked.push_back(i);
    }
  }

  foreach(unlinked, node_1) {
    foreach(untouched, node_2) {
      if ((*node_1) == (*node_2)) {
        untouched.erase(node_2);
        break;
      }
    }
  }

  if (2 <= unlinked.size()) {
    T1 = dtree.dt_nodes[unlinked.front()];
    unlinked.erase(unlinked.begin());
    T2 = dtree.dt_nodes[unlinked.front()];
    unlinked.erase(unlinked.begin());

    untouched.push_back(dtree.dt_nodes.size());
    temp = {
      .node_id = (int64_t)dtree.dt_nodes.size(),
      .type = PAR,
      .job_id = MAX_INT,
      .wcet = 0,
      .left = T1.node_id,
      .right = T2.node_id,
    };
    dtree.dt_nodes.push_back(temp);

    while (0 < unlinked.size()) {

      T1 = dtree.dt_nodes[dtree.dt_nodes.size() - 1];
      untouched.erase(untouched.end() - 1);
      T2 = dtree.dt_nodes[unlinked.front()];
      unlinked.erase(unlinked.begin());


      untouched.push_back(dtree.dt_nodes.size());
      temp = {
        .node_id = (int64_t)dtree.dt_nodes.size(),
        .type = PAR,
        .job_id = MAX_INT,
        .wcet = 0,
        .left = T1.node_id,
        .right = T2.node_id,
      };
      dtree.dt_nodes.push_back(temp);
    }
  }
  

  while (1 < untouched.size()) {
    T1 = dtree.dt_nodes[untouched.front()];
    untouched.erase(untouched.begin());
    T2 = dtree.dt_nodes[untouched.front()];
    untouched.erase(untouched.begin());

    if (is_parallel_composition(ti, dtree, T1, T2)) {
      untouched.push_back(dtree.dt_nodes.size());
      temp = {
        .node_id = (int64_t)dtree.dt_nodes.size(),
        .type = PAR,
        .job_id = MAX_INT,
        .wcet = 0,
        .left = T1.node_id,
        .right = T2.node_id,
      };
      dtree.dt_nodes.push_back(temp);
    } else {
      untouched.push_back(dtree.dt_nodes.size());
      temp = {
        .node_id = (int64_t)dtree.dt_nodes.size(),
        .type = SER,
        .job_id = MAX_INT,
        .wcet = 0,
        .left = T1.node_id,
        .right = T2.node_id,
      };
      dtree.dt_nodes.push_back(temp);
    }
  }
  dtree.root = dtree.dt_nodes.size() - 1;
  return dtree;
}


bool RTA_DAG_GFP_JOSE::is_parallel_composition(const DAG_Task& ti, DTree dtree, dt_node T1, dt_node T2) {
  vector<uint> T1_nodes, T2_nodes;
  dt_node *curr;
  bool end = false;
  vector<dt_node*> stack;

  stack.push_back(&T1);
  while (0 != stack.size()) {
    curr = stack.back();
    stack.pop_back();
    if (EXT != curr->type) {
      stack.push_back(&dtree.dt_nodes[curr->right]);
      stack.push_back(&dtree.dt_nodes[curr->left]);
    } else {
      T1_nodes.push_back(curr->job_id);
    }
  }

  stack.push_back(&T2);
  while (0 != stack.size()) {
    curr = stack.back();
    stack.pop_back();
    if (EXT != curr->type) {
      stack.push_back(&dtree.dt_nodes[curr->right]);
      stack.push_back(&dtree.dt_nodes[curr->left]);
    } else {
      T2_nodes.push_back(curr->job_id);
    }
  }

  foreach(T1_nodes, v1) {
    foreach(T2_nodes, v2) {
      if ( (*v1) < (*v2)) {
        if (ti.is_arc_exist((*v1), (*v2)))
          return false;
      } else {
        if (ti.is_arc_exist((*v2), (*v1)))
          return false;
      }
    }
  }
  return true;
}

vector<int64_t> RTA_DAG_GFP_JOSE::par(DTree *dtree, int64_t node_id) {
  dt_node node = dtree->dt_nodes[node_id];
  vector<int64_t> nodes, l_nodes, r_nodes;
  if (PAR == node.type) {
    if (-1 != node.left)
      l_nodes = par(dtree, node.left);
    if (-1 != node.right)
      r_nodes = par(dtree, node.right);
    nodes = l_nodes;
    foreach(r_nodes, r_node) {
      nodes.push_back((*r_node));
    }
  } else if (SER == node.type) {
    if (-1 != node.left)
      l_nodes = par(dtree, node.left);
    if (-1 != node.right)
      r_nodes = par(dtree, node.right);
    if (l_nodes.size() >= r_nodes.size())
      nodes = l_nodes;
    else
      nodes = r_nodes;
  } else {
    nodes.push_back(node.node_id);
  }
  return nodes;
}

void RTA_DAG_GFP_JOSE::dt_delete(DTree *dtree, uint node_id) {
  dt_node *brother = NULL, *parent = NULL;
  uint parent_id;
  int i;

  foreach(dtree->dt_nodes, node) {
    if (node_id == node->node_id) {
      dtree->dt_nodes.erase(node);
      break;
    }
  }

  for(i = node_id; i < dtree->dt_nodes.size(); i++) {
    dtree->dt_nodes[i].node_id--;
    if ( dtree->dt_nodes[i].left > node_id)
      dtree->dt_nodes[i].left--;
    if ( dtree->dt_nodes[i].right > node_id)
      dtree->dt_nodes[i].right--;
  }

  foreach(dtree->dt_nodes, node) {
    if (node_id == node->left) {
      parent_id = node->node_id;
      parent = &(*node);
      parent->left = -1;
      brother = &dtree->dt_nodes[parent->right];
      dtree->dt_nodes.erase(node);
      break;
    } else if (node_id == node->right) {
      parent_id = node->node_id;
      parent = &(*node);
      parent->right = -1;
      brother = &dtree->dt_nodes[parent->left];
      dtree->dt_nodes.erase(node);
      break;
    }
  }

  if (NULL != parent) {
    foreach(dtree->dt_nodes, node) {
      if (parent_id == node->left) {
        node->left = brother->node_id;
        break;
      } else if (parent_id == node->right) {
        node->right = brother->node_id;
        break;
      }
    }

    for(i = parent_id; i < dtree->dt_nodes.size(); i++) {
      dtree->dt_nodes[i].node_id--;
      if ( dtree->dt_nodes[i].left > parent_id)
        dtree->dt_nodes[i].left--;
      if ( dtree->dt_nodes[i].right > parent_id)
        dtree->dt_nodes[i].right--;
    } 
  }

  dtree->root = dtree->dt_nodes.size() - 1;
}

uint64_t RTA_DAG_GFP_JOSE::get_CO(const DAG_Task& ti, workload_dist WD, int64_t interval) {
  int64_t sum = 0;
  int64_t temp, precedence;
  int64_t T_i = ti.get_period();
  int64_t R_i = ti.get_response_time();
  uint i, j;
  for (i = 0; i < WD.size(); i++) {
    precedence = 0;
    for (j = 0; j < i; j++) {
      precedence += WD[j].width;
    }
    temp = max(min(interval - precedence, WD[i].width), (int64_t)0);
    sum += WD[i].height * temp;
  }
  return min(sum, min((int64_t)processors.get_processor_num() * interval, (int64_t)ti.get_wcet() - max((int64_t)ti.get_critical_path_length() - interval, (int64_t)0)));
}

uint64_t RTA_DAG_GFP_JOSE::get_WC(const DAG_Task& ti, int64_t interval) {
  int64_t workload;
  int64_t x1, x2;
  workload_CIO WD_CIO;
  work_block temp;
  int i;

  foreach(workload_CIOs, w_CIO) {
    if (w_CIO->task_id == ti.get_id())
      WD_CIO = *w_CIO;
  }

  workload = get_CO(ti, WD_CIO.WD_CO, interval);
  x1 = ti.get_period() - ti.get_response_time();

  for(i = WD_CIO.WD_CI.size() -1; i >= 0; i--) {
    temp = WD_CIO.WD_CI[i];
    x1 += temp.width;
    x2 = interval - x1;
    workload = max(workload, (int64_t)get_CI(ti, WD_CIO.WD_CI, x1) + (int64_t)get_CI(ti, WD_CIO.WD_CO, x2));
  }

  workload = max(workload, (int64_t)get_CI(ti, WD_CIO.WD_CI, interval));
  x2 = 0;
  for(i = 0; i < WD_CIO.WD_CO.size(); i++) {
    temp = WD_CIO.WD_CO[i];
    x2 += temp.width;
    x1 = interval - x2;
    workload = max(workload, (int64_t)get_CI(ti, WD_CIO.WD_CI, x1) + (int64_t)get_CI(ti, WD_CIO.WD_CO, x2));
  }
  return workload;
}

uint64_t RTA_DAG_GFP_JOSE::intra_interference(const DAG_Task& ti) {
  return ti.get_wcet() - ti.get_critical_path_length();
}

uint64_t RTA_DAG_GFP_JOSE::inter_interference(const DAG_Task& ti,
                                               int64_t interval) {
  uint64_t inter_interf = 0;
  int64_t delta_C;
  int64_t L_x;
  int64_t T_x;
  int64_t W_x;
  try {
    foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
      L_x = tx->get_critical_path_length();
      T_x = tx->get_period();
      W_x = tx->get_wcet();
      delta_C = interval - max((int64_t)0, (interval - L_x)/ T_x) * T_x;
      inter_interf += max((int64_t)0, (interval - delta_C)/ T_x) * W_x + (int64_t)get_WC((*tx), delta_C);
    }
  } catch (exception& e) {
    cout << e.what() << endl;
  }
  return inter_interf;
}

uint64_t RTA_DAG_GFP_JOSE::response_time(const DAG_Task& ti) {
  uint32_t t_id = ti.get_id();
  uint64_t CPL = ti.get_critical_path_length();
  uint64_t intra_interf = intra_interference(ti);
  uint p_num = processors.get_processor_num();

  uint64_t test_end = ti.get_deadline();
  uint64_t test_start = CPL + intra_interf / p_num;
  uint64_t response = test_start;
  uint64_t demand = 0;

  while (response <= test_end) {
    uint64_t inter_interf = inter_interference(ti, response);
    demand = test_start + inter_interf / p_num;

    if (response >= demand) {
      return response;
    } else {
      response = demand;
    }
  }
  return test_end + 100;
}

bool RTA_DAG_GFP_JOSE::is_schedulable() {
  uint64_t response_bound;

  foreach(tasks.get_tasks(), ti) {
    if (ti->get_critical_path_length() > ti->get_deadline())
      return false;
    response_bound = response_time((*ti));
    if (response_bound > ti->get_deadline()) {
      return false;
    }
    ti->set_response_time(response_bound);
  }
  return true;
}

/**
 * with blocking analysis for FMLP [Non-LP-based]
 **/


RTA_DAG_GFP_JOSE_FMLP::RTA_DAG_GFP_JOSE_FMLP() : RTA_DAG_GFP_JOSE() {}

RTA_DAG_GFP_JOSE_FMLP::RTA_DAG_GFP_JOSE_FMLP(DAG_TaskSet tasks, ProcessorSet processors,
                  ResourceSet resources) : RTA_DAG_GFP_JOSE(tasks, processors, resources) {}

RTA_DAG_GFP_JOSE_FMLP::~RTA_DAG_GFP_JOSE_FMLP() {}

uint32_t RTA_DAG_GFP_JOSE_FMLP::instance_number(const DAG_Task& ti,
                                                int64_t interval) {
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  return ceiling(interval + r, p);
}

uint64_t RTA_DAG_GFP_JOSE_FMLP::intra_blocking(const DAG_Task& ti,
                                          uint32_t resource_id,
                                          uint32_t request_num) {
  const Request& request = ti.get_request_by_id(resource_id);
  uint32_t N_i_q = request.get_num_requests();
  uint64_t L_i_q = request.get_max_length();
  uint32_t X_i_q = request_num;
  uint32_t Y_i_q = request.get_num_requests() - X_i_q;
  uint32_t m_i = ti.get_parallel_degree();
  return min((m_i - 1) * X_i_q, Y_i_q) * L_i_q;
}

uint64_t RTA_DAG_GFP_JOSE_FMLP::inter_blocking(const DAG_Task& ti,
                                          uint32_t resource_id,
                                          uint32_t request_num) {
  const Request& request = ti.get_request_by_id(resource_id);
  uint32_t N_i_q = request.get_num_requests();
  uint64_t L_i_q = request.get_max_length();
  uint32_t X_i_q = request_num;
  uint32_t Y_i_q = request.get_num_requests() - X_i_q;
  uint64_t d_i = ti.get_deadline();
  uint64_t sum = 0;
  foreach_task_except(tasks.get_tasks(), ti, tx) {
    if (tx->is_request_exist(resource_id)) {
      const Request& request_x = tx->get_request_by_id(resource_id);
      uint32_t N_x_q = request_x.get_num_requests();
      uint64_t L_x_q = request_x.get_max_length();
      // uint32_t MPD = min(processors.get_processor_num(),
      //                tx->get_parallel_degree());  // Minimum Parallel Degree
      uint32_t m_x = tx->get_parallel_degree();
      sum +=
          min(min(m_x, N_x_q) * X_i_q, (instance_number((*tx), d_i) * N_x_q))
          * L_x_q;
    }
  }
  return sum;
}

uint64_t RTA_DAG_GFP_JOSE_FMLP::CS_workload(const DAG_Task& ti, uint32_t resource_id,
                                    uint64_t interval) {
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  const Request& request = ti.get_request_by_id(resource_id);
  uint64_t agent_length = request.get_num_requests() * request.get_max_length();
  return ceiling((interval + r - agent_length), p) * agent_length;
}

uint64_t RTA_DAG_GFP_JOSE_FMLP::inter_interference(const DAG_Task& ti,
                                              int64_t interval) {
  uint64_t inter_interf = 0;
  int64_t delta_C;
  int64_t L_x;
  int64_t T_x;
  int64_t W_x;
  try {
    foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
      L_x = tx->get_critical_path_length();
      T_x = tx->get_period();
      W_x = tx->get_wcet();
      delta_C = interval - max((int64_t)0, (interval - L_x)/ T_x) * T_x;
      inter_interf += max((int64_t)0, (interval - delta_C)/ T_x) * W_x + (int64_t)get_WC((*tx), delta_C);
    }
  } catch (exception& e) {
    cout << e.what() << endl;
  }

  try {
    foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
      foreach(tl->get_requests(), l_u) {
        uint32_t r_id = l_u->get_resource_id();
        Resource& resource = resources.get_resource_by_id(r_id);
        if (resource.get_ceiling() < ti.get_priority()) {
          inter_interf += CS_workload((*tl), r_id, interval);
        }
      }
    }
  } catch (exception& e) {
    cout << e.what() << endl;
  }
  
  return inter_interf;
}

uint64_t RTA_DAG_GFP_JOSE_FMLP::response_time(const DAG_Task& ti) {
  uint32_t t_id = ti.get_id();
  uint64_t CPL = ti.get_critical_path_length();
  uint64_t intra_interf = intra_interference(ti);
  uint64_t total_blocking = 0;
  uint p_num = processors.get_processor_num();

  foreach(ti.get_requests(), request) {
    uint32_t r_id = request->get_resource_id();
    uint64_t blocking = 0;
    uint32_t N_i_q = request->get_num_requests();
    for (uint32_t x = 1; x <= N_i_q; x++) {
      uint64_t temp = intra_blocking(ti, r_id, x) + inter_blocking(ti, r_id, x);
      if (temp > blocking)
        blocking = temp;
    }
    total_blocking += blocking;
  }

  uint64_t test_end = ti.get_deadline();
  uint64_t test_start = CPL + total_blocking + intra_interf / p_num;
  uint64_t response = test_start;
  uint64_t demand = 0;

  while (response <= test_end) {
    uint64_t inter_interf = inter_interference(ti, response);
    demand = test_start + inter_interf / p_num;

    if (response >= demand) {
      return response;
    } else {
      response = demand;
    }
  }
return test_end + 100;
}

bool RTA_DAG_GFP_JOSE_FMLP::is_schedulable() {
  uint64_t response_bound;

  foreach(tasks.get_tasks(), ti) {
    response_bound = response_time((*ti));
    if (response_bound > ti->get_deadline()) {
      return false;
    }
    ti->set_response_time(response_bound);
  }
  return true;
}

/**
 * with blocking analysis for PIP [Non-LP-based]
 **/


RTA_DAG_GFP_JOSE_PIP::RTA_DAG_GFP_JOSE_PIP() : RTA_DAG_GFP_JOSE() {}

RTA_DAG_GFP_JOSE_PIP::RTA_DAG_GFP_JOSE_PIP(DAG_TaskSet tasks, ProcessorSet processors,
                  ResourceSet resources) : RTA_DAG_GFP_JOSE(tasks, processors, resources) {}

RTA_DAG_GFP_JOSE_PIP::~RTA_DAG_GFP_JOSE_PIP() {}

uint32_t RTA_DAG_GFP_JOSE_PIP::instance_number(const DAG_Task& ti,
                                                int64_t interval) {
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  return ceiling(interval + r, p);
}

uint64_t RTA_DAG_GFP_JOSE_PIP::intra_blocking(const DAG_Task& ti,
                                          uint32_t resource_id,
                                          uint32_t request_num) {
  const Request& request = ti.get_request_by_id(resource_id);
  uint32_t N_i_q = request.get_num_requests();
  uint64_t L_i_q = request.get_max_length();
  uint32_t X_i_q = request_num;
  uint32_t Y_i_q = request.get_num_requests() - X_i_q;
  uint32_t m_i = ti.get_parallel_degree();
  return min((m_i - 1) * X_i_q, Y_i_q) * L_i_q;
}

uint64_t RTA_DAG_GFP_JOSE_PIP::inter_blocking(const DAG_Task& ti,
                                          uint32_t resource_id,
                                          uint32_t request_num) {
  uint64_t d_i = ti.get_deadline();
  uint64_t L_l_q = 0;
  foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
    if (!tl->is_request_exist(resource_id))
      continue;
    uint64_t temp = tl->get_request_by_id(resource_id).get_max_length();
    if (L_l_q < temp)
      L_l_q = temp;
  }

  uint64_t inter_h = 0;
  foreach_higher_priority_task(tasks.get_tasks(), ti, th) {
    if (!th->is_request_exist(resource_id))
      continue;
    const Request& request = th->get_request_by_id(resource_id);
    uint32_t N_h_q = request.get_num_requests();
    uint64_t L_h_q = request.get_max_length();
    uint32_t N_h = instance_number((*th), d_i);

    inter_h += N_h * N_h_q * L_h_q;
  }
  return L_l_q * request_num + inter_h;
}

uint64_t RTA_DAG_GFP_JOSE_PIP::CS_workload(const DAG_Task& ti, uint32_t resource_id,
                                    uint64_t interval) {
  uint64_t e = ti.get_wcet();
  uint64_t p = ti.get_period();
  uint64_t r = ti.get_response_time();
  const Request& request = ti.get_request_by_id(resource_id);
  uint64_t agent_length = request.get_num_requests() * request.get_max_length();
  return ceiling((interval + r - agent_length), p) * agent_length;
}

uint64_t RTA_DAG_GFP_JOSE_PIP::inter_interference(const DAG_Task& ti,
                                              int64_t interval) {
  uint64_t inter_interf = 0;
  int64_t delta_C;
  int64_t L_x;
  int64_t T_x;
  int64_t W_x;
  try {
    foreach_higher_priority_task(tasks.get_tasks(), ti, tx) {
      L_x = tx->get_critical_path_length();
      T_x = tx->get_period();
      W_x = tx->get_wcet();
      delta_C = interval - max((int64_t)0, (interval - L_x)/ T_x) * T_x;
      uint64_t temp = max((int64_t)0, (interval - delta_C)/ T_x) * W_x + (int64_t)get_WC((*tx), delta_C);
      inter_interf += temp;
    }
  } catch (exception& e) {
    cout << e.what() << endl;
  }

  try {
    foreach_lower_priority_task(tasks.get_tasks(), ti, tl) {
      foreach(tl->get_requests(), l_u) {
        uint32_t r_id = l_u->get_resource_id();
        Resource& resource = resources.get_resource_by_id(r_id);
        if (resource.get_ceiling() <= ti.get_priority()) {
          inter_interf += CS_workload((*tl), r_id, interval);
        }
      }
    }
  } catch (exception& e) {
    cout << e.what() << endl;
  }
  
  return inter_interf;
}

uint64_t RTA_DAG_GFP_JOSE_PIP::response_time(const DAG_Task& ti) {
  uint32_t t_id = ti.get_id();
  uint64_t CPL = ti.get_critical_path_length();
  uint64_t intra_interf = intra_interference(ti);
  uint64_t total_blocking = 0;
  uint p_num = processors.get_processor_num();

  uint64_t intra_b = 0;
  foreach(ti.get_requests(), request) {
    uint32_t r_id = request->get_resource_id();
    uint64_t blocking = 0;
    uint32_t N_i_q = request->get_num_requests();
    uint32_t X_log;
    for (uint32_t x = 1; x <= N_i_q; x++) {
      uint64_t temp = intra_blocking(ti, r_id, x) + inter_blocking(ti, r_id, x);
      if (temp > blocking) {
        blocking = temp;
        X_log = x;
      }
    }
    intra_b = intra_blocking(ti, r_id, X_log);
    total_blocking += blocking;
  }

  uint64_t test_end = ti.get_deadline();
  uint64_t test_start = CPL + total_blocking + intra_interf / p_num;
  uint64_t response = test_start;
  uint64_t demand = 0;

  while (response <= test_end) {
    uint64_t inter_interf = inter_interference(ti, response);
    demand = test_start + inter_interf / p_num;

    if (response >= demand) {
      return response;
    } else {
      response = demand;
    }
  }
return test_end + 100;
}

bool RTA_DAG_GFP_JOSE_PIP::is_schedulable() {
  uint64_t response_bound;

  foreach(tasks.get_tasks(), ti) {
    response_bound = response_time((*ti));
    if (response_bound > ti->get_deadline()) {
      return false;
    }
    ti->set_response_time(response_bound);
  }
  return true;
}
