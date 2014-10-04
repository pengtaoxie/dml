#ifndef CHANNEL_H_
#define CHANNEL_H_

#include "buffer.h"
#include <vector>
class channel
{
private:
  buffer *buf;// ptr to the bufffer
  int comm_type;// 0 recv ;  1 send
  std::vector<int> machine_list;
public: 
  channel(int buf_capacity, int dim, int comm_type, int push_type, int pop_type, bool auto_release_after_send, \
           std::vector<int> machine_list,int num_submsgs_foreach_smp,int rank);
  ~channel();  
  buffer * get_buf();
  int get_comm_type();
  std::vector<int> get_machine_list();
  int rank;
};


#endif


