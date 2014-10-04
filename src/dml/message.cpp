#include "message.h"

message::message()
{
}
float * message::get_ptr()
{
  return data_ptr;
}

void message::evict()
{
  //smp_idx is used to evict the message whose sending is controled manually
  //to erase the map_pair in map_smpidx_slotidxlist
  buf->update_queue_slotidx_ready_push(slot_idx, smp_idx);
}
void message::commit()
{
  //when the message is ready, put it into map_smpidx_slotidxlist
  buf->post_processing_after_write(slot_idx);

}

float message::get_data(int i)
{
  return data_ptr[i];
}
void message::set_data(int i,float v)
{
  //*(data_ptr+i)=v;
  //buf->buffer_region[slot_idx][i+2]=v;
  buf->buffer_update(slot_idx, i+2, v);
}
int message::get_smp_idx()
{
  return smp_idx;
}
void message::set_smp_idx(int idx)
{
  *(data_ptr-2)=idx;
  smp_idx=idx;
}
int message::get_ptt_idx()
{
  return ptt_idx;
}
void message::set_ptt_idx(int idx)
{
  *(data_ptr-1)=idx;
  ptt_idx=idx;
}

int message::get_data_len()
{
  return data_len;
}

message::message(int smp_idx,int ptt_idx,int data_len, float * data_ptr, buffer *buf, int slot_idx)
{
  this->smp_idx=smp_idx;
  this->ptt_idx=ptt_idx;
  this->data_len=data_len;
  this->data_ptr=data_ptr;
  this->buf=buf;
  this->slot_idx=slot_idx;
}