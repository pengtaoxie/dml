#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "buffer.h"
class buffer;
class message
{
private:
  //point to data region
  
  //sample index
  int smp_idx;
  //partition index
  int ptt_idx;  
  //length of the data
  int data_len;
  //which buffer the message is in
  
  //which slot the message is in
  
public:
  buffer * buf;
  int slot_idx;

  message();
  message(int smp_idx,int ptt_idx,int data_len, float * data_ptr, buffer * buf, int slot_idx);
  float get_data(int i);
  void set_data(int i,float v);
  int get_smp_idx();
  void set_smp_idx(int idx);
  int get_ptt_idx();
  void set_ptt_idx(int idx);
  int get_data_len();
  //after operation(recv or computation) on this message, commit it
  void commit();
  //evict the message from buffer
  void evict();
  float * get_ptr();
  float * data_ptr;
};



#endif


