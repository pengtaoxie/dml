#include "buffer.h"
#include <cstring>
#include <iostream>


int buffer::req_slotid_tosend(int machine)
{
  int slotid=-1;
  {
    std::lock_guard<std::mutex> lock(mutex_machine_slottosend);
    if(!machine_slottosend[machine].empty())
    {
      slotid=(machine_slottosend[machine]).front();
      machine_slottosend[machine].pop();
    }
  }
  return slotid;
}

void buffer::post_processing_after_write(int slotid)
{
  //for all the machines
  {
    std::lock_guard<std::mutex> lock(mutex_machine_slottosend);
    for ( auto it = machine_slottosend.begin(); it != machine_slottosend.end(); ++it )
      it->second.push(slotid);
  }
  if(!auto_release_after_send)
  {
    std::lock_guard<std::mutex> lock(mutex_map_smpidx_slotidxlist);
    std::vector<int>slotidlist;
    slotidlist.push_back(slotid);
    map_smpidx_slotidxlist[buffer_region[slotid][0]]=slotidlist;
  }
}

void buffer::post_processing_after_send(int slotid)
{
  bool totaldone=false;
  {
    std::lock_guard<std::mutex> lock(mutex_slotid_numfinishedmachines);
    slotid_numfinishedmachines[slotid]=slotid_numfinishedmachines[slotid]+1;
    if(slotid_numfinishedmachines[slotid]== num_send_machines)
    {
      slotid_numfinishedmachines[slotid]=0;
      totaldone=true;
    }
  }
  if(totaldone)
  {
    if(auto_release_after_send)//auto release
    {
      std::lock_guard<std::mutex> lock(mutex_queue_slotidx_ready_push);
      queue_slotidx_ready_push.push(slotid);
    }
    else//
    {
      std::lock_guard<std::mutex> lock(mutex_send_done_status);
      send_done_status[slotid]=1;
    }
  }
}

void buffer::buffer_update(int i,int j,float v)
{
  {
    std::lock_guard<std::mutex> lock(mutex_buffer_region);
    buffer_region[i][j]=v;
  }
}

void buffer::insert_map_slotid_machineid(int slotid, int machineid)
{
  {
    std::lock_guard<std::mutex> lock(mutex_slotid_machineid);
    slotid_machineid[slotid]=machineid;
  }
}


void buffer::inc_machine_buffer_usage(int machineid)
{
  {
    std::lock_guard<std::mutex> lock(mutex_machine_buffer_usage);
    machine_buffer_usage[machineid]=machine_buffer_usage[machineid]+1;;
  }

}



bool buffer::machine_valid_use(int machineid)
{
  {
    std::lock_guard<std::mutex> lock(mutex_machine_buffer_usage);
    if(machine_buffer_usage[machineid]<=capacity*1.0/machine_buffer_usage.size())
      return true;
    else
      return false;
  }
}
buffer::buffer(int capacity, int dim, int push_type, int pop_type, bool auto_release_after_send, int num_submsgs_foreach_smp,int rank,
     std::vector<int> machines)
{
  this->capacity=capacity;
  this->dim=dim;
  this->push_type=push_type;
  this->pop_type=pop_type;
  this->auto_release_after_send=auto_release_after_send;
  this->num_submsgs_foreach_smp=num_submsgs_foreach_smp;
  buffer_region=new float*[capacity];
  for(int i=0;i<capacity;i++){
    buffer_region[i]=new float[dim];
  }
  send_done_status=new int[capacity];
  std::memset(send_done_status,0, sizeof(int)*capacity);
  for(int i=0;i<capacity;i++)
    queue_slotidx_ready_push.push(i);
  this->rank=rank;
  if(push_type==0)
  {
    for(int i=0;i<machines.size();i++)
      machine_buffer_usage[machines[i]]=0;
  }
  else if (pop_type==0)
  {
    num_send_machines=machines.size();
    for(int i=0;i<machines.size();i++)
    {
      std::queue<int> q;
      machine_slottosend[machines[i]]=q;
    }
    for(int i=0;i<capacity;i++)
      slotid_numfinishedmachines[i]=0;
  }
}

buffer::~buffer()
{
  delete []send_done_status;
  for(int i=0;i<capacity;i++){
    delete[] buffer_region[i];
  }
  delete [] buffer_region;
}

int buffer::get_dim()
{
  return dim;
}

bool buffer::get_auto_release_after_send()
{
  return auto_release_after_send;
}

std::vector<message> buffer::read_msg_group()
{
  std::vector<int>submsgids;
  int suc=0;

  while(true){
    suc=get_slotids_for_submsgs(submsgids);
    if(suc==1)
      break;
  }
  std::vector<message> msg_grp;
  for(int i=0;i<submsgids.size();i++){
    int slot_idx=submsgids[i];
    message m(buffer_region[slot_idx][0],buffer_region[slot_idx][1],dim-2, buffer_region[slot_idx]+2, this, slot_idx);
    msg_grp.push_back(m);
  }
  return msg_grp;
}
message buffer::req_msg_to_write(int blocking, int & status)
{
  int idx=-1;
  while(true){
    idx=req_push_slot();
    if(blocking!=1&&idx==-1)
    {
      status=0;
      return message();
    }
    if(idx!=-1)
      break;
  }
  message m(-1,-1, dim-2, buffer_region[idx]+2, this, idx);
  return m;
}

int buffer::req_push_slot()
{
  int idx=-1;
  //critical region
  {
    std::lock_guard<std::mutex> recv_lock(mutex_queue_slotidx_ready_push);
    if(!queue_slotidx_ready_push.empty()){
      idx=queue_slotidx_ready_push.front();
      queue_slotidx_ready_push.pop();
    }
  }
  return idx;  
}

float * buffer::get_slot(int slotid)
{
  return buffer_region[slotid];
}

void buffer::update_queue_slotidx_ready_push(int slotid, int smpidx)
{
  
  while(true)
  { 
    if(!(pop_type==SEND&&auto_release_after_send==false))//if not manual release after send
      break;
    {
      std::lock_guard<std::mutex> lock(mutex_send_done_status);
      if(send_done_status[slotid]==1)
      {
         send_done_status[slotid]=0;
         break;
      }
    }
  }

  {
    std::lock(mutex_machine_buffer_usage,mutex_slotid_machineid,mutex_queue_slotidx_ready_push);
    std::lock_guard<std::mutex> guard_1(mutex_machine_buffer_usage,std::adopt_lock);
    std::lock_guard<std::mutex> guard_2(mutex_slotid_machineid,std::adopt_lock);
    std::lock_guard<std::mutex> guard_3(mutex_queue_slotidx_ready_push, std::adopt_lock);

    queue_slotidx_ready_push.push(slotid);
    if(push_type==RECV)
    {
      machine_buffer_usage[slotid_machineid[slotid]]=machine_buffer_usage[slotid_machineid[slotid]]-1;
      slotid_machineid.erase(slotid);
    }
    
  }
  if((pop_type==SEND&&auto_release_after_send==false)||push_type==RECV){//manual release
    {
      std::lock_guard<std::mutex> lock(mutex_map_smpidx_slotidxlist);
      map_smpidx_slotidxlist.erase(smpidx);
    }
  }
}



//the submsg is either recved or written by computing thread
void buffer::aggregrate_submsg(int slotid)
{
  
  int smpid=buffer_region[slotid][0];
  //int pttid=buffer_region[slotid][1];
  bool ready=false;
  {
    std::lock_guard<std::mutex> lock(mutex_map_smpidx_slotidxlist);
    std::unordered_map<int,std::vector<int>>::const_iterator coit = map_smpidx_slotidxlist.find (smpid);
    if ( coit == map_smpidx_slotidxlist.end() )
    {
      std::vector<int> slotlist;
      slotlist.push_back(slotid);
      map_smpidx_slotidxlist[smpid]=slotlist;
    }
    else
    {
      map_smpidx_slotidxlist[smpid].push_back(slotid);
    }
    if(map_smpidx_slotidxlist[smpid].size()==num_submsgs_foreach_smp)
      ready=true;
  }
  if(ready)
  {
    std::lock_guard<std::mutex> lock(mutex_queue_smpidx_ready_process);
    queue_smpidx_ready_process.push(smpid);
  }
}
void buffer::update_send_done_status(int slotid)
{
  
  {
    std::lock_guard<std::mutex> lock(mutex_send_done_status);
    send_done_status[slotid]=1;
  }
}

message buffer::retrv_msg_bysmpid(int smpid)
{
  int slotid=-1;
  {
    std::lock_guard<std::mutex> lock(mutex_map_smpidx_slotidxlist);
    slotid=map_smpidx_slotidxlist[smpid][0];
  }
  message m(buffer_region[slotid][0],buffer_region[slotid][1], dim-2, buffer_region[slotid]+2, this, slotid);
  return m;

}

int buffer::get_slotids_for_submsgs(std::vector<int> & slotidlist)
{
  int ready_smpidx=-1;
  //critical region
  {
    std::lock_guard<std::mutex> read_lock(mutex_queue_smpidx_ready_process);
    if(!queue_smpidx_ready_process.empty()){
      ready_smpidx=queue_smpidx_ready_process.front();
      queue_smpidx_ready_process.pop();
    }
  }
  if(ready_smpidx!=-1){
    
      std::lock_guard<std::mutex> map_lock(mutex_map_smpidx_slotidxlist);
      slotidlist=map_smpidx_slotidxlist[ready_smpidx];
      if(!(pop_type==0&&auto_release_after_send==false)&&!push_type==RECV)//if not manual release after send
        map_smpidx_slotidxlist.erase(ready_smpidx);
      
      return 1;
  }
  return 0;
}
