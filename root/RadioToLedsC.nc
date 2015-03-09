#include "Timer.h"
#include "../RadioMsg.h"
#include "printf.h"
//root node
module RadioToLedsC {
	uses {
		interface Boot;
		interface SplitControl as AMControl;
		interface Packet;
		interface AMSend;
		interface Receive;
		interface Leds;
	}
}

implementation {
	bool busy = FALSE;
	message_t packet;
	int self_gradient = 0;
	int self_energy = MAX_ENERGY;
	int send_cnt = 1;
	event void Boot.booted() {
		printf("ROOT BOOT\n");
		call AMControl.start();
		//call Leds.led0On();
		//call Leds.led1On();
		//call Leds.led2On();
	}

	event void AMControl.startDone(error_t err) {
		if(err==SUCCESS) {
			//good!
		} else {
			init_link_quality_table();
			call AMControl.start();
		}
	}

	event void AMControl.stopDone(error_t error) {}

	event message_t* Receive.receive(message_t *msg, 
		void *payload, uint8_t len) {
		route_message_t *rm;
		route_message_t *sm;
		bool on = FALSE;
		link_quality_entry_t *lqe;

		rm = (route_message_t *) payload;
		sm = (route_message_t *) call Packet.getPayload(&packet,sizeof(route_message_t));
		lqe = access_link_quality_table(rm->last_hop_addr);
		if(lqe->node_id==0) {
			lqe->node_id = rm->last_hop_addr;
			lqe->recv_cnt = 1;
			lqe->send_cnt = rm->self_send_cnt;
			if(rm->pair_addr == TOS_NODE_ID) {
				lqe->local_lqi = rm->energy_lqi & 0x0f;
			}
		}
		if((rm->type_gradient & 0xf0)>>4 == TYPE_JREQ) {
			printf("ROOT RECV");
			print_route_message(rm);
			sm->last_hop_addr = TOS_NODE_ID;
			sm->next_hop_addr = rm->last_hop_addr;
			sm->src_addr = TOS_NODE_ID;
			sm->dst_addr = rm->src_addr;
			sm->type_gradient = TYPE_JRES<<4 | self_gradient;
			sm->energy_lqi = calc_uniform_energy(self_energy)<<4 | lqe->local_lqi; // TODO 
			sm->self_send_cnt = send_cnt;
			//printf("sm->energy_lqi: %d\n",sm->energy_lqi);
			if(!busy && call AMSend.send(AM_BROADCAST_ADDR,&packet, sizeof(route_message_t))==SUCCESS) {
				busy = TRUE;
				printf("ROOT SEND");
				print_route_message(sm);
			}
		} else if ((rm->type_gradient & 0xf0)>>4 == TYPE_DATA) {
			printf("ROOT RECV");
			print_route_message(rm);
		}
		return msg;
	}

	event void AMSend.sendDone(message_t *msg, error_t error) 
	{
		send_cnt++;
		if(self_energy <=0 ) {
			busy = TRUE;
		} else {
			self_energy--;
		}
		busy=FALSE;
	}


}