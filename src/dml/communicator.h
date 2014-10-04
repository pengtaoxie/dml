#ifndef COMMUNICATION_H_
#define COMMUNICATION_H_

#include "buf_para.h"
#include "buffer.h"
#include "channel.h"
#include "message.h"
#include "mpi.h"
#include <vector>
#include <unordered_map>

class communicator
{
private:
  
  int num_connections;//number of connections
  int * machines;//machines corresponding to the connections
  int * channel_idxes;//channel idxes corresponding to the connections
  int * comm_types;//for each connecton, send or recv, 0 recv ;  1 send
  int * num_float_comm;//number of float to communicate 
  int * slot_id_inuse;//which slot the communication is using
  MPI_Request  * reqs;//test the status of each machine
  int num_pending;//number of unfinished communications
  int * status;//status of each connection, 0 finished; 1 not finished
  
  void init();
  void send_or_recv();
  int send(buffer * buf_ptr, int conn_id);//send data
  int recv(buffer * buf_ptr, int conn_id);//recv data
  int check_status();//check the status of connections
  void post_processing(int connid);//
public:
  int rank;
  std::vector<channel *> channels;
  communicator(buf_para_grp bpg,int rank);
  ~communicator();
  void communicate();
  std::vector<message> read_msg_grp(int buf_id);
  message req_write_msg(int buf_id, int blocking, int & status);
  message retrv_msg_bysmpid(int buf_id, int smpid);
  
  
};


#endif












