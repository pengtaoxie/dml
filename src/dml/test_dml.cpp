#include "test_dml.h"
#include "mpi.h"
#include "utils.h"
#include "dml.h"
#include "dml_paras.h"
#include <thread>
#include <vector>
#include <fstream>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include "server.h"





int test_dml(int argc, char* argv[])
{

  if(argc!=2){
    std::cout<<"usage: <para_file>"<<std::endl;
    return 0;
  }
  //mpi configure
  MPI::Init(argc,argv);
  
  int rank=MPI::COMM_WORLD.Get_rank();
  int nmachines=MPI::COMM_WORLD.Get_size();
  
  //configure meta
  dml_paras dmlprs;
  load_dml_paras(dmlprs, argv[1]);
  if(rank==0)
    print_dml_paras(dmlprs);

  if(rank==0){//server
   if(dmlprs.independent==0){
    
float **data=new float*[dmlprs.num_total_pts];
  for(int i=0;i<dmlprs.num_total_pts;i++)
  {
	  data[i]=new float[dmlprs.src_feat_dim];
	  memset(data[i],0, sizeof(float)*dmlprs.src_feat_dim);
  }
  //load data, sparse format
  if(dmlprs.dense==0)
    load_sparse_data(data,dmlprs.num_total_pts, dmlprs.data_file);
  else
    load_dense_data(data, dmlprs.num_total_pts, dmlprs.src_feat_dim, dmlprs.data_file);
  //load simi pairs
  pair * simi_pairs=new pair[dmlprs.num_simi_pairs];
  if(dmlprs.pair_bin==1)
	load_pairs_bin(simi_pairs, dmlprs.num_simi_pairs, dmlprs.simi_pair_file);
  else
  	load_pairs(simi_pairs,dmlprs.num_simi_pairs, dmlprs.simi_pair_file);
  //load diff pairs
  pair * diff_pairs=new pair[dmlprs.num_diff_pairs];
  if(dmlprs.pair_bin==1)
	load_pairs_bin(diff_pairs, dmlprs.num_diff_pairs, dmlprs.diff_pair_file);
  else
  	load_pairs(diff_pairs,dmlprs.num_diff_pairs, dmlprs.diff_pair_file);



    server myserver(dmlprs.dst_feat_dim, dmlprs.src_feat_dim, rank, nmachines, dmlprs.buf_capacity, dmlprs.num_iters_send);
    std::vector<std::thread> thread_pool;
    thread_pool.push_back(std::thread(&server::update_weight_remote,&myserver));
    thread_pool.push_back(std::thread(&server::communicate,&myserver));
    

    



    thread_pool.push_back(std::thread(&server::evaluate_obj, &myserver, simi_pairs, diff_pairs, dmlprs.num_simi_pairs, \
   dmlprs.num_diff_pairs, dmlprs.step_evaluate, \
      dmlprs.num_total_pts, data, dmlprs.thre, dmlprs.lambda, dmlprs.num_iters_evaluate));
    for(int i=0;i<thread_pool.size();i++)
      thread_pool[i].join();
   }

  }
  else{//client
	//load data
    dml mydml(dmlprs.src_feat_dim, dmlprs.dst_feat_dim, dmlprs.lambda, dmlprs.thre, \
           rank, dmlprs.buf_capacity, dmlprs.independent, dmlprs.step_evaluate, dmlprs.size_mb);
    mydml.load_data(dmlprs.num_total_pts,  dmlprs.num_simi_pairs, dmlprs.num_diff_pairs, \
                dmlprs.data_file, dmlprs.simi_pair_file,  dmlprs.diff_pair_file, dmlprs.dense, dmlprs.pair_bin);
    //mydml.load_eval_data(dmlprs.num_total_eval_pts, dmlprs.num_eval_simi_pairs, \
               //dmlprs.num_eval_diff_pairs, dmlprs.eval_data_file, dmlprs.eval_simi_pair_file, dmlprs.eval_diff_pair_file);
    std::vector<std::thread> thread_pool;
    if(dmlprs.independent==0){
      thread_pool.push_back(std::thread(&dml::update_weight_remote,&mydml));
      thread_pool.push_back(std::thread(&dml::communicate,&mydml));
    }
    thread_pool.push_back(std::thread(&dml::learn, &mydml, dmlprs.learn_rate, dmlprs.epoch, dmlprs.num_iters_evaluate));
    for(int i=0;i<thread_pool.size();i++)
      thread_pool[i].join();

  }

  //mpi release
  MPI::Finalize();
  return 0;
}




















