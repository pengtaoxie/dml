#ifndef BUF_COMM_PARA_H_
#define BUF_COMM_PARA_H_

#include <vector>

enum com_type {RECV_COM, SEND_COM};
enum ps_type {RECV, WRITE};
enum pp_type {SEND,READ};
 
struct buf_para
{
  int dim;
  int capacity; 
  com_type comm_type;
  ps_type push_type;
  pp_type pop_type;
  bool auto_release_after_send;
  int num_submsgs_foreach_smp;
  std::vector<int> machines;
};

typedef std::vector<buf_para> buf_para_grp;

#endif