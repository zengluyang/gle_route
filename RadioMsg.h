#include "printf.h"


#ifndef RadioMsg_H_
#define RadioMsg_H_

enum
{
	MY_AM_ID = 0xC8
};

#define SEND_JREQ_INTERVAL 5000//ms
#define MAX_ENERGY 1000

#define TYPE_JREQ       0x01
#define TYPE_JRES       0x02
#define TYPE_RU         0x03
#define TYPE_RR         0x04
#define TYPE_DATA       0x05
#define TYPE_SETTTING   0x06

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
  nx_uint8_t seq;
  nx_uint8_t self_send_cnt;
  nx_uint8_t length;
  nx_uint8_t payload[];
}route_message_t;

void print_route_message(route_message_t* rm){
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
    case TYPE_SETTTING:
      printf(" SETTING");
      break;
  }
  printf(" %d %d %d %d %d %d %d %d %d %d %d\n",
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
#define RANK_A 256
#define RANK_B 16
#define RANK_C 16
uint32_t calc_father_rank(uint8_t gradient,uint8_t energy,uint8_t lqi) {
  return RANK_A/(gradient+1) + RANK_B*energy + RANK_C*lqi;
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




#endif