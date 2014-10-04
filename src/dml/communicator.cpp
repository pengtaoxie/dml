#include "communicator.h"
#include <iostream>
#include <unistd.h>


std::vector<message> communicator::read_msg_grp(int buf_id)
{
  return channels[buf_id]->get_buf()->read_msg_group();
}

message communicator::req_write_msg(int buf_id, int blocking, int & status)
{
  
  return channels[buf_id]->get_buf()->req_msg_to_write(blocking, status);
}

message communicator::retrv_msg_bysmpid(int buf_id, int smpid)
{
  
  return channels[buf_id]->get_buf()->retrv_msg_bysmpid(smpid);
}



communicator::communicator(buf_para_grp bpg,int rank)
{
  for(int i=0;i<bpg.size();i++)
  {
    //create channel;
    channel * cnl=new channel(bpg[i].capacity, bpg[i].dim, bpg[i].comm_type, bpg[i].push_type, bpg[i].pop_type, \
                              bpg[i].auto_release_after_send, bpg[i].machines, bpg[i].num_submsgs_foreach_smp,rank);
    channels.push_back(cnl);
  }
  this->rank=rank;
  init();
}

void communicator::post_processing(int connid)
{
  int slotid=slot_id_inuse[connid];
  //identify the buffer
  buffer * buf_ptr=(channels[channel_idxes[connid]])->get_buf();
  if(comm_types[connid]==RECV_COM)
  {
    buf_ptr->inc_machine_buffer_usage(machines[connid]);
    buf_ptr->insert_map_slotid_machineid(slotid,machines[connid]);
    buf_ptr->aggregrate_submsg(slotid);
  }  
  else if(comm_types[connid]==SEND_COM)//send
  {
    buf_ptr->post_processing_after_send(slotid); 
  }

  status[connid]=0;
  num_pending--;
}
void communicator::send_or_recv()
{
  //send or recv
  
  for(int i=0;i<num_connections;i++){
    //judge whether the connection is in use
    if(status[i]==1)
      continue;
    //identify the buffer
    buffer * buf_ptr=(channels[channel_idxes[i]])->get_buf();
    if(!buf_ptr->machine_valid_use(machines[i]))
      continue;

    int suc=-1;
    if(comm_types[i]==RECV_COM){
      //to recv
      suc=recv(buf_ptr, i);}
    else
      //to send
      suc=send(buf_ptr, i);
    if(suc==1)
    {
      num_pending++;
      status[i]=1;
    }
  }
}
int communicator::recv(buffer * buf_ptr, int conn_id)
{
  //req a free buffer
  int slotid=buf_ptr->req_push_slot();
  //std::cout<<buf_ptr->queue_slotidx_ready_push.size()<<std::endl;
  //std::cout<<slotid<<std::endl;
  //slotid=0;
  if(slotid!=-1)
  {
      slot_id_inuse[conn_id]=slotid;
      MPI_Irecv(buf_ptr->get_slot(slotid), num_float_comm[conn_id], MPI_FLOAT, machines[conn_id], \
             	1, MPI_COMM_WORLD, &(reqs[conn_id]));
      return 1;
  }
  return 0;
}

int communicator::send(buffer * buf_ptr, int conn_id)
{
  int slotid=buf_ptr->req_slotid_tosend(machines[conn_id]);
  if(slotid==-1)
    return 0;
  slot_id_inuse[conn_id]=slotid;
  MPI_Isend(buf_ptr->get_slot(slotid), num_float_comm[conn_id], MPI_FLOAT, machines[conn_id],
             	1, MPI_COMM_WORLD, &(reqs[conn_id]));
  return 1;

}



void communicator::init()
{
  num_connections=0;
  for(int i=0;i<channels.size();i++)
      num_connections+=(channels[i]->get_machine_list()).size();
  machines=new int[num_connections];
  channel_idxes=new int[num_connections];
  comm_types=new int[num_connections];
  num_float_comm=new int[num_connections];
  int idx=0;
  for(int i=0;i<channels.size();i++){
    int c_type=channels[i]->get_comm_type();
    int num_float=(channels[i]->get_buf())->get_dim();
    std::vector<int> machs=channels[i]->get_machine_list();
    for(int m=0;m<machs.size();m++){
      machines[idx]=machs[m];
      channel_idxes[idx]=i;
      comm_types[idx]=c_type;
      num_float_comm[idx]=num_float;
      idx++;
    }
  }

  slot_id_inuse=new int[num_connections];
  reqs=new MPI_Request[num_connections];
  num_pending=0;
  status=new int[num_connections];
  memset(status, 0, sizeof(int)*num_connections);
}
communicator::~communicator()
{
  delete[]machines;
  delete[]channel_idxes;
  delete[]comm_types;
  delete[]num_float_comm;
  delete[]slot_id_inuse;
  delete[]reqs;
  delete[]status;
  for(int i=0;i<channels.size();i++)
  delete[] channels[i];
}



int communicator::check_status()
{
  if(num_pending==0)
    return -1;
  MPI_Status  stat;
  int flag=0;
  MPI_Request * reqtmp=new MPI_Request[num_pending];
  int idx=0;
  std::unordered_map<int,int> map_checkid_connid;
  for(int i=0;i<num_connections;i++){
    if(status[i]==1){
      reqtmp[idx]=reqs[i];
      map_checkid_connid[idx]=i;
      idx++;
    }
  }
  int hit_idx=-1;
  MPI_Testany( num_pending, reqtmp, &hit_idx, &flag, &stat);
  if(flag!=0&&hit_idx>=0&&hit_idx<num_pending){
    delete[]reqtmp;

    return map_checkid_connid[hit_idx];
  }
  delete[]reqtmp;
  return -1;
}
void communicator::communicate()
{
  while(true)
  {
    //send or recv
    send_or_recv();
    //check_status
    int any_done=check_status();
    //post processing
    if(any_done!=-1)
       post_processing(any_done);
  }
}