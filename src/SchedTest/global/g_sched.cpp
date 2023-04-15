// Copyright [2017] <Zewei Chen>
// ------By Zewei Chen------
// Email:czwking1991@gmail.com
#include <g_sched.h>

GlobalSched::GlobalSched(bool LinearProgramming, uint TestMethod,
                         uint PriorityAssignment, uint LockingProtocol,
                         string name, string remark)
    : SchedTestBase(LinearProgramming, TestMethod, GLOBAL, PriorityAssignment,
                    LockingProtocol, name, remark) {}

GlobalSched::~GlobalSched() {}
