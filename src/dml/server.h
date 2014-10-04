#ifndef MLR_H_
#define MLR_H_
#include "communicator.h"
#include "buf_para.h"
#include "utils.h"
#include <mutex>
#include <atomic>
class server
{
private:
  int num_rows;
  int num_cols;
  float ** mat;
  int num_iters_send;
  communicator * comm;//communicator

  buf_para_grp config_buf(int rank, int nmachines, int buf_capacity);
  int rank;
  std::mutex * weight_row_mutexes;
  std::atomic<int> update_cnter;
  std::atomic<int> cur_time;
public:
  server(int num_rows, int num_cols, int rank, int nmachines, int buf_capacity, int num_iters_send);
  void update_weight_remote();
  void evaluate_obj(pair * simi_pairs, pair * diff_pairs, int num_simi_pairs, int num_diff_pairs, int step_evaluate, \
      int num_total_pts, float ** data, float thre, float lambda, int freq_eval);
  void communicate();
  ~server();
  
};


#endif