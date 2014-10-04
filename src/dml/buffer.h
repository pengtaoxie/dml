#ifndef BUFFER_H_
#define BUFFER_H_

#include <queue>
#include <unordered_map>
#include <mutex>
#include <vector>
#include "message.h"
#include "buf_para.h"
class message;

class buffer
{
private:
  int capacity;//capacity of the buffer
  int dim;//dimension of buffered vector
  
  //data structure to sync buffer
  int num_submsgs_foreach_smp;//number of submsgs for each smp
  
  std::mutex mutex_queue_slotidx_ready_push;//mutex
  
  std::mutex mutex_queue_smpidx_ready_process;//mutex
  std::unordered_map<int,std::vector<int>> map_smpidx_slotidxlist;//smpid -- the slot id of its submsgs
  std::mutex mutex_map_smpidx_slotidxlist;//mutex

  //storing send status
  int * send_done_status;
  std::mutex mutex_send_done_status;

  std::mutex mutex_buffer_region;

public:
  //for send
  int num_send_machines;
  std::unordered_map<int, std::queue<int>> machine_slottosend;
  std::mutex mutex_machine_slottosend;
  std::unordered_map<int, int> slotid_numfinishedmachines;
  std::mutex mutex_slotid_numfinishedmachines;

  std::unordered_map<int,int> machine_buffer_usage;
  std::mutex mutex_machine_buffer_usage;
  
  std::unordered_map<int,int> slotid_machineid;
  std::mutex mutex_slotid_machineid;


  int rank;
  int push_type;// 0 recv  1 compute
  int pop_type;// 0 send 1 compute
  bool auto_release_after_send;//whether to auto release after sending the message
  std::queue<int> queue_smpidx_ready_process;//storing the smp id which are ready to be processed
  std::queue<int> queue_slotidx_ready_push;//storing the slot id which are ready to push message in
  float ** buffer_region;//storing buffered data

  buffer(int capacity, int dim, int push_type, int pop_type, bool auto_release_after_send,int num_submsgs_foreach_smp,int rank, \
        std::vector<int> machines);
  ~buffer();
  bool get_auto_release_after_send();
  int get_dim();//get the dimension of the buffer
  std::vector<message> read_msg_group();//read message group from the buffer
  message req_msg_to_write(int blocking, int & status);//request a output buffer
  int req_push_slot();// request a slot to push message in
  float * get_slot(int slotid);// get a slot
  void update_queue_slotidx_ready_push(int slotid, int smpidx);
  void aggregrate_submsg(int slotid);
  void update_send_done_status(int slotid);
  message retrv_msg_bysmpid(int smpid);
  int get_slotids_for_submsgs(std::vector<int> & slotidlist);
  bool machine_valid_use(int machineid);
  void inc_machine_buffer_usage(int machineid);
  void insert_map_slotid_machineid(int slotid,int machineid);

  void buffer_update(int i,int j, float v);
  void post_processing_after_send(int slotid);
  void post_processing_after_write(int slotid);
  int req_slotid_tosend(int machine);
};

#endif



