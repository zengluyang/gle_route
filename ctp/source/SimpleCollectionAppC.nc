#include "SimpleCollection.h"
#define USE_PRINT

configuration SimpleCollectionAppC {}
implementation {
	components SimpleCollectionC as App;
	components MainC, ActiveMessageC;
	components CollectionC as Collector;
	components new CollectionSenderC(AM_ROUTE);
	components new AMReceiverC(AM_BEACON) as AMDataReceiverC;
	components new AMReceiverC(AM_CONTROL) as AMControlReceiverC;

	components new AMSenderC(AM_CONTROL) as AMControlSenderC;

	components new TimerMilliC() as DATA_Timer;

	#ifdef USE_PRINT
	components SerialPrintfC;
	components SerialStartC;
	#endif
	App.Boot -> MainC.Boot;
	App.AMControl -> ActiveMessageC;
	App.RoutingControl -> Collector;
	App.AMDataReceive -> AMDataReceiverC;
	App.AMControlReceive -> AMControlReceiverC;
	App.RoutingReceive -> Collector.Receive[AM_ROUTE];
	App.RootControl -> Collector;
	App.AMControlSend -> AMControlSenderC;
	App.RoutingSend -> CollectionSenderC;
	App.DATA_Timer -> DATA_Timer;

}