

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
        rpkt->next_hop_addr = 38;
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

            //printfflush();
    }

	event void AMControl.stopDone(error_t error) {}

	event message_t* Receive.receive(message_t *msg, 
		void *payload, uint8_t len) {
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