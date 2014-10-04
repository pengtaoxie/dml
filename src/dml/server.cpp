#include "server.h"
#include "utils.h"
#include <omp.h>
#include <iostream>
#include <algorithm>
#include <unistd.h>


float pair_dis(pair p, float **data_ptr, float ** paras, int num_rows, int num_cols, float * vec_buf_1, float* vec_buf_2)
{
  vec_sub(data_ptr[p.x],data_ptr[p.y],vec_buf_1, num_cols);
  mat_vec_mul(paras, vec_buf_1, vec_buf_2, num_rows, num_cols);
  return vec_sqr(vec_buf_2,num_rows);
}
void server::evaluate_obj(pair * simi_pairs, pair * diff_pairs, int num_simi_pairs, int num_diff_pairs, int step_evaluate, \
      int num_total_pts, float ** data, float thre, float lambda, int freq_eval)
{

  //take a snapshot
  float ** paras_tmp=new float*[num_rows];
  for(int i=0;i<num_rows;i++){
    paras_tmp[i]=new float[num_cols];
  }
  float * vec_buf_1=new float[num_cols];
  float * vec_buf_2=new float[num_rows];
  

  while(true)
  {
	sleep(1);
	int update_cnter_snap=update_cnter;
       int time_snap=cur_time;
	if(!(update_cnter_snap>=1&&update_cnter_snap%freq_eval==0))
        continue;
  
  

  #pragma omp parallel for
  for(int i=0;i<num_rows;i++){
    std::lock_guard<std::mutex> lock(weight_row_mutexes[i]);
    for(int j=0;j<num_cols;j++){
	paras_tmp[i][j]=mat[i][j];
    }
  }

  float simi_loss=0;
  float diff_loss=0;
  float total_loss=0;
  //traverse all simi pairs
  int simicnt=0;
  for(int i=0;i<num_simi_pairs;i+=step_evaluate)
  {
    if(simi_pairs[i].x>=num_total_pts||simi_pairs[i].y>=num_total_pts)
	continue;
    simi_loss+=pair_dis(simi_pairs[i], data, paras_tmp,num_rows, num_cols, vec_buf_1, vec_buf_2);
    simicnt++;
  }
  simi_loss/=simicnt;
  //traverse all diff pairs
  int diffcnt=0;
  for(int i=0;i<num_diff_pairs;i+=step_evaluate)
  {
    if(diff_pairs[i].x>=num_total_pts||diff_pairs[i].y>=num_total_pts)
	continue;
    float dis=pair_dis(diff_pairs[i], data, paras_tmp, num_rows, num_cols, vec_buf_1, vec_buf_2);
    if(dis<thre)
      diff_loss+=(thre-dis);
    diffcnt++;
  }
  diff_loss/=diffcnt;
  diff_loss*=lambda;
  total_loss=simi_loss+diff_loss;
  std::cout<<" iters: "<<update_cnter_snap<<" loss: "<<total_loss<<" time: "<<time_snap<<std::endl;


  }
delete[]vec_buf_1;
delete[]vec_buf_2;



for(int i=0;i<num_rows;i++)
    delete[] paras_tmp[i];
  delete[]paras_tmp;
}

server::server(int num_rows, int num_cols, int rank, int nmachines, int buf_capacity, int num_iters_send)
{
  
  this->num_rows=num_rows;
  this->num_cols=num_cols;
  this->rank=rank;

  this->num_iters_send=num_iters_send;
  buf_para_grp bpg=config_buf(rank, nmachines, buf_capacity);
  comm=new communicator(bpg,rank);
  std::srand(777);
  mat=new float*[num_rows];
  for(int i=0;i<num_rows;i++)
  {
    mat[i]=new float[num_cols];
    for(int j=0;j<num_cols;j++)
	mat[i][j]=randfloat();
  }
  update_cnter=0;
  weight_row_mutexes=new std::mutex[num_rows];
  cur_time=0;
}


buf_para_grp server::config_buf(int rank, int nmachines, int buf_capacity)
{
  buf_para_grp bpg;
  //input buf
  buf_para inbuf;
  inbuf.dim=num_rows*num_cols+1+2;//the additional value is for learning rate
  inbuf.capacity=buf_capacity;
  inbuf.comm_type=RECV_COM;
  inbuf.push_type=RECV;
  inbuf.pop_type=READ;
  inbuf.auto_release_after_send=false;
  inbuf.num_submsgs_foreach_smp=1;
  for(int i=1;i<nmachines;i++)
      inbuf.machines.push_back(i);
  bpg.push_back(inbuf);

  //outbuf
  buf_para outbuf;
  outbuf.dim=num_rows*num_cols+2;
  outbuf.capacity=buf_capacity;
  outbuf.comm_type=SEND_COM;
  outbuf.push_type=WRITE;
  outbuf.pop_type=SEND;
  outbuf.auto_release_after_send=true;
  outbuf.num_submsgs_foreach_smp=1;
  for(int i=1;i<nmachines;i++)
      outbuf.machines.push_back(i);
  bpg.push_back(outbuf);
  
  return bpg;
} 

void server::update_weight_remote()
{
  int version=0;
  int num_update=0;
  int stime=(int)get_time();
  while(true)
  {

    message m=(comm->read_msg_grp(0))[0];
    float * data_ptr=m.get_ptr();

    float lr=m.get_data(num_rows*num_cols);

    for(int i=0;i<num_rows;i++){
      std::lock_guard<std::mutex> lock(weight_row_mutexes[i]);
      for(int j=0;j<num_cols;j++){
        

	 mat[i][j]-=lr*data_ptr[i*num_cols+j];

      }
    }
    

    m.evict();
    num_update++;
    update_cnter++;
    
    cur_time=cur_time+(int)get_time()-stime;
    stime=(int)get_time();

    
    if(num_update%num_iters_send!=0)
      continue;
    int status=-1;
    message m1=comm->req_write_msg(1, 1, status);//1 is the write buffer
    m1.set_smp_idx(version);
    float * dt_ptr=m1.get_ptr();
    int idx=0;
    for(int i=0;i<num_rows;i++){
      for(int j=0;j<num_cols;j++){

	 dt_ptr[idx]=mat[i][j];
	 idx++;
      }
    }
    m1.commit();
    version++;
  }
}
void server::communicate()
{
  comm->communicate();
}



server::~server()
{
  for(int i=0;i<num_rows;i++)
    delete[]mat[i];
  delete []mat;  
}

