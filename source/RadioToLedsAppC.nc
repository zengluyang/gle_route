#include "../RadioMsg.h"
#define USE_PRINT


configuration RadioToLedsAppC {}
implementation {
	components MainC;
	components RadioToLedsC as App;
	components LedsC;
	components new AMSenderC(MY_AM_ID);
	components new AMReceiverC(MY_AM_ID);
	components ActiveMessageC;
	components new TimerMilliC() as JREQ_Timer;
	components new TimerMilliC() as DATA_Timer;
	#ifdef USE_PRINT
	components SerialPrintfC;
	components SerialStartC;
	#endif
	App.Boot -> MainC.Boot;
	App.Receive -> AMReceiverC;
	App.AMSend -> AMSenderC;
	App.AMControl -> ActiveMessageC;
	App.Leds -> LedsC;
	App.Packet -> AMReceiverC;
	App.JREQ_Timer -> JREQ_Timer;
	App.DATA_Timer -> DATA_Timer;
}