
CC            = gcc
CXX           = g++
DEFINES       = -DQT_NO_DEBUG -DQT_PRINTSUPPORT_LIB -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB
CFLAGS        = -pipe -O2 -Wall -W -D_REENTRANT -fPIE $(DEFINES) -fpermissive
CXXFLAGS      = -pipe -O2 -Wall -W -g -D_REENTRANT -fPIE $(DEFINES) -std=gnu++11 -fpermissive -static-libstdc++ 
#-fext-numeric-literals 
GTKMMFLAGS	  =	`pkg-config gtkmm-3.0 --cflags --libs`
GLIBMMGLAGS	  =	`pkg-config glibmm-2.4 --cflags --libs`
INCPATH       = -I. -I../include -I../include/SchedTest -I../include/SchedTest/global -I../include/SchedTest/global/lp -I../include/SchedTest/global/DAG -I../include/SchedTest/partitioned -I../include/SchedTest/partitioned/lp -I../include/SchedTest/partitioned/DAG -I../include/SchedTest/lp -I../include/QCustomPlot -I../include/mathGL -I../include/LP -I../include/SchedTest/blocking/lp
LINK          = g++
LFLAGS        = #-Wl,-O1
LIBS          = -lgmp -lgmpxx -lpthread -lglpk -lmgl -lmysqlclient
SERVER_LIBS   = -lgmp -lgmpxx -lpthread -lglpk -lmgl 
CLIENT_LIBS   = -lgmp -lgmpxx -lpthread -lglpk
COPY          = cp -f
COPY_FILE     = cp -f
COPY_DIR      = cp -f -R
DEL_FILE      = rm -f
DEL_DIR       = rmdir
MOVE          = mv -f
MKDIR         = mkdir -p
TARGET	      = pso_test

BASIC_OBJ_FILES =tasks.o processors.o resources.o param.o random_gen.o tinyxml2.o xml.o sched_result.o time_record.o toolkit.o 

SCHED_TEST_OBJ_FILES = lp.o solution.o varmapper.o sched_test_base.o sched_test_factory.o p_sched.o g_sched.o rta_gfp_native.o rta_gfp_bc.o rta_gfp_ng.o rta_pfp_wf.o rta_pfp_ff.o rta_pfp_ff_opa.o rta_pfp_wf_spinlock.o rta_pfp_wf_spinlock_heterogeneous.o rta_pfp_wf_semaphore.o lp_rta_gfp_pip.o lp_rta_gfp_fmlp.o lp_rta_pfp_dpcp.o lp_rta_pfp_mpcp.o lp_rta_pfp_mpcp_heterogeneous.o lp_rta_pfp_rop_dpcp.o lp_rta_pfp_rop_dpcp_plus.o ilp_rta_pfp_spinlock.o ilp_rta_pfp_rop.o rta_pfp_gs.o nc_lp_ee_vpr.o rta_gdc_native.o rta_pdc_ro.o rta_pfp_rop.o rta_pfp_rop_heterogeneous.o rta_pfp_rop_fast_first.o rta_pfp_rop_fast_first_v2.o rta_pfp_rop_slow_first.o rta_pfp_ro_sm.o rta_pfp_ro_np.o rta_pfp_ro_np_opa.o rta_pfp_ro_np_opa_p2.o rta_pfp_ro_np_opa_p3.o rta_pfp_ro_np_sm.o rta_pfp_ro_np_sm_opa.o rta_pfp_ro_opa.o rta_pfp_ro_sm_opa.o rta_pfp_ro_feasible.o rta_dag_gfp_fmlp.o rta_dag_gfp_pip.o rta_dag_cgfp_fmlp.o rta_dag_cgfp_pip.o cab_dag_fed_spin_fifo.o cab_dag_fed_spin_prio.o rta_dag_gfp_spin_fifo.o rta_dag_gfp_spin_prio.o lp_rta_dag_gfp_fmlp.o lp_rta_dag_gfp_pip.o rta_dag_fed_lpp.o rta_dag_fed_omlp.o rta_dag_fed_omip.o rta_dag_fed_spin_xu.o rta_dag_fed_hybridlocks.o rta_dag_fed_fmlp.o rta_dag_fed_fp.o rta_dag_fed_li.o rta_dag_sfed_xu.o rta_dag_cgfp.o rta_dag_cgedf.o rta_dag_gfp_mel.o rta_dag_gedf_mel.o cab_dag_gfp_li.o cab_dag_gedf_li.o rta_dag_fed_dpcp.o rta_dag_gfp_jose.o  rta_dag_gedf_hoo.o ilp_rta_fed_h2lp.o lp_rta_gfp_pfmlp.o lp_rta_gfp_ppip.o ilp_rta_fed_h2lp_v2.o ilp_rta_fed_h2lp_v3.o ilp_rta_fed_h2lp_v4.o lp_rta_fed_spin.o lp_rta_gfp_spin.o lp_rta_gfp_spin_xu.o lp_rta_pfp_gpu.o ilp_rta_fed_mpcp.o  rta_pso.o

OUTPUT_OBJ_FILES = mgl_chart.o output.o

OBJ_FILES	  = $(BASIC_OBJ_FILES) $(SCHED_TEST_OBJ_FILES) $(OUTPUT_OBJ_FILES)

SERVER_OBJ_FILES	  = $(BASIC_OBJ_FILES) $(OUTPUT_OBJ_FILES)

CLIENT_OBJ_FILES	  = $(BASIC_OBJ_FILES) $(SCHED_TEST_OBJ_FILES)




pso_test:pso_test.o $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(INCPATH) -o $(TARGET) pso_test.o $(OBJ_FILES) $(LFLAGS) $(LIBS) -std=gnu++11



SchedulingAnalysis:scheduling_test.o $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(INCPATH) -o SchedulingAnalysis scheduling_test.o $(OBJ_FILES) $(LFLAGS) $(LIBS) -std=gnu++11

BlockingTest:Blocking_Test.o $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(INCPATH) -o BlockingTest Blocking_Test.o $(OBJ_FILES) $(LFLAGS) $(LIBS) -std=gnu++11

DAGTest:dag_test.o $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(INCPATH) -o DAGTest dag_test.cpp $(OBJ_FILES) $(LFLAGS) $(LIBS) -std=gnu++11
	
runtime_test:runtime_complexity.o $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(INCPATH) -o rt_test runtime_complexity.o $(OBJ_FILES) $(LFLAGS) $(LIBS) -std=gnu++11

STJJC:STJJC.o $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(INCPATH) -o STJJC STJJC.o $(OBJ_FILES) $(LFLAGS) $(LIBS) -std=gnu++11

SubTest:sub_test.o $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(INCPATH) -o SubTest sub_test.o $(OBJ_FILES) $(LFLAGS) $(LIBS) -std=gnu++11

server:server.o $(SERVER_OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(INCPATH) -o server server.o $(SERVER_OBJ_FILES) $(LFLAGS) $(SERVER_LIBS) -std=gnu++11

client:client.o $(CLIENT_OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(INCPATH) -o client client.o $(CLIENT_OBJ_FILES) $(LFLAGS) $(CLIENT_LIBS) -std=gnu++11

bt_server:bt_server.o $(SERVER_OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(INCPATH) -o bt_server bt_server.o $(SERVER_OBJ_FILES) $(LFLAGS) $(SERVER_LIBS) -std=gnu++11

bt_client:bt_client.o $(CLIENT_OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(INCPATH) -o bt_client bt_client.o $(CLIENT_OBJ_FILES) $(LFLAGS) $(CLIENT_LIBS) -std=gnu++11

task_gen_test:task_gen_test.o $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(INCPATH) -o task_gen_test task_gen_test.o $(CLIENT_OBJ_FILES) $(LFLAGS) $(CLIENT_LIBS) -std=gnu++11

dag_task_gen_test:dag_task_gen_test.o $(OBJ_FILES)
	$(CXX) $(CXXFLAGS) $(INCPATH) -o dag_task_gen_test dag_task_gen_test.o $(CLIENT_OBJ_FILES) $(LFLAGS) $(CLIENT_LIBS) -std=gnu++11

test:test.o tasks.o processors.o tinyxml2.o random_gen.o mgl_chart.o resources.o xml.o param.o sched_result.o output.o
	$(CXX) $(CXXFLAGS) $(INCPATH) -o test test.o tasks.o processors.o tinyxml2.o random_gen.o mgl_chart.o resources.o xml.o param.o sched_result.o output.o  $(LFLAGS) $(LIBS) -std=gnu++11

pso_test.o:pso_test.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c pso_test.cpp $(LIBS)

scheduling_test.o:scheduling_test.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c scheduling_test.cpp $(LIBS)

Blocking_Test.o:Blocking_Test.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c Blocking_Test.cpp $(LIBS)
	


dag_test.o:dag_test.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c dag_test.cpp $(LIBS)
	
runtime_complexity.o:runtime_complexity.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c runtime_complexity.cpp $(LIBS)



STJJC.o:Collaboration/UTD/STJJC.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c Collaboration/UTD/STJJC.cpp $(LIBS)

sub_test.o:sub_test.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c sub_test.cpp $(LIBS)

server.o:server.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c server.cpp $(LIBS)

client.o:client.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c client.cpp $(LIBS)

bt_server.o:bt_server.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c bt_server.cpp $(LIBS)

bt_client.o:bt_client.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c bt_client.cpp $(LIBS)

task_gen_test.o:task_gen_test.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c task_gen_test.cpp $(LIBS)

dag_task_gen_test.o:dag_task_gen_test.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c dag_task_gen_test.cpp $(LIBS)

test.o:test.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c test.cpp $(LIBS)

tasks.o:tasks.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c tasks.cpp $(LIBS)

processors.o:processors.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c processors.cpp $(LIBS)

random_gen.o:random_gen.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c random_gen.cpp $(LIBS)

resources.o:resources.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c resources.cpp $(LIBS)

tinyxml2.o:tinyxml2.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c tinyxml2.cpp $(LIBS)

mgl_chart.o:mathGL/mgl_chart.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c mathGL/mgl_chart.cpp $(LIBS)

xml.o:xml.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c xml.cpp $(LIBS)

param.o:param.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c param.cpp $(LIBS)

sched_result.o:sched_result.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c sched_result.cpp $(LIBS)

output.o:output.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c output.cpp $(LIBS)

time_record.o:time_record.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c time_record.cpp $(LIBS)

toolkit.o:toolkit.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c toolkit.cpp $(LIBS)

#GTKMM_window.o:GTKMM_window.cpp
#	$(CXX) $(GTKMMFLAGS) $(GLIBMMGLAGS) $(CXXFLAGS) $(INCPATH) -c GTKMM_window.cpp $(LIBS)

lp.o:LP/lp.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c LP/lp.cpp $(LIBS)

solution.o:LP/solution.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c LP/solution.cpp $(LIBS)

varmapper.o:LP/varmapper.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c LP/varmapper.cpp $(LIBS)

sched_test_base.o:SchedTest/sched_test_base.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/sched_test_base.cpp $(LIBS)

sched_test_factory.o:SchedTest/sched_test_factory.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/sched_test_factory.cpp $(LIBS)

p_sched.o:SchedTest/partitioned/p_sched.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/p_sched.cpp $(LIBS)

g_sched.o:SchedTest/global/g_sched.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/g_sched.cpp $(LIBS)


rta_gfp_native.o:SchedTest/global/rta_gfp_native.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/rta_gfp_native.cpp $(LIBS)

rta_gfp_bc.o:SchedTest/global/rta_gfp_bc.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/rta_gfp_bc.cpp $(LIBS)

rta_gfp_ng.o:SchedTest/global/rta_gfp_ng.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/rta_gfp_ng.cpp $(LIBS)


rta_pso.o:SchedTest/partitioned/rta_pso.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/rta_pso.cpp $(LIBS)

rta_pfp_wf.o:SchedTest/partitioned/rta_pfp_wf.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/rta_pfp_wf.cpp $(LIBS)

rta_pfp_ff.o:SchedTest/partitioned/rta_pfp_ff.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/rta_pfp_ff.cpp $(LIBS)

rta_pfp_ff_opa.o:SchedTest/partitioned/rta_pfp_ff_opa.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/rta_pfp_ff_opa.cpp $(LIBS)

rta_pfp_wf_spinlock.o:SchedTest/partitioned/rta_pfp_wf_spinlock.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/rta_pfp_wf_spinlock.cpp $(LIBS)

rta_pfp_wf_spinlock_heterogeneous.o:SchedTest/partitioned/rta_pfp_wf_spinlock_heterogeneous.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/rta_pfp_wf_spinlock_heterogeneous.cpp $(LIBS)

rta_pfp_wf_semaphore.o:SchedTest/partitioned/rta_pfp_wf_semaphore.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/rta_pfp_wf_semaphore.cpp $(LIBS)

lp_rta_gfp_pip.o:SchedTest/global/lp/lp_rta_gfp_pip.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/lp/lp_rta_gfp_pip.cpp $(LIBS)

lp_rta_gfp_fmlp.o:SchedTest/global/lp/lp_rta_gfp_fmlp.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/lp/lp_rta_gfp_fmlp.cpp $(LIBS)

lp_rta_pfp_dpcp.o:SchedTest/partitioned/lp/lp_rta_pfp_dpcp.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/lp/lp_rta_pfp_dpcp.cpp $(LIBS)

lp_rta_pfp_mpcp.o:SchedTest/partitioned/lp/lp_rta_pfp_mpcp.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/lp/lp_rta_pfp_mpcp.cpp $(LIBS)

lp_rta_pfp_mpcp_heterogeneous.o:SchedTest/partitioned/lp/lp_rta_pfp_mpcp_heterogeneous.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/lp/lp_rta_pfp_mpcp_heterogeneous.cpp $(LIBS)

lp_rta_pfp_rop_dpcp.o:SchedTest/partitioned/lp/lp_rta_pfp_rop_dpcp.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/lp/lp_rta_pfp_rop_dpcp.cpp $(LIBS)

lp_rta_pfp_rop_dpcp_plus.o:SchedTest/partitioned/lp/lp_rta_pfp_rop_dpcp_plus.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/lp/lp_rta_pfp_rop_dpcp_plus.cpp $(LIBS)

rta_pfp_ro.o:SchedTest/partitioned/rta_pfp_ro.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/rta_pfp_ro.cpp $(LIBS)

ilp_rta_pfp_spinlock.o:SchedTest/partitioned/lp/ilp_rta_pfp_spinlock.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/lp/ilp_rta_pfp_spinlock.cpp $(LIBS)

ilp_rta_pfp_rop.o:SchedTest/partitioned/lp/ilp_rta_pfp_rop.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/lp/ilp_rta_pfp_rop.cpp $(LIBS)

ilp_rta_pfp_rop_comp.o:SchedTest/partitioned/lp/ilp_rta_pfp_rop_comp.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/lp/ilp_rta_pfp_rop_comp.cpp $(LIBS)

rta_pfp_gs.o:SchedTest/partitioned/rta_pfp_gs.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/rta_pfp_gs.cpp $(LIBS)

nc_lp_ee_vpr.o:SchedTest/global/nc_lp_ee_vpr.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/nc_lp_ee_vpr.cpp $(LIBS)

rta_gdc_native.o:SchedTest/global/rta_gdc_native.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/rta_gdc_native.cpp $(LIBS)

rta_pdc_ro.o:SchedTest/partitioned/rta_pdc_ro.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/rta_pdc_ro.cpp $(LIBS)

rta_pfp_rop.o:SchedTest/partitioned/rta_pfp_rop.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/rta_pfp_rop.cpp $(LIBS)

rta_pfp_rop_heterogeneous.o:SchedTest/partitioned/rta_pfp_rop_heterogeneous.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/rta_pfp_rop_heterogeneous.cpp $(LIBS)

rta_pfp_rop_fast_first.o:SchedTest/partitioned/rta_pfp_rop_fast_first.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/rta_pfp_rop_fast_first.cpp $(LIBS)

rta_pfp_rop_fast_first_v2.o:SchedTest/partitioned/rta_pfp_rop_fast_first_v2.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/rta_pfp_rop_fast_first_v2.cpp $(LIBS)

rta_pfp_rop_slow_first.o:SchedTest/partitioned/rta_pfp_rop_slow_first.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/rta_pfp_rop_slow_first.cpp $(LIBS)

rta_pfp_ro_sm.o:SchedTest/partitioned/rta_pfp_ro_sm.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/rta_pfp_ro_sm.cpp $(LIBS)

rta_pfp_ro_np.o:SchedTest/partitioned/rta_pfp_ro_np.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/rta_pfp_ro_np.cpp $(LIBS)

rta_pfp_ro_np_opa.o:SchedTest/partitioned/rta_pfp_ro_np_opa.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/rta_pfp_ro_np_opa.cpp $(LIBS)

rta_pfp_ro_np_opa_p2.o:SchedTest/partitioned/rta_pfp_ro_np_opa_p2.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/rta_pfp_ro_np_opa_p2.cpp $(LIBS)

rta_pfp_ro_np_opa_p3.o:SchedTest/partitioned/rta_pfp_ro_np_opa_p3.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/rta_pfp_ro_np_opa_p3.cpp $(LIBS)

rta_pfp_ro_np_sm.o:SchedTest/partitioned/rta_pfp_ro_np_sm.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/rta_pfp_ro_np_sm.cpp $(LIBS)

rta_pfp_ro_np_sm_opa.o:SchedTest/partitioned/rta_pfp_ro_np_sm_opa.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/rta_pfp_ro_np_sm_opa.cpp $(LIBS)

rta_pfp_ro_opa.o:SchedTest/partitioned/rta_pfp_ro_opa.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/rta_pfp_ro_opa.cpp $(LIBS)

rta_pfp_ro_sm_opa.o:SchedTest/partitioned/rta_pfp_ro_sm_opa.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/rta_pfp_ro_sm_opa.cpp $(LIBS)

rta_pfp_ro_feasible.o:SchedTest/partitioned/rta_pfp_ro_feasible.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/rta_pfp_ro_feasible.cpp $(LIBS)

rta_dag_gfp_fmlp.o:SchedTest/global/DAG/rta_dag_gfp_fmlp.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/DAG/rta_dag_gfp_fmlp.cpp $(LIBS)

rta_dag_gfp_pip.o:SchedTest/global/DAG/rta_dag_gfp_pip.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/DAG/rta_dag_gfp_pip.cpp $(LIBS)

rta_dag_cgfp_fmlp.o:SchedTest/global/DAG/rta_dag_cgfp_fmlp.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/DAG/rta_dag_cgfp_fmlp.cpp $(LIBS)

rta_dag_cgfp_pip.o:SchedTest/global/DAG/rta_dag_cgfp_pip.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/DAG/rta_dag_cgfp_pip.cpp $(LIBS)

cab_dag_fed_spin_fifo.o:SchedTest/partitioned/DAG/cab_dag_fed_spin_fifo.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/DAG/cab_dag_fed_spin_fifo.cpp $(LIBS)

cab_dag_fed_spin_prio.o:SchedTest/partitioned/DAG/cab_dag_fed_spin_prio.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/DAG/cab_dag_fed_spin_prio.cpp $(LIBS)

rta_dag_gfp_spin_fifo.o:SchedTest/global/DAG/rta_dag_gfp_spin_fifo.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/DAG/rta_dag_gfp_spin_fifo.cpp $(LIBS)

rta_dag_gfp_spin_prio.o:SchedTest/global/DAG/rta_dag_gfp_spin_prio.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/DAG/rta_dag_gfp_spin_prio.cpp $(LIBS)

lp_rta_dag_gfp_fmlp.o:SchedTest/global/DAG/lp_rta_dag_gfp_fmlp.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/DAG/lp_rta_dag_gfp_fmlp.cpp $(LIBS)

lp_rta_dag_gfp_pip.o:SchedTest/global/DAG/lp_rta_dag_gfp_pip.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/DAG/lp_rta_dag_gfp_pip.cpp $(LIBS)

rta_dag_fed_lpp.o:SchedTest/partitioned/DAG/rta_dag_fed_lpp.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/DAG/rta_dag_fed_lpp.cpp $(LIBS)

rta_dag_fed_omlp.o:SchedTest/partitioned/DAG/rta_dag_fed_omlp.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/DAG/rta_dag_fed_omlp.cpp $(LIBS)

rta_dag_fed_omip.o:SchedTest/partitioned/DAG/rta_dag_fed_omip.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/DAG/rta_dag_fed_omip.cpp $(LIBS)

rta_dag_fed_spin_xu.o:SchedTest/partitioned/DAG/rta_dag_fed_spin_xu.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/DAG/rta_dag_fed_spin_xu.cpp $(LIBS)

rta_dag_fed_hybridlocks.o:SchedTest/partitioned/DAG/rta_dag_fed_hybridlocks.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/DAG/rta_dag_fed_hybridlocks.cpp $(LIBS)

rta_dag_fed_fmlp.o:SchedTest/partitioned/DAG/rta_dag_fed_fmlp.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/DAG/rta_dag_fed_fmlp.cpp $(LIBS)

rta_dag_fed_fp.o:SchedTest/partitioned/DAG/rta_dag_fed_fp.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/DAG/rta_dag_fed_fp.cpp $(LIBS)

rta_dag_fed_li.o:SchedTest/partitioned/DAG/rta_dag_fed_li.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/DAG/rta_dag_fed_li.cpp $(LIBS)

rta_dag_sfed_xu.o:SchedTest/partitioned/DAG/rta_dag_sfed_xu.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/DAG/rta_dag_sfed_xu.cpp $(LIBS)

rta_dag_cgfp.o:SchedTest/global/DAG/rta_dag_cgfp.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/DAG/rta_dag_cgfp.cpp $(LIBS)

rta_dag_cgedf.o:SchedTest/global/DAG/rta_dag_cgedf.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/DAG/rta_dag_cgedf.cpp $(LIBS)

rta_dag_gfp_mel.o:SchedTest/global/DAG/rta_dag_gfp_mel.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/DAG/rta_dag_gfp_mel.cpp $(LIBS)

rta_dag_gedf_mel.o:SchedTest/global/DAG/rta_dag_gedf_mel.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/DAG/rta_dag_gedf_mel.cpp $(LIBS)

rta_dag_gedf_hoo.o:SchedTest/global/DAG/rta_dag_gedf_hoo.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/DAG/rta_dag_gedf_hoo.cpp $(LIBS)

cab_dag_gfp_li.o:SchedTest/global/DAG/cab_dag_gfp_li.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/DAG/cab_dag_gfp_li.cpp $(LIBS)

cab_dag_gedf_li.o:SchedTest/global/DAG/cab_dag_gedf_li.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/DAG/cab_dag_gedf_li.cpp $(LIBS)

rta_dag_fed_dpcp.o:SchedTest/partitioned/DAG/rta_dag_fed_dpcp.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/DAG/rta_dag_fed_dpcp.cpp $(LIBS)

rta_dag_gfp_jose.o:SchedTest/global/DAG/rta_dag_gfp_jose.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/DAG/rta_dag_gfp_jose.cpp $(LIBS)

ilp_rta_fed_h2lp.o:SchedTest/partitioned/lp/ilp_rta_fed_h2lp.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/lp/ilp_rta_fed_h2lp.cpp $(LIBS)

lp_rta_gfp_pfmlp.o:SchedTest/global/lp/lp_rta_gfp_pfmlp.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/lp/lp_rta_gfp_pfmlp.cpp $(LIBS)

lp_rta_gfp_ppip.o:SchedTest/global/lp/lp_rta_gfp_ppip.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/lp/lp_rta_gfp_ppip.cpp $(LIBS)

ilp_rta_fed_h2lp_v2.o:SchedTest/partitioned/lp/ilp_rta_fed_h2lp_v2.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/lp/ilp_rta_fed_h2lp_v2.cpp $(LIBS)

ilp_rta_fed_h2lp_v3.o:SchedTest/partitioned/lp/ilp_rta_fed_h2lp_v3.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/lp/ilp_rta_fed_h2lp_v3.cpp $(LIBS)

ilp_rta_fed_h2lp_v4.o:SchedTest/partitioned/lp/ilp_rta_fed_h2lp_v4.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/lp/ilp_rta_fed_h2lp_v4.cpp $(LIBS)

lp_rta_fed_spin.o:SchedTest/partitioned/lp/lp_rta_fed_spin.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/lp/lp_rta_fed_spin.cpp $(LIBS)

lp_rta_gfp_spin.o:SchedTest/global/lp/lp_rta_gfp_spin.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/lp/lp_rta_gfp_spin.cpp $(LIBS)

lp_rta_gfp_spin_xu.o:SchedTest/global/lp/lp_rta_gfp_spin_xu.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/global/lp/lp_rta_gfp_spin_xu.cpp $(LIBS)

lp_rta_pfp_gpu.o:SchedTest/partitioned/lp/lp_rta_pfp_gpu.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/lp/lp_rta_pfp_gpu.cpp $(LIBS)

ilp_rta_fed_mpcp.o:SchedTest/partitioned/lp/ilp_rta_fed_mpcp.cpp
	$(CXX) $(CXXFLAGS) $(INCPATH) -c SchedTest/partitioned/lp/ilp_rta_fed_mpcp.cpp $(LIBS)



clean:
	rm -f *.o ${ALL}

all:
	make clean
	make
