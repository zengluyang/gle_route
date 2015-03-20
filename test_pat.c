

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#define TRUE 1
#define FALSE 0

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


int main() {
	init_packet_to_be_acked_table();
	add_to_packet_to_be_acked_table(1,38);
	add_to_packet_to_be_acked_table(2,38);
	add_to_packet_to_be_acked_table(3,38);
	add_to_packet_to_be_acked_table(4,38);
	print_packet_to_be_acked_table();

	add_to_packet_to_be_acked_table(5,38);
	add_to_packet_to_be_acked_table(6,38);
	add_to_packet_to_be_acked_table(7,38);
	add_to_packet_to_be_acked_table(8,38);
	print_packet_to_be_acked_table();

	int rlt = add_to_packet_to_be_acked_table(9,38);
	assert(rlt==FALSE);

	delete_packet_to_be_acked_table(1,38);
	print_packet_to_be_acked_table();

	delete_packet_to_be_acked_table(7,38);
	print_packet_to_be_acked_table();

	delete_packet_to_be_acked_table(9,38);
	print_packet_to_be_acked_table();

	rlt = add_to_packet_to_be_acked_table(9,38);
	assert(rlt==TRUE);
	print_packet_to_be_acked_table();

	delete_packet_to_be_acked_table(7,38);
	print_packet_to_be_acked_table();



	printf("sizeof(packet_to_be_acked_table): %dbytes\n",sizeof(packet_to_be_acked_table));
	return 0;
}


