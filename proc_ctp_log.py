#!/usr/bin/env python
f = open("log_ctp_200885.txt", "r")
energy = dict()
for i in range(1,52):
	energy[i]=3000
#print energy[22]
for line in f:
	#send_or_recvprint line
	data=line.split()
	#print data
	time = data[0]
	id=int(data[1][3:])
	type=data[2]

	if type=="CTP":
		send_or_recv=data[4]
		#print send_or_recv
		if(send_or_recv=="SEND_CNT" or send_or_recv=="RECV_CNT"):
			energy[id]=energy[id]-2
			print time,id,energy[id]
	#print time,id,type



