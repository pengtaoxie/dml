
#include "channel.h"

channel::channel(int buf_capacity,int dim, int comm_type, int push_type, int pop_type, bool auto_release_after_send, std::vector<int> machine_list, \
  int num_submsgs_foreach_smp,int rank)
{
  this->comm_type=comm_type;
  this->machine_list=machine_list;
  buf=new buffer(buf_capacity, dim, push_type, pop_type, auto_release_after_send,num_submsgs_foreach_smp,rank, machine_list);
  this->rank=rank;
}
channel::~channel()
{
  delete[] buf;
}
buffer * channel::get_buf()
{
  return buf;
}
int channel::get_comm_type()
{
  return comm_type;
}
std::vector<int> channel::get_machine_list()
{
  return machine_list;
}