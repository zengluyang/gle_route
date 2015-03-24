#!/usr/bin/env python
f = open("log_ctp_23624.txt", "r")
for line in f:
	#print line
	data=line.split()
	#print data
	time = data[0]
	id=data[1][3:]
	type=data[2]
	print time,id,type

