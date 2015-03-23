#ifndef SimpleCollection_H
#define SimpleCollection_H 

enum {
  AM_BEACON = 7,
  AM_CONTROL = 10,
  AM_ROUTE = 11
};

typedef nx_struct BeaconMsg {
  nx_uint16_t nodeid;
  nx_uint16_t counter;
  nx_uint8_t  data[40];
} BeaconMsg;


typedef nx_struct ControlMsg {
	nx_uint16_t srcNodeid;
	nx_uint16_t rootNodeid;
	nx_uint8_t isSource;
} ControlMsg;

#endif