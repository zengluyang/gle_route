#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

#define TRUE 1
#define FALSE 0

#define BEST_FATHER_NODE_HISTORY_TABLE_SIZE 4
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

void print_best_father_history_table_stable() {
  int i;
  printf("BFHT: ");
  for(i=0;i<BEST_FATHER_NODE_HISTORY_TABLE_SIZE;i++) {
    printf("%d ",best_father_node_history_table[i]);
  }
  printf("\n");
}

int main () {
  init_best_father_node_history_table();
  print_best_father_history_table_stable();
  printf("is_best_father_history_table_stable(): %d\n",is_best_father_history_table_stable());

  add_to_best_father_node_history_table(8);
  print_best_father_history_table_stable();
  printf("is_best_father_history_table_stable(): %d\n\n",is_best_father_history_table_stable());

  add_to_best_father_node_history_table(8);
  add_to_best_father_node_history_table(8);
  add_to_best_father_node_history_table(8);
  add_to_best_father_node_history_table(8);
  add_to_best_father_node_history_table(8);
  add_to_best_father_node_history_table(8);
  print_best_father_history_table_stable();
  printf("is_best_father_history_table_stable(): %d\n\n",is_best_father_history_table_stable());

  add_to_best_father_node_history_table(8);
  print_best_father_history_table_stable();
  printf("is_best_father_history_table_stable(): %d\n\n",is_best_father_history_table_stable());

  add_to_best_father_node_history_table(9);
  print_best_father_history_table_stable();
  printf("is_best_father_history_table_stable(): %d\n\n",is_best_father_history_table_stable());

  return 0;
}