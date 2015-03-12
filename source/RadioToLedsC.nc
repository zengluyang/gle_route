

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
		interface Timer<TMilli> as JREQ_Timer;
		interface Queue<route_message_t> as SendQueue;
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
		printf("sizeof(route_message_t):%d\n",sizeof(route_message_t));
		call AMControl.start();
		//call Leds.led0On();
		//call Leds.led1On();
		//call Leds.led2On();
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
        rpkt->payload[0] = 'h';
        rpkt->payload[1] = 'e';
        rpkt->payload[2] = 'l';
        rpkt->payload[3] = 'l';
        rpkt->payload[4] = 'o';
        rpkt->payload[5] = '\0';
        if(!busy && call AMSend.send(AM_BROADCAST_ADDR,&packet,sizeof(route_message_t)) == SUCCESS ){
        	printf("SOURCE SEND");
        	print_route_message(rpkt);
        	busy=TRUE;
        }
            //printfflush();
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
			call DATA_Timer.startPeriodic(9500);
		} else {
			
			call AMControl.start();
		}
	}
	int last_send_id = 0;
	event void JREQ_Timer.fired() {
		link_quality_entry_t *lqe;
        route_message_t* rpkt;
        int to_send_node_id;
        if(send_cnt>200 && current_best_father_node!=0 && self_gradient!=0xf) {
        	return ;
        }
        to_send_node_id = last_send_id;
        lqe = access_link_quality_table(to_send_node_id);
        printf("LEAF DEBUG before for loop to_send_node_id:%d last_send_id:%d\n",to_send_node_id,last_send_id);
        for (
        	;
        	lqe->node_id==0 && to_send_node_id<LQT_SIZE;
        	to_send_node_id++
        ) {
			lqe = access_link_quality_table(to_send_node_id);
        	if(lqe->node_id!=0)
        		printf("LEAF DEBUG to_send_node_id %d lqe->node_id: %d %d %d\n",to_send_node_id,lqe->node_id,lqe->recv_cnt,lqe->send_cnt);
        }
        if(to_send_node_id==256) {
        	last_send_id = 0;
        } else {
        	 last_send_id = to_send_node_id + 1;
        }
        rpkt = (route_message_t*) call Packet.getPayload(&packet, sizeof(route_message_t));
        rpkt->last_hop_addr = TOS_NODE_ID;
        rpkt->next_hop_addr = 0xff;
        rpkt->src_addr = TOS_NODE_ID;
        rpkt->dst_addr = 0xff;
        rpkt->type_gradient = TYPE_JREQ<<4 | self_gradient;
        rpkt->energy_lqi = calc_uniform_energy(self_energy)<<4 | lqe->recv_cnt*0xf/lqe->send_cnt;
        rpkt->pair_addr = lqe->node_id;
        rpkt->seq = 0;
        rpkt->self_send_cnt = send_cnt;
        rpkt->length = 0;
        if(!busy && call AMSend.send(AM_BROADCAST_ADDR,&packet,sizeof(route_message_t)) == SUCCESS ){
        	printf("LEAF SEND");
        	print_route_message(rpkt);
        	busy=TRUE;
        }
        printf("LEAF DEBUG after for loop to_send_node_id:%d last_send_id:%d\n",to_send_node_id,last_send_id);
        refresh_best_father_node();
        print_link_quality_table();
        print_father_node_table();
            //printfflush();
    }

	event void AMControl.stopDone(error_t error) {}

	event message_t* Receive.receive(message_t *msg, 
		void *payload, uint8_t len) {
		route_message_t *rm;
		route_message_t *sm;
		link_quality_entry_t *lqe;
		father_node_table_entry_t *fne;
 		int i;
 		int to_send_node_id;

		rm = (route_message_t *) payload;
		sm = (route_message_t *) call Packet.getPayload(&packet,sizeof(route_message_t));
		lqe = access_link_quality_table(rm->last_hop_addr);


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

		if((rm->type_gradient & 0xf0)>>4 == TYPE_JREQ && self_gradient!=0X0f) {
			printf("LEAF RECV %d",len);
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

		if ((rm->type_gradient & 0xf0)>>4 == TYPE_DATA && rm->next_hop_addr == TOS_NODE_ID) {
			printf("LEAF RECV %d",len);
			print_route_message(rm);
			sm->last_hop_addr = TOS_NODE_ID;
			sm->next_hop_addr = current_best_father_node;
			sm->src_addr = rm->src_addr;
			sm->dst_addr = rm->dst_addr;
			sm->type_gradient = TYPE_DATA<<4 | self_gradient;
			sm->energy_lqi = calc_uniform_energy(self_energy)<<4 | lqe->local_lqi; // TODO 
			sm->pair_addr = lqe->node_id;
			sm->length = rm->length;
			for(i=0;i<sm->length;i++) {
				sm->payload[i] = rm->payload[i];
			}
			if(!busy && call AMSend.send(AM_BROADCAST_ADDR,&packet, sizeof(route_message_t))==SUCCESS) {
				busy = TRUE;
				printf("LEAF SEND");
				print_route_message(sm);
			}
		}


		if((rm->type_gradient & 0x0f) > self_gradient) {
			return msg;
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