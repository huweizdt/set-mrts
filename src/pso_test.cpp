#include <iteration-helper.h>

#include <param.h>

#include <processors.h>

#include <random_gen.h>

#include <resources.h>

#include <tasks.h>

#include <rta_pso.h>





#include <vector>

#include <cstdlib>

#include <cstring>

#include <stdio.h>

#include <unistd.h>

#include <iostream>

#include <mysql/mysql.h>




using namespace std;





int main(int argc, char **argv) {

  

  MYSQL mysql;

  mysql_init(&mysql);

  MYSQL_RES *result,*result_cluster,*result_task,*result_ready;

  MYSQL_ROW row; 

  int cluster_num;
  int task_num;
  int cluster_judge;

  if(!mysql_real_connect(&mysql,"127.0.0.1", "root", "wygwan123", "mysql", 3306, NULL, CLIENT_FOUND_ROWS))

    {

        cout << mysql_error(&mysql) << endl;

        return -1;

    }

  const char* judge_sql = "select value from control where control_id = 1";
  const char* cluster_num_sql = "select value from control where control_id = 2";
  const char* task_num_sql = "select value from control where control_id = 3";
  const char* cluster_ready_sql = "select value from control where control_id = 4";

  const char * res ;

  while(1){

  mysql_query(&mysql,judge_sql);

  result = mysql_store_result(&mysql);  //获取结果集

  if (result)  // 返回了结果集

  {
      row=mysql_fetch_row(result);

      res = row[0];

  }
  else  // result==NULL
  {
    cout<<"Get result error: "<<mysql_error(&mysql);
          return false;
  }

  int signal = atoi(res);

  if(signal == 1){
      //get cluster_num
      mysql_query(&mysql,cluster_num_sql);
      result_cluster= mysql_store_result(&mysql);  //获取结果集
      if (result_cluster)  // 返回了结果集
      {
          row=mysql_fetch_row(result_cluster);
          res = row[0];
      }
      else  // result==NULL
      {
        cout<<"Get result error: "<<mysql_error(&mysql);
              return false;
      }
      cluster_num = atoi(res);

      printf(" cluster_num : %d\n",cluster_num);


      //get task_num
      mysql_query(&mysql,task_num_sql);
      result_task= mysql_store_result(&mysql);  //获取结果集
      if (result_task)  
      {
          row=mysql_fetch_row(result_task);
          res = row[0];
      }
      else  
      {
        cout<<"Get result error: "<<mysql_error(&mysql);
              return false;
      }
      task_num = atoi(res);   
      printf("tasknum: %d \n ",task_num);
    
      int turn = 0;


      while(turn < cluster_num){
        
            mysql_query(&mysql,cluster_ready_sql);

            result_ready = mysql_store_result(&mysql);  //获取结果集

            if (result_ready)  // 返回了结果集

            {
                row=mysql_fetch_row(result_ready);
                res = row[0];
            }
            else  // result==NULL
            {
              cout<<"Get result error: "<<mysql_error(&mysql);
                    return false;
            }
            cluster_judge = atoi(res);

            printf("ready : %d",cluster_judge);

            if(cluster_judge == 1){

                  const char* del_sql1 = "truncate table core;";

                  const char* del_sql2 = "truncate table task;";

                  mysql_query(&mysql, del_sql1);

                  mysql_query(&mysql, del_sql2);

                  vector<Param> parameters = get_parameters();         //获取参数

                  Param param = parameters[0];      //获取参数

                  BPTaskSet tasks = BPTaskSet();    //初始化任务集

                  ProcessorSet processors = ProcessorSet(param);    //根据参数设置初始化处理器集合

                  ResourceSet resources = ResourceSet();    //初始化资源集

                  resource_gen(&resources, param);  //根据参数设置生成资源集

                  tasks.task_gen(&resources, param, 1.2, task_num); //初始化主版本任务集

                  tasks.task_copy(tasks.get_taskset_size());    //初始化副版本任务集

                  cout<<"now t_num is"<<tasks.get_taskset_size()<<endl;

                  RTA_PSO *pso_test=new RTA_PSO(tasks, processors,resources);

                  int** solution_arry=pso_test->PSO(20,100); //pso启发式算法获得多组分配方案

                  tasks=pso_test->choose_solution(solution_arry,20);    //可调度行分析，回溯法确定jitter

                  // printf("tasks!!!!\n");
                  foreach(tasks.get_tasks(),ti){
                    int id = ti->get_id();

                    int wcet = (ti->get_wcet()) ;

                    int period = (ti->get_period()) ;

                    int deadline = (ti->get_deadline()) ;

                    int jitter = (ti->get_jitter());

                    int status = ti->get_status();

                    int partition = ti->get_partition();

                    char task_sql[80] ;

                    sprintf(task_sql, "insert into task values ('%d','%d','%d','%d','%d','%d','%d')", id,status,wcet,period,jitter,deadline,partition);

                    mysql_query(&mysql,task_sql);

                    printf("%s!!!!\n",task_sql);

                  }

                  

                  foreach(processors.get_processors(),ti){

                    int id = ti->get_processor_id();

                    double rate = ti ->get_speedfactor();

                    char task_sql[80] ;

                    sprintf(task_sql, "insert into core values ('%d','%lf','%d')", id,rate,20);

                    mysql_query(&mysql,task_sql);

                    printf("%s!!!!\n",task_sql);

                  }
                  turn++;
                  const char * change_sql1 = "UPDATE control SET value = -1 where control_id = 4";

                  mysql_query(&mysql,change_sql1);

                
              }
              else {
                printf("wait for turn %d",turn);
                sleep(2);
              }
            }
 



      // foreach(resources.get_resources(),ti){

        

      // }

      // for(int i=0;i<tasks.get_taskset_size();i++)

      // printf("%d \t",solution[i]);

      printf("okok\n");

      cout<<endl;

      const char * change_sql = "UPDATE control SET value = -1 where control_id = 1";

      mysql_query(&mysql,change_sql);

    }

    else {

      printf("not request   \n");

      sleep(2);

    }

  }

  return 0;

}



bool clear(){



}
