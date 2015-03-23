#include "Timer.h"
#include "printf.h"
#include "SimpleCollection.h"

#define MAX_TABLE_LEN 256
#define RECV_NODE_ID 1
module SimpleCollectionC {
	uses interface Boot;
	uses interface SplitControl as AMControl;
	uses interface StdControl as RoutingControl;
	uses interface AMSend as AMControlSend;
	uses interface Send as RoutingSend;
	uses interface RootControl;
	uses interface Receive as AMControlReceive;
	uses interface Receive as AMDataReceive;
	uses interface Receive as RoutingReceive;
  uses interface Timer<TMilli> as DATA_Timer;
}

implementation {
	message_t ControlPacket;
	message_t RoutingPacket;
	bool busy = FALSE;
	uint8_t isSource[MAX_TABLE_LEN] = {0};
	bool controlSent = FALSE;
	//uint8_t isLocked[MAX_TABLE_LEN][2] = {0};
  int send_cnt=1;
  int recv_cnt=0;

event void DATA_Timer.fired() {
  BeaconMsg* bmToSend;
  if(TOS_NODE_ID == 51) {
    bmToSend = (BeaconMsg*) call RoutingSend.getPayload(&RoutingPacket, sizeof(BeaconMsg));
    if(!busy) {
        if(call RoutingSend.send(&RoutingPacket, sizeof(BeaconMsg)) == SUCCESS) {
          printf("CTP_SOURCE SEND DATA %d\n",send_cnt);
          busy = TRUE;
        }
    }
  }
}

	event void Boot.booted(){
		call AMControl.start();
		printf("SimpleCollectionC BOOT\n");
		//printf("SimpleCollectionC AM_BEACON %d\n",AM_BEACON);
		printfflush();
	}

	event void AMControl.startDone(error_t err) {
	    if (err == SUCCESS) {
	    	call RoutingControl.start();
        call DATA_Timer.startPeriodic(9500);
	    	if(TOS_NODE_ID==RECV_NODE_ID) {
	    		call RootControl.setRoot();
	    	}
	      	
	    } else {
	    	call AMControl.start();
	    }
  	}


  	event void AMControl.stopDone(error_t err) {}

  	event void AMControlSend.sendDone(message_t *m, error_t err) {
  		if(err == SUCCESS) {
  			busy = FALSE;
  		}
  	}


  	event void RoutingSend.sendDone(message_t *m, error_t err){
  		if(err == SUCCESS) {
  			busy = FALSE;
        send_cnt++;
  		} else {

  		}
  	}

  	event message_t* AMDataReceive.receive(message_t *msg, 
		void *payload, uint8_t len) {
  		return msg;
  	}

  	event message_t* AMControlReceive.receive(message_t *msg, 
		void *payload, uint8_t len) {
      /*
  		ControlMsg* cm = (ControlMsg*) payload;
  		if(len == sizeof(ControlMsg)) {
  			printf("SimpleCollectionC RECV_CTRL %d %d %d\n", cm->srcNodeid, cm->rootNodeid, cm->isSource);
  			isSource[cm->rootNodeid] = cm->isSource;
  		}
  		printfflush();
  		return msg;
      */
  	}

  	event message_t* RoutingReceive.receive(message_t *msg, 
		void *payload, uint8_t len) {
  		BeaconMsg* bm = (BeaconMsg*) payload;
  		if(len == sizeof(BeaconMsg) && call	RootControl.isRoot()) {
        recv_cnt++;
  			printf("CTP_ROOT RECV DATA %d\n",recv_cnt);
  		}
  		return msg;
  	}


}