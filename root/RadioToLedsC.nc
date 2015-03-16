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
		interface Timer<TMilli> as SETTING_Timer;
	}
}

implementation {
	bool busy = FALSE;
	message_t packet;
	int self_gradient = 0;
	int self_energy = MAX_ENERGY;
	int send_cnt = 1;
	uint16_t recv_cnt = 0;
	int JREQ_Timer_interval = SEND_SETTING_INTERVAL;
	bool SETTING_Timer_started = FALSE;
	uint16_t self_setting_seq = 0;
	event void Boot.booted() {
		printf("ROOT BOOT\n");
		call AMControl.start();
		//call Leds.led0On();
		//call Leds.led1On();
		//call Leds.led2On();
	}

	event void AMControl.startDone(error_t err) {
		if(err==SUCCESS) {
			init_link_quality_table();
			//good!
		} else {		
			call AMControl.start();

		}
	}

	void sendTestSetting() {
		route_message_t* rpkt;
		self_setting_seq++;
        rpkt = (route_message_t*) call Packet.getPayload(&packet, sizeof(route_message_t));
        rpkt->last_hop_addr = TOS_NODE_ID;
        rpkt->next_hop_addr = 255;
        rpkt->src_addr = TOS_NODE_ID;
        rpkt->dst_addr = 51;
        rpkt->type_gradient = TYPE_SETTING<<4 | self_gradient;
        //rpkt->energy_lqi = calc_uniform_energy(self_energy)<<4 | lqe->local_lqi;
        //rpkt->pair_addr = lqe->node_id;
        rpkt->seq = self_setting_seq;
        rpkt->self_send_cnt = send_cnt;
        rpkt->length = 8;
        rpkt->payload[0] = 's';
        rpkt->payload[1] = 'e';
        rpkt->payload[2] = 't';
        rpkt->payload[3] = 't';
        rpkt->payload[4] = 'i';
        rpkt->payload[5] = 'n';
        rpkt->payload[6] = 'g';
        rpkt->payload[7] = '\0';
        if(!busy && call AMSend.send(AM_BROADCAST_ADDR,&packet,sizeof(route_message_t)) == SUCCESS ){
        	printf("ROOT SEND");
        	print_route_message(rpkt);
        	busy=TRUE;
        }

	}

	event void SETTING_Timer.fired() {
            sendTestSetting();
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

		recv_cnt++;

		if(rm->pair_addr == TOS_NODE_ID && (rm->energy_lqi & 0x0f)!=0) {
				lqe->local_lqi = rm->energy_lqi & 0x0f;
		}

		if(lqe->node_id==0) {
			lqe->node_id = rm->last_hop_addr;
			lqe->recv_cnt = 1;
		} else {
			lqe->recv_cnt++;
		}
		if((rm->type_gradient & 0xf0)>>4 == TYPE_JREQ) {
			printf("ROOT RECV %d",recv_cnt);
			print_route_message(rm);
			sm->last_hop_addr = TOS_NODE_ID;
			sm->next_hop_addr = rm->last_hop_addr;
			sm->src_addr = TOS_NODE_ID;
			sm->dst_addr = rm->src_addr;
			sm->type_gradient = TYPE_JRES<<4 | self_gradient;
			sm->energy_lqi = calc_uniform_energy(self_energy)<<4 | lqe->recv_cnt*0xf/lqe->send_cnt;  // TODO 
			sm->self_send_cnt = send_cnt;
			sm->length = 0;
			sm->pair_addr = lqe->node_id;
			//printf("sm->energy_lqi: %d\n",sm->energy_lqi);
			if(!busy && call AMSend.send(AM_BROADCAST_ADDR,&packet, sizeof(route_message_t))==SUCCESS) {
				busy = TRUE;
				printf("ROOT SEND");
				print_route_message(sm);
			}
		} else if ((rm->type_gradient & 0xf0)>>4 == TYPE_DATA) {
			printf("ROOT RECV %d",recv_cnt);
			print_route_message(rm);
			if(!SETTING_Timer_started) {
				printf("ROOT DEBUG SETTING TIMER call SETTING_Timer.startPeriodic(SEND_SETTING_INTERVAL);\n");
				call SETTING_Timer.startPeriodic(SEND_SETTING_INTERVAL);
				SETTING_Timer_started = TRUE;
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