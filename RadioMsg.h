#include "printf.h"


#ifndef RadioMsg_H_
#define RadioMsg_H_

enum
{
	MY_AM_ID = 0xC8
};

#define SEND_JREQ_INTERVAL 5000//ms
#define MAX_ENERGY 1000
#define SEND_SETTING_INTERVAL 60000ul


#define TYPE_JREQ       0x01
#define TYPE_JRES       0x02
#define TYPE_RU         0x03
#define TYPE_RR         0x04
#define TYPE_DATA       0x05
#define TYPE_SETTING    0x06
#define TYPE_ACK        0x07

int calc_uniform_energy(int engery) {
  int ue = 0xf*(engery*1.0/MAX_ENERGY);
  //printf("energy: %d\n",ue);
  return ue;
}

typedef nx_struct route_message {
  nx_uint8_t last_hop_addr;
  nx_uint8_t next_hop_addr;
  nx_uint8_t src_addr;
  nx_uint8_t dst_addr;
  nx_uint8_t type_gradient;
  nx_uint8_t energy_lqi;
  nx_uint8_t pair_addr;
  nx_uint16_t seq;
  nx_uint16_t self_send_cnt;
  nx_uint8_t length;
  nx_uint8_t payload[32];
}route_message_t;

void print_route_message(route_message_t* rm){
  int i;
  switch((rm->type_gradient& 0xf0)>>4) {
    case TYPE_JREQ:
      printf(" JREQ");
      break;
    case TYPE_JRES:
      printf(" JRES");
      break;
    case TYPE_RU:
      printf(" RU");
      break;
    case TYPE_RR:
      printf(" RR");
      break;
    case TYPE_DATA:
      printf(" DATA");
      break;
    case TYPE_SETTING:
      printf(" SETTING");
      break;
    case TYPE_ACK:
      printf(" ACK");
      break;
  }
  printf(" %d %d %d %d %d %d %d %d %d %d %d",
          rm->last_hop_addr,
          rm->next_hop_addr,
          rm->src_addr,
          rm->dst_addr,
          rm->type_gradient & 0x0f,
          (rm->energy_lqi & 0xf0)>>4,
          (rm->energy_lqi & 0x0f),
          rm->pair_addr,
          rm->seq,
          rm->self_send_cnt,
          rm->length
        );
  if(rm->length!=0) {
    printf(" ");
    for(i=0;i<rm->length;i++) {
      //printf("t");
      printf("%c",(char)rm->payload[i]);
    }
  }
  printf("\n");
}

typedef struct link_quality_entry {
  uint8_t node_id;
  uint16_t recv_cnt;
  uint16_t send_cnt;
  uint8_t local_lqi;
  bool to_send;
}link_quality_entry_t;

#define LQT_SIZE 256

link_quality_entry_t link_quality_table[LQT_SIZE];
void init_link_quality_table() {
  int i=0;
  for
  (
    ;
    i<sizeof(link_quality_table)/sizeof(link_quality_entry_t);
    i++) {
    link_quality_table[i].node_id=0;
    link_quality_table[i].recv_cnt=0;
    link_quality_table[i].send_cnt=0;
    link_quality_table[i].local_lqi=0;
    link_quality_table[i].to_send=0;
  }
}

link_quality_entry_t* access_link_quality_table(uint8_t node_id){
  return &(link_quality_table[node_id]);
}

void print_link_quality_table() {
  int i=0;
  printf("LQT:");
  for
  (
    ;
    i<sizeof(link_quality_table)/sizeof(link_quality_entry_t);
    i++) {
    if(link_quality_table[i].node_id!=0){
          #ifdef DEBUG
          //if(link_quality_table[i].node_id==1)
            //printf("LEAF DEBUG link_quality_table[i].recv_cnt:%d link_quality_table[i].send_cnt:%d\n",link_quality_table[i].recv_cnt,link_quality_table[i].send_cnt);
          #endif
          printf(
            "%d:%d %d, ",
            link_quality_table[i].node_id,
            link_quality_table[i].recv_cnt*0xf/link_quality_table[i].send_cnt,
            link_quality_table[i].local_lqi
          );

    }
  }
  printf("\n");
}

void print_link_quality_table_s() {
  int i=0;
  printf("LQTS:");
  for
  (
    ;
    i<sizeof(link_quality_table)/sizeof(link_quality_entry_t);
    i++) {
    if(link_quality_table[i].node_id!=0){
          #ifdef DEBUG
          //if(link_quality_table[i].node_id==1)
            //printf("LEAF DEBUG link_quality_table[i].recv_cnt:%d link_quality_table[i].send_cnt:%d\n",link_quality_table[i].recv_cnt,link_quality_table[i].send_cnt);
          #endif
          printf(
            "%d:%d %d, ",
            link_quality_table[i].node_id,
            link_quality_table[i].recv_cnt*0xf/link_quality_table[i].send_cnt,
            link_quality_table[i].local_lqi
          );

    }
  }
  printf("\n");
}

typedef struct father_node_table_entry {
  uint8_t node_id;
  uint8_t gradient;
  uint8_t energy;
  uint8_t lqi;
}father_node_table_entry_t;

#define FNT_SIZE 256

father_node_table_entry_t father_node_table[FNT_SIZE];

uint8_t current_best_father_node = 0;
void init_father_node_table() {
  int i=0;
  for
  (
    ;
    i<sizeof(father_node_table)/sizeof(father_node_table_entry_t);
    i++) {
    father_node_table[i].node_id=0;
    father_node_table[i].gradient=0;
    father_node_table[i].energy=0;
    father_node_table[i].lqi=0;
  }
}

father_node_table_entry_t* access_father_node_table(uint8_t node_id){
  return &(father_node_table[node_id]);
}

void delete_father_node_table(uint8_t node_id) {
    father_node_table[node_id].node_id=0;
    father_node_table[node_id].gradient=0;
    father_node_table[node_id].energy=0;
    father_node_table[node_id].lqi=0;

}

void print_father_node_table() {
  int i=0;
  printf("FNT: current:%d. ",current_best_father_node);
  for
  (
    ;
    i<sizeof(father_node_table)/sizeof(father_node_table_entry_t);
    i++) {
    if(father_node_table[i].node_id!=0){
          #ifdef DEBUG
          //if(link_quality_table[i].node_id==1)
            //printf("LEAF DEBUG link_quality_table[i].recv_cnt:%d link_quality_table[i].send_cnt:%d\n",link_quality_table[i].recv_cnt,link_quality_table[i].send_cnt);
          #endif
          printf(
            "%d:%d %d %d, ",
            father_node_table[i].node_id,
            father_node_table[i].gradient,
            father_node_table[i].energy,
            father_node_table[i].lqi
          );

    }
  }
  printf("\n");
}

void print_father_node_table_s() {
  int i=0;
  printf("FNTS: current:%d. ",current_best_father_node);
  for
  (
    ;
    i<sizeof(father_node_table)/sizeof(father_node_table_entry_t);
    i++) {
    if(father_node_table[i].node_id!=0){
          #ifdef DEBUG
          //if(link_quality_table[i].node_id==1)
            //printf("LEAF DEBUG link_quality_table[i].recv_cnt:%d link_quality_table[i].send_cnt:%d\n",link_quality_table[i].recv_cnt,link_quality_table[i].send_cnt);
          #endif
          printf(
            "%d:%d %d %d, ",
            father_node_table[i].node_id,
            father_node_table[i].gradient,
            father_node_table[i].energy,
            father_node_table[i].lqi
          );

    }
  }
  printf("\n");
}

#define RANK_A 16
#define RANK_B 64
#define RANK_C 16
uint32_t calc_father_rank(uint8_t gradient,uint8_t energy,uint8_t lqi) {
  return RANK_A*(15-gradient) + RANK_B*energy + RANK_C*lqi;
}
uint8_t calc_best_father_node() {
  uint8_t best_node_id = 0;
  uint32_t best_rank = 0;
  int i=0;
  uint32_t rank=0;
  for(;i<FNT_SIZE;i++) {
    if(father_node_table[i].node_id!=0) {
      rank = calc_father_rank(
        father_node_table[i].gradient,
        father_node_table[i].energy,
        father_node_table[i].lqi);
      if(rank>best_rank) {
        best_node_id = i;
        best_rank = rank;
      }
    }
  }
  return best_node_id;
}

#define BEST_FATHER_NODE_HISTORY_TABLE_SIZE 8
uint8_t best_father_node_history_table[BEST_FATHER_NODE_HISTORY_TABLE_SIZE];

void init_best_father_node_history_table() {
  int i;
  for(i=0;i<BEST_FATHER_NODE_HISTORY_TABLE_SIZE;i++) {
    best_father_node_history_table[i] = 0;
  }
}

void add_to_best_father_node_history_table(uint8_t node_id) {
  int i;
  for(i=0;i<BEST_FATHER_NODE_HISTORY_TABLE_SIZE-1;i++) {
    best_father_node_history_table[i]=best_father_node_history_table[i+1];
  }
  best_father_node_history_table[BEST_FATHER_NODE_HISTORY_TABLE_SIZE-1] = node_id;
}

bool is_best_father_history_table_stable() {
  int i;
  for(i=0;i<BEST_FATHER_NODE_HISTORY_TABLE_SIZE-1;i++) {
    if(best_father_node_history_table[BEST_FATHER_NODE_HISTORY_TABLE_SIZE-i-1]==0) {
      return FALSE;
    }
    if(best_father_node_history_table[BEST_FATHER_NODE_HISTORY_TABLE_SIZE-i-1]!=best_father_node_history_table[BEST_FATHER_NODE_HISTORY_TABLE_SIZE-i-2]) {
      return FALSE;
    }
  }
  return TRUE;
}

void print_best_father_history_table() {
  int i;
  printf("BFHT: ");
  for(i=0;i<BEST_FATHER_NODE_HISTORY_TABLE_SIZE;i++) {
    printf("%d ",best_father_node_history_table[i]);
  }
  printf("\n");
}

#define ACKED_PACKET_TABLE_SIZE 8

typedef struct{
  uint16_t seq;
  bool acked;
}acked_packet_table_entry_t;

acked_packet_table_entry_t acked_packet_table[ACKED_PACKET_TABLE_SIZE];
int acked_packet_table_cur_index = 0;
void init_acked_packet_table() {
  int i;
  for(i=0;i<ACKED_PACKET_TABLE_SIZE;i++) {
    acked_packet_table[i].seq = 0;
    acked_packet_table[i].acked = FALSE;
  }
  acked_packet_table_cur_index = 0;
}

void printf_acked_packet_table(){
  int i;
  printf("APT:");
  for(i=0;i<acked_packet_table_cur_index;i++) {
    printf("%d %d, ",acked_packet_table[i].seq,acked_packet_table[i].acked);
  }
  printf("\n");
}


void add_to_acked_packet_table(uint16_t seq) {
  acked_packet_table[acked_packet_table_cur_index].seq = seq;
  acked_packet_table[acked_packet_table_cur_index].acked = FALSE;
  acked_packet_table_cur_index++;
  if(acked_packet_table_cur_index==ACKED_PACKET_TABLE_SIZE+1) {
    acked_packet_table_cur_index=0;
  }
}

void set_acked_packet_table(uint16_t seq){
  int i;
  printf("seq: %d, ",seq);
  printf_acked_packet_table();
  for(i=0;i<acked_packet_table_cur_index;i++) {
    if(acked_packet_table[acked_packet_table_cur_index].seq == seq) {
      acked_packet_table[acked_packet_table_cur_index].acked = TRUE;
    }
  }
}

bool is_no_ack_acked_packet_table(){
  int i;
  if(acked_packet_table_cur_index!=ACKED_PACKET_TABLE_SIZE){
    return FALSE;
  }
  for(i=0;i<acked_packet_table_cur_index;i++) {
    if(acked_packet_table[acked_packet_table_cur_index].acked==TRUE){
      return FALSE;
    }
  }
  return TRUE;
}


#define SETTING_ROUTE_TABLE_SIZE 256 

typedef struct setting_route_table_entry{
  uint8_t dst_addr;
  uint8_t next_hop_addr;
} setting_route_table_entry_t;

setting_route_table_entry_t setting_route_table[SETTING_ROUTE_TABLE_SIZE];

void init_setting_route_table() {
  int i;
  for(i=0;i<SETTING_ROUTE_TABLE_SIZE;i++) {
    setting_route_table[i].dst_addr = 0;
    setting_route_table[i].next_hop_addr = 0;
  }
}

setting_route_table_entry_t* access_setting_route_table(uint8_t node_id){
  return &(setting_route_table[node_id]);
}

void delete_setting_route_table(uint8_t node_id) {
    setting_route_table[node_id].dst_addr=0;
    setting_route_table[node_id].next_hop_addr=0;
}

void print_setting_route_table() {
  int i=0;
  printf("SRT:");
  for
  (
    ;
    i<sizeof(setting_route_table)/sizeof(setting_route_table_entry_t);
    i++) {
    if((setting_route_table[i]).dst_addr!=0){
          #ifdef DEBUG
          //if(link_quality_table[i].node_id==1)
            //printf("LEAF DEBUG link_quality_table[i].recv_cnt:%d link_quality_table[i].send_cnt:%d\n",link_quality_table[i].recv_cnt,link_quality_table[i].send_cnt);
          #endif
          printf(
            "%d %d, ",
            setting_route_table[i].dst_addr,
            setting_route_table[i].next_hop_addr
          );

    }
  }
  printf("\n");
}



void print_best_father_history_table_s() {
  int i;
  printf("BFHTS: ");
  for(i=0;i<BEST_FATHER_NODE_HISTORY_TABLE_SIZE;i++) {
    printf("%d ",best_father_node_history_table[i]);
  }
  printf("\n");
}

#define PACKET_TO_BE_ACKED_TABLE_SIZE 8



typedef struct 
{
  uint16_t seq;
  uint8_t next_hop;
}packet_to_be_acked_table_entry_t;


packet_to_be_acked_table_entry_t packet_to_be_acked_table[PACKET_TO_BE_ACKED_TABLE_SIZE];

void init_packet_to_be_acked_table() {
  int i;
  for(i=0;i<PACKET_TO_BE_ACKED_TABLE_SIZE;i++) {
    packet_to_be_acked_table[i].seq=0;
    packet_to_be_acked_table[i].next_hop=0;
  }
}

bool add_to_packet_to_be_acked_table(uint16_t seq,uint8_t next_hop) {
// if the table is full returns FALSE
  int i;
  for(i=0;i<PACKET_TO_BE_ACKED_TABLE_SIZE;i++) {
    if(packet_to_be_acked_table[i].seq==0){
      packet_to_be_acked_table[i].seq = seq;
      packet_to_be_acked_table[i].next_hop = next_hop;
      return TRUE;
    }
  }
  return FALSE;
}


void delete_packet_to_be_acked_table(uint16_t seq,uint8_t next_hop){

  int i;
  for(i=0;i<PACKET_TO_BE_ACKED_TABLE_SIZE;i++) {
    if(packet_to_be_acked_table[i].seq==seq && packet_to_be_acked_table[i].next_hop==next_hop){
      packet_to_be_acked_table[i].seq = 0;
      packet_to_be_acked_table[i].next_hop = 0;
      return;
    }
  }
}

void print_packet_to_be_acked_table() {


  int i;
  printf("PAT ");
  for(i=0;i<PACKET_TO_BE_ACKED_TABLE_SIZE;i++) {
    if(packet_to_be_acked_table[i].seq!=0){
      printf("%d %d, ", packet_to_be_acked_table[i].seq,packet_to_be_acked_table[i].next_hop);
    }
  }
  printf("\n");
}

#endif