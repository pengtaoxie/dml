
#include "dml.h"
#include "message.h"
#include <algorithm>
#include <unistd.h>
#include <iostream>

void dml::update_weight_remote()
{
  int * idx_perm_arr=new int[dst_feat_dim];
  for(int i=0;i<dst_feat_dim;i++)
    idx_perm_arr[i]=dst_feat_dim-1-i;


  while(true)
  {
    
    std::vector<message> msg_group=comm->read_msg_grp(0);
    float * mptr=msg_group[0].get_ptr();
    //update weight
    {
      #pragma omp parallel for
      for(int i=0;i<dst_feat_dim;i++)
      {
        int rnd_idx=idx_perm_arr[i];

        std::lock_guard<std::mutex> lock(weight_row_mutexes[rnd_idx]);
        for(int j=0;j<src_feat_dim;j++)
        {
          L[rnd_idx][j]=mptr[rnd_idx*src_feat_dim+j];
	 }
      }
    }
    for(int i=0;i<msg_group.size();i++)
      msg_group[i].evict();

  }

}

void dml::update_para(float ** grad, int grad_version, float learn_rate)
{
#pragma omp parallel for
for(int i=0;i<dst_feat_dim;i++)
	{
		if(independent==0)
		  std::lock_guard<std::mutex> lock(weight_row_mutexes[i]);
		for(int j=0;j<src_feat_dim;j++)
		{
			grad[i][j]/=(size_mb*2);
			L[i][j]-=learn_rate*grad[i][j];
		
		}
	}

  if(independent==1)
         return;
	message m;
	float * mptr;
       int status=-1;
	m=comm->req_write_msg(1, 1, status);//1 is the write buffer

       m.set_smp_idx(grad_version+rank*1e+7);
	mptr=m.get_ptr();

	#pragma omp parallel for
	for(int i=0;i<dst_feat_dim;i++)
	{
		for(int j=0;j<src_feat_dim;j++)
		{

		  m.data_ptr[i*src_feat_dim+j]=grad[i][j];	
			
		}
	}
       m.set_data(dst_feat_dim*src_feat_dim, learn_rate);
       

	m.commit();
}

void dml::update(float *x, float * y,int simi_update,float learn_rate, int dataidx, float ** grad)
{
	vec_sub(x, y, vec_buf_1, src_feat_dim);
	#pragma omp parallel for
       for(int i=0;i<dst_feat_dim;i++){
	    if(independent==0)
	    std::lock_guard<std::mutex> lock(weight_row_mutexes[i]);
	    float sum=0;
	    for(int j=0;j<src_feat_dim;j++){
		sum+=L[i][j]*vec_buf_1[j];
	    }
	    vec_buf_2[i]=sum;
	}

	if(simi_update==0)//dis similar pair
	{
		float dis=vec_sqr(vec_buf_2,dst_feat_dim);
		if(dis>thre)
		{
			return;
		}
	}
	#pragma omp parallel for
       for(int i=0;i<dst_feat_dim;i++)
	{

		for(int j=0;j<src_feat_dim;j++)
		{
			float gd;
			if(simi_update==1)//update similar pairs
			    gd=2*vec_buf_2[i]*vec_buf_1[j];
			else
			    gd=(-2)*vec_buf_2[i]*vec_buf_1[j];
			grad[i][j]+=gd;			
		}
	}
       

}
void dml::learn(float learn_rate, int epochs, int num_iters_evaluate)
{
  float time_begin=0, time_len=0;

  float ** grad=new float*[dst_feat_dim];
  for(int i=0;i<dst_feat_dim;i++){
    grad[i]=new float[src_feat_dim];
    memset(grad[i], sizeof(float)* src_feat_dim,0);
  }

  sleep(rank*5);

  std::vector<int> idx_perm_simi_pairs;
  for(int i=0;i<num_simi_pairs;i++)
    idx_perm_simi_pairs.push_back(i);
  std::random_shuffle ( idx_perm_simi_pairs.begin(), idx_perm_simi_pairs.end(), myrandom2);
  int * idx_perm_arr_simi_pairs=new int[num_simi_pairs];
  for(int i=0;i<num_simi_pairs;i++)
    idx_perm_arr_simi_pairs[i]=idx_perm_simi_pairs[i];

  std::cout<<"machine "<<rank<<std::endl;
  for(int i=0;i<10;i++)
    std::cout<<idx_perm_arr_simi_pairs[i]<<" ";
  std::cout<<std::endl;

  std::vector<int> idx_perm_diff_pairs;
  for(int i=0;i<num_diff_pairs;i++)
    idx_perm_diff_pairs.push_back(i);
  sleep(1);
  std::random_shuffle ( idx_perm_diff_pairs.begin(), idx_perm_diff_pairs.end(), myrandom2);
  int * idx_perm_arr_diff_pairs=new int[num_diff_pairs];
  for(int i=0;i<num_diff_pairs;i++)
    idx_perm_arr_diff_pairs[i]=idx_perm_diff_pairs[i];
  for(int i=0;i<10;i++)
    std::cout<<idx_perm_arr_diff_pairs[i]<<" ";
  std::cout<<std::endl;
  int smpcnt=0;
  int grad_version=0;
  for(int e=0;e<epochs;e++)
  {
    
    //compute gradient and update parameter
    int i=0;
	time_begin=get_time();
	for(i=0;i<num_simi_pairs;i++)
	{
		
	       

		int idx;
 		idx=idx_perm_arr_simi_pairs[i];
		if(!(simi_pairs[idx].x>=num_total_pts||simi_pairs[idx].y>=num_total_pts))
		  update(data[simi_pairs[idx].x], data[simi_pairs[idx].y],1,learn_rate, idx, grad);

		//pick a disimi pair
		idx=idx_perm_arr_diff_pairs[i];
		if(!(diff_pairs[idx].x>=num_total_pts||diff_pairs[idx].y>=num_total_pts))
		  update(data[diff_pairs[idx].x], data[diff_pairs[idx].y],0, learn_rate, idx, grad);

		smpcnt++;

		if(smpcnt%size_mb==0){
			
			update_para(grad, grad_version, learn_rate);
 			grad_version++;
			for(int gi=0;gi<dst_feat_dim;gi++)
			  memset(grad[gi], sizeof(float)*src_feat_dim, 0);
			/*if(false&&grad_version%num_iters_evaluate==0){

			time_len=get_time()-time_begin;
		       //evaluate
    			float simi_loss, diff_loss, total_loss;
    			evaluate(simi_loss, diff_loss, total_loss);
    			//std::cout<<"epoch:\t"<<e<<"\tsimi_loss:\t"<<simi_loss<<"\tdiff_loss:\t"<<diff_loss<<"\ttotal_loss:\t"<<total_loss<<std::endl;
    			std::cout<<"machine: "<<rank<<" iters: "<<grad_version<<" loss: "<<total_loss<<" time: "<<time_len<<std::endl;
			time_begin=get_time();
		      }*/
		
		}
              
	       

	}
	/*for(;i<num_diff_pairs;i++)
	{
	       int idx;
 		idx=idx_perm_arr_diff_pairs[i];
		update(data[diff_pairs[idx].x], data[diff_pairs[idx].y],0, learn_rate,idx,grad);
	}*/
       

  }
  for(int i=0;i<dst_feat_dim;i++)
    delete[] grad[i];
  delete[]grad;
}

void dml::load_data(int num_total_data, int num_simi_pairs, int num_diff_pairs, \
        char * data_file, char* simi_pair_file, char* diff_pair_file, int dense, int pair_bin)
{
  this->num_total_pts=num_total_data;
  this->num_simi_pairs=num_simi_pairs;
  this->num_diff_pairs=num_diff_pairs;
  data=new float*[num_total_data];
  for(int i=0;i<num_total_data;i++)
  {
	  data[i]=new float[src_feat_dim];
	  memset(data[i],0, sizeof(float)*src_feat_dim);
  }
  //load data, sparse format
  if(dense==0)
    load_sparse_data(data,num_total_data, data_file);
  else
    load_dense_data(data, num_total_data, src_feat_dim, data_file);
  //load simi pairs
  simi_pairs=new pair[num_simi_pairs];
  if(pair_bin==1)
	load_pairs_bin(simi_pairs, num_simi_pairs, simi_pair_file);
  else
  	load_pairs(simi_pairs,num_simi_pairs, simi_pair_file);
  //load diff pairs
  diff_pairs=new pair[num_diff_pairs];
  if(pair_bin==1)
	load_pairs_bin(diff_pairs, num_diff_pairs, diff_pair_file);
  else
  	load_pairs(diff_pairs,num_diff_pairs, diff_pair_file);

  /*for(int i=0;i<5;i++)
    std::cout<<simi_pairs[i].x<<"\t"<<simi_pairs[i].y<<std::endl;
  std::cout<<std::endl;
  for(int i=num_simi_pairs-5;i<num_simi_pairs;i++)
    std::cout<<simi_pairs[i].x<<"\t"<<simi_pairs[i].y<<std::endl;

  std::cout<<std::endl;
  for(int i=0;i<5;i++)
    std::cout<<diff_pairs[i].x<<"\t"<<diff_pairs[i].y<<std::endl;
  std::cout<<std::endl;
  for(int i=num_diff_pairs-5;i<num_diff_pairs;i++)
    std::cout<<diff_pairs[i].x<<"\t"<<diff_pairs[i].y<<std::endl;*/

  /*for(int i=0;i<5;i++){
    for(int j=0;j<5;j++)
      std::cout<<data[i][j]<<" ";
    std::cout<<std::endl;
  }

  for(int i=num_total_pts-5;i<num_total_pts;i++){
    for(int j=src_feat_dim-5;j<src_feat_dim;j++)
      std::cout<<data[i][j]<<" ";
    std::cout<<std::endl;
  }*/

}



float dml::pair_dis(pair p, float **data_ptr)
{
  vec_sub(data_ptr[p.x],data_ptr[p.y],vec_buf_1, src_feat_dim);
  mat_vec_mul(L, vec_buf_1, vec_buf_2, dst_feat_dim, src_feat_dim);
  return vec_sqr(vec_buf_2,dst_feat_dim);
}


float dml::pair_dis2(pair p, float **data_ptr, float ** paras)
{
  vec_sub(data_ptr[p.x],data_ptr[p.y],vec_buf_1, src_feat_dim);
  mat_vec_mul(paras, vec_buf_1, vec_buf_2, dst_feat_dim, src_feat_dim);
  return vec_sqr(vec_buf_2,dst_feat_dim);
}

void dml::evaluate(float & simi_loss, float & diff_loss, float & total_loss)
{
  //simi_loss=0;
  //diff_loss=0;
  //total_loss=0;
  ////traverse all simi pairs
  //for(int i=0;i<num_eval_simi_pairs;i++)
  //{
  //  simi_loss+=pair_dis(eval_simi_pairs[i], eval_data);
  //}
  //simi_loss/=num_eval_simi_pairs;
  ////traverse all diff pairs
  //for(int i=0;i<num_eval_diff_pairs;i++)
  //{
  //  float dis=pair_dis(eval_diff_pairs[i], eval_data);
  //  if(dis<thre)
  //    diff_loss+=(thre-dis);
  //}
  //diff_loss/=num_eval_diff_pairs;
  //diff_loss*=lambda;
  //total_loss=simi_loss+diff_loss;

  //take a snapshot
  float ** paras_tmp=new float*[dst_feat_dim];
  for(int i=0;i<dst_feat_dim;i++){
    paras_tmp[i]=new float[src_feat_dim];
  }	
  #pragma omp parallel for
  for(int i=0;i<dst_feat_dim;i++){
    std::lock_guard<std::mutex> lock(weight_row_mutexes[i]);
    for(int j=0;j<src_feat_dim;j++){
	paras_tmp[i][j]=L[i][j];
    }
  }

  simi_loss=0;
  diff_loss=0;
  total_loss=0;
  //traverse all simi pairs
  int simicnt=0;
  for(int i=0;i<num_simi_pairs;i+=step_evaluate)
  {
    if(simi_pairs[i].x>=num_total_pts||simi_pairs[i].y>=num_total_pts)
	continue;
    simi_loss+=pair_dis2(simi_pairs[i], data, paras_tmp);
    simicnt++;
  }
  simi_loss/=simicnt;
  //traverse all diff pairs
  int diffcnt=0;
  for(int i=0;i<num_diff_pairs;i+=step_evaluate)
  {
    if(diff_pairs[i].x>=num_total_pts||diff_pairs[i].y>=num_total_pts)
	continue;
    float dis=pair_dis2(diff_pairs[i], data, paras_tmp);
    if(dis<thre)
      diff_loss+=(thre-dis);
    diffcnt++;
  }
  diff_loss/=diffcnt;
  diff_loss*=lambda;
  total_loss=simi_loss+diff_loss;
  
  for(int i=0;i<dst_feat_dim;i++)
    delete[] paras_tmp[i];
  delete[]paras_tmp;

}



void dml::load_eval_data(int num_total_eval_data, int num_eval_simi_pairs, int num_eval_diff_pairs, char * eval_data_file, char* eval_simi_pair_file, char* eval_diff_pair_file)
{
  this->num_total_eval_pts=num_total_eval_data;
  this->num_eval_simi_pairs=num_eval_simi_pairs;
  this->num_eval_diff_pairs=num_eval_diff_pairs;

  eval_data=new float*[num_total_eval_data];
  for(int i=0;i<num_total_eval_data;i++)
  {
	  eval_data[i]=new float[src_feat_dim];
	  memset(eval_data[i], 0, sizeof(float)*src_feat_dim);
  }
  //load data, sparse format
  load_sparse_data(eval_data,num_total_eval_data, eval_data_file);
  //load simi pairs
  eval_simi_pairs=new pair[num_eval_simi_pairs];
  load_pairs(eval_simi_pairs,num_eval_simi_pairs, eval_simi_pair_file);
  //load diff pairs
  eval_diff_pairs=new pair[num_eval_diff_pairs];
  load_pairs(eval_diff_pairs,num_eval_diff_pairs, eval_diff_pair_file);

}

void dml::communicate()
{
  comm->communicate();
}

buf_para_grp dml::config_buf(int rank, int buf_capacity)
{
  buf_para_grp bpg;
  //input buf
  buf_para inbuf;
  inbuf.dim=dst_feat_dim*src_feat_dim+2;
  inbuf.capacity=buf_capacity;
  inbuf.comm_type=RECV_COM;
  inbuf.push_type=RECV;
  inbuf.pop_type=READ;
  inbuf.auto_release_after_send=false;
  inbuf.num_submsgs_foreach_smp=1;

  inbuf.machines.push_back(0);
  bpg.push_back(inbuf);

  //outbuf
  buf_para outbuf;
  outbuf.dim=dst_feat_dim*src_feat_dim+1+2;//learning rate
  outbuf.capacity=buf_capacity;
  outbuf.comm_type=SEND_COM;
  outbuf.push_type=WRITE;
  outbuf.pop_type=SEND;
  outbuf.auto_release_after_send=true;
  outbuf.num_submsgs_foreach_smp=1;

  outbuf.machines.push_back(0);
  bpg.push_back(outbuf);
  
  return bpg;
} 


dml::dml(int src_feat_dim, int dst_feat_dim, float lambda,  float thre,int rank, int buf_capacity, \ 
   int independent, int step_evaluate, int size_mb)
{
  this->src_feat_dim=src_feat_dim;
  this->dst_feat_dim=dst_feat_dim;
  this->lambda=lambda;
  this->thre=thre;
  L=new float*[dst_feat_dim];
  for(int i=0;i<dst_feat_dim;i++)
    L[i]=new float[src_feat_dim];
  //random initialize parameters
  std::srand(777);
  for(int i=0;i<dst_feat_dim;i++)
  {
	 for(int j=0;j<src_feat_dim;j++)
	 {
		 L[i][j]=randfloat();
	 }
  }
  vec_buf_1=new float[src_feat_dim];
  vec_buf_2=new float[dst_feat_dim];

  buf_para_grp bpg=config_buf(rank, buf_capacity);
  comm=new communicator(bpg,rank);
  weight_row_mutexes=new std::mutex[dst_feat_dim];
  this->rank=rank;
  this->independent=independent;
  this->step_evaluate=step_evaluate;
  this->size_mb=size_mb;
}

dml::~dml()
{
  delete[]simi_pairs;
  delete[]diff_pairs;
  for(int i=0;i<num_total_pts;i++)
    delete[]data[i];
  delete[]data;

  for(int i=0;i<dst_feat_dim;i++)
    delete[]L[i];
  delete []L;
}