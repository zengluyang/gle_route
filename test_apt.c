#include <stdio.h>
#include <stdint.h>

#define TRUE 1
#define FALSE 0
#define bool int
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
    if(acked_packet_table[i].seq == seq) {
      acked_packet_table[i].acked = TRUE;
    }
  }
}

bool is_no_ack_acked_packet_table(){
  int i;
  if(acked_packet_table_cur_index!=ACKED_PACKET_TABLE_SIZE){
    return FALSE;
  }
  for(i=0;i<acked_packet_table_cur_index;i++) {
    if(acked_packet_table[i].acked==TRUE){
      return FALSE;
    }
  }
  return TRUE;
}

int main