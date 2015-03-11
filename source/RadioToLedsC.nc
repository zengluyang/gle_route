#include "Timer.h"
#include "../RadioMsg.h"
#include "printf.h"
//root node

#define SEND_DATA_INTERVAL 7000
module RadioToLedsC {
	uses {
		interface Boot;
		interface SplitControl as AMControl;
		interface Packet;
		interface AMSend;
		interface Receive;
		interface Leds;
		interface Timer<TMilli> as JREQ_Timer;
		interface Timer<TMilli> as DATA_Timer;
	}
}

implementation {
	bool busy = FALSE;
	message_t packet;
	int self_gradient = 15;
	int self_energy = MAX_ENERGY;
	int send_cnt = 1;
	event void Boot.booted() {
		printf("LEAF BOOT\n");
		call AMControl.start();
		//call Leds.led0On();
		//call Leds.led1On();
		//call Leds.led2On();
	}


	void refresh_best_father_node() {
		father_node_table_entry_t *fne;

  		current_best_father_node = calc_best_father_node();
  		fne = access_father_node_table(current_best_father_node);
  		if(current_best_father_node !=0 && fne->gradient + 1< self_gradient)
			self_gradient = fne->gradient + 1;
	}

	event void AMControl.startDone(error_t err) {
		if(err==SUCCESS) {
			//good!
			init_link_quality_table();
			call JREQ_Timer.startPeriodic(SEND_JREQ_INTERVAL);
			call DATA_Timer.startPeriodic(SEND_DATA_INTERVAL);
		} else {
			
			call AMControl.start();
		}
	}
	event void JREQ_Timer.fired() {
		link_quality_entry_t *lqe;
        route_message_t* rpkt;
        int to_send_node_id;
        for (
        	to_send_node_id = send_cnt % (LQT_SIZE),lqe = access_link_quality_table(to_send_node_id);
        	lqe->node_id==0 && to_send_node_id<LQT_SIZE;
        	lqe = access_link_quality_table(to_send_node_id++)
        ) {
        	//printf("LEAF DEBUG to_send_node_id %d lqe->node_id: %d\n",to_send_node_id,lqe->node_id);
        }

        rpkt = (route_message_t*) call Packet.getPayload(&packet, sizeof(route_message_t));
        rpkt->last_hop_addr = TOS_NODE_ID;
        rpkt->next_hop_addr = 0xff;
        rpkt->src_addr = TOS_NODE_ID;
        rpkt->dst_addr = 0xff;
        rpkt->type_gradient = TYPE_JREQ<<4 | self_gradient;
        rpkt->energy_lqi = calc_uniform_energy(self_energy)<<4 | lqe->local_lqi;
        rpkt->pair_addr = lqe->node_id;
        rpkt->seq = 0;
        rpkt->self_send_cnt = send_cnt;
        rpkt->length = 0;
        if(!busy && call AMSend.send(AM_BROADCAST_ADDR,&packet,sizeof(route_message_t)) == SUCCESS ){
        	printf("LEAF SEND");
        	print_route_message(rpkt);
        	busy=TRUE;
        }
        refresh_best_father_node();
        print_link_quality_table();
        print_father_node_table();
            //printfflush();
    }

    event void DATA_Timer.fired() {
        route_message_t* rpkt;
        rpkt = (route_message_t*) call Packet.getPayload(&packet, sizeof(route_message_t));
        rpkt->last_hop_addr = TOS_NODE_ID;
        rpkt->next_hop_addr = current_best_father_node;
        rpkt->src_addr = TOS_NODE_ID;
        rpkt->dst_addr = 0x01;
        rpkt->type_gradient = TYPE_DATA<<4 | self_gradient;
        //rpkt->energy_lqi = calc_uniform_energy(self_energy)<<4 | lqe->local_lqi;
        //rpkt->pair_addr = lqe->node_id;
        rpkt->seq = 0;
        rpkt->self_send_cnt = send_cnt;
        rpkt->length = 6;
        rpkt->payload[0] = 0x1;
        rpkt->payload[1] = 0x2;
        rpkt->payload[2] = 0x3;
        rpkt->payload[3] = 0x4;
        rpkt->payload[4] = 0x5;
        rpkt->payload[5] = 0x6;
        if(!busy && call AMSend.send(AM_BROADCAST_ADDR,&packet,sizeof(route_message_t)) == SUCCESS ){
        	printf("SOURCE SEND");
        	print_route_message(rpkt);
        	busy=TRUE;
        }
            //printfflush();
    }


	event void AMControl.stopDone(error_t error) {}



	event message_t* Receive.receive(message_t *msg, 
		void *payload, uint8_t len) {
		route_message_t *rm;
		route_message_t *sm;
		bool on = FALSE;
		link_quality_entry_t *lqe;
		father_node_table_entry_t *fne;

		rm = (route_message_t *) payload;
		sm = (route_message_t *) call Packet.getPayload(&packet,sizeof(route_message_t));
		lqe = access_link_quality_table(rm->last_hop_addr);

		if((rm->type_gradient & 0xf0)>>4 == TYPE_JREQ) {
			printf("LEAF RECV");
			print_route_message(rm);
			sm->last_hop_addr = TOS_NODE_ID;
			sm->next_hop_addr = rm->last_hop_addr;
			sm->src_addr = TOS_NODE_ID;
			sm->dst_addr = rm->src_addr;
			sm->type_gradient = TYPE_JRES<<4 | self_gradient;
			sm->energy_lqi = calc_uniform_energy(self_energy)<<4 | lqe->local_lqi; // TODO 
			sm->pair_addr = lqe->node_id;
			sm->length = 0;
			//printf("sm->energy_lqi: %d\n",sm->energy_lqi);
			if(!busy && call AMSend.send(AM_BROADCAST_ADDR,&packet, sizeof(route_message_t))==SUCCESS) {
				busy = TRUE;
				printf("LEAF SEND");
				print_route_message(sm);
			}
		}

		if ((rm->type_gradient & 0xf0)>>4 == TYPE_DATA) {

		}


		if(rm->type_gradient & 0x0f > self_gradient) {
			return msg;
		}

		if(rm->pair_addr == TOS_NODE_ID) {
				lqe->local_lqi = rm->energy_lqi & 0x0f;
		}
		lqe->send_cnt = rm->self_send_cnt;
		if(lqe->node_id==0) {
			lqe->node_id = rm->last_hop_addr;
			lqe->recv_cnt = 1;
			
		} else {
			lqe->recv_cnt++;
		}

		if ((rm->type_gradient & 0xf0)>>4 == TYPE_JRES) {
			printf("LEAF RECV");
			print_route_message(rm);
			fne = access_father_node_table(rm->last_hop_addr);
			fne->node_id = rm->last_hop_addr;
			fne->gradient = rm->type_gradient & 0x0f;
			fne->energy = (rm->energy_lqi & 0xf0) >> 4;
			fne->lqi = lqe->local_lqi;
			if(current_best_father_node==0) {
				refresh_best_father_node();
			}
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