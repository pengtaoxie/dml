

#include <fstream>
#include <string>
#include "dml_paras.h"
#include <iostream>

void load_dml_paras(dml_paras & para, const char * file_dml_para)
{
  std::ifstream infile;
  infile.open(file_dml_para);
  std::string tmp;
  infile>>tmp>>para.src_feat_dim;
  infile>>tmp>>para.dst_feat_dim;
  infile>>tmp>>para.lambda;
  infile>>tmp>>para.thre;
  infile>>tmp>>para.learn_rate;
  infile>>tmp>>para.epoch;
  infile>>tmp>>para.num_total_pts;
  infile>>tmp>>para.num_simi_pairs;
  infile>>tmp>>para.num_diff_pairs;
  infile>>tmp>>para.num_total_eval_pts;
  infile>>tmp>>para.num_eval_simi_pairs;
  infile>>tmp>>para.num_eval_diff_pairs;
  infile>>tmp>>para.data_file;
  infile>>tmp>>para.simi_pair_file;
  infile>>tmp>>para.diff_pair_file;
  infile>>tmp>>para.eval_data_file;
  infile>>tmp>>para.eval_simi_pair_file;
  infile>>tmp>>para.eval_diff_pair_file;
  infile>>tmp>>para.buf_capacity;
  infile>>tmp>>para.num_iters_send;
  infile>>tmp>>para.independent;
  infile>>tmp>>para.num_iters_evaluate;
  infile>>tmp>>para.step_evaluate;
  infile>>tmp>>para.size_mb;
  infile>>tmp>>para.dense;
  infile>>tmp>>para.pair_bin;

  infile.close();

  //print the parameters to let users check
  //print_dml_paras(para);
}
void print_dml_paras(dml_paras para)
{
  std::cout<<"src feat dim: "<<para.src_feat_dim<<std::endl;
  std::cout<<"dst feat dim: "<<para.dst_feat_dim<<std::endl;
  std::cout<<"lambda: "<<para.lambda<<std::endl;
  std::cout<<"thre: "<<para.thre<<std::endl;
  std::cout<<"learn_rate: "<<para.learn_rate<<std::endl;
  std::cout<<"epoch: "<<para.epoch<<std::endl;
  std::cout<<"num_total_pts: "<<para.num_total_pts<<std::endl;
  std::cout<<"num_simi_pairs: "<<para.num_simi_pairs<<std::endl;
  std::cout<<"num_diff_pairs: "<<para.num_diff_pairs<<std::endl;
  std::cout<<"num_total_eval_pairs: "<<para.num_total_eval_pts<<std::endl;
  std::cout<<"num_eval_simi_pairs: "<<para.num_eval_simi_pairs<<std::endl;
  std::cout<<"num_eval_diff_pairs: "<<para.num_eval_diff_pairs<<std::endl;
  std::cout<<"data_file: "<<para.data_file<<std::endl;
  std::cout<<"simi_pair_file: "<<para.simi_pair_file<<std::endl;
  std::cout<<"diff_pair_file: "<<para.diff_pair_file<<std::endl;
  std::cout<<"eval_data_file: "<<para.eval_data_file<<std::endl;
  std::cout<<"eval_simi_pair_file: "<<para.eval_simi_pair_file<<std::endl;
  std::cout<<"eval_diff_pair_file: "<<para.eval_diff_pair_file<<std::endl;
  std::cout<<"buf_capacity: "<<para.buf_capacity<<std::endl;
  std::cout<<"num_iters_send: "<<para.num_iters_send<<std::endl;
  std::cout<<"independent: "<<para.independent<<std::endl;
  std::cout<<"num_iters_evaluate: "<<para.num_iters_evaluate<<std::endl;
  std::cout<<"step_evaluate: "<<para.step_evaluate<<std::endl;
  std::cout<<"size_mb: "<<para.size_mb<<std::endl;
  std::cout<<"dense: "<<para.dense<<std::endl;
  std::cout<<"pair_bin: "<<para.pair_bin<<std::endl;
  
}














