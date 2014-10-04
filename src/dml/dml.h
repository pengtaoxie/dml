
#include "utils.h"
#include <iostream>
#include "communicator.h"
#include "buf_para.h"
#include "aux_data.h"
class dml
{
private:
  int src_feat_dim;//original feature dimension
  int dst_feat_dim;//target feature dimension
  float ** L;//transformation matrix
  float lambda;//tradeoff parameter
  float thre;//distance threshold
  int rank;

  //data
  int num_total_pts;//number of total pts
  float ** data;//all the data points
  int num_simi_pairs;//number of similar pairs
  int num_diff_pairs;//num of dissimilar pairs
  pair * simi_pairs;//pairs labeled as similar
  pair * diff_pairs;//pairs labeled as dissimilar pairs

  //evaluation data
  int num_total_eval_pts;//number of total evaluation pts
  float ** eval_data;//all the evaluation data points
  int num_eval_simi_pairs;//number of evaluation similar pairs
  int num_eval_diff_pairs;//num of evaluation dissimilar pairs
  pair * eval_simi_pairs;//pairs labeled as similar for evaluation
  pair * eval_diff_pairs;//pairs labeled as dissimilar pairs for evaluation

  communicator * comm;//communicator
  

  buf_para_grp config_buf(int rank, int buf_capacity);
  
  std::mutex * weight_row_mutexes;
  int independent;
  int step_evaluate;
  int size_mb;

  float pair_dis(pair p, float **data_ptr);//the metric distance between a pair
  float pair_dis2(pair p, float **data_ptr, float ** paras);
  //tmp buffers
  float *vec_buf_1;//src_feat_dim
  float *vec_buf_2;//dst_feat_dim
  void update(float * x, float * y, int simi_update, float learn_rate,int data_idx, float ** grad);
  void evaluate(float & simi_loss, float & diff_loss, float & total_loss);

public:
  dml(int src_feat_dim, int dst_feat_dim, float lambda, float thre, int rank, int buf_capacity, int independent, \
  int size_mb, int step_evaluate);
  void load_data(int num_total_data, int num_simi_pairs, int num_diff_pairs, char * data_file, \ 
        char* simi_pair_file, char* diff_pair_file, int dense, int pair_bin);
  void load_eval_data(int num_total_eval_data, int num_eval_simi_pairs, int num_eval_diff_pairs, \ 
           char * eval_data_file, char* eval_simi_pair_file, char* eval_diff_pair_file);
  void learn(float learn_rate, int epochs, int num_iters_evaluate);
  void update_weight_remote();
  void update_para(float ** grad, int grad_version, float learn_rate);
  void communicate();
  ~dml();
};
