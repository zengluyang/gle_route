COMPONENT = SimpleCollectionAppC

CFLAGS += -I$(TOSDIR)/lib/printf \
			-I$(TOSDIR)/lib/net \
			-I$(TOSDIR)/lib/net/ctp \
			-I$(TOSDIR)/lib/net/4bitle 


CFLAGS += -DTOSH_DATA_LENGTH=114 
# channels from 11 to 26
CFLAGS += -DCC2420_DEF_CHANNEL=20 
# transmission power from 1 to 31
CFLAGS += -DCC2420_DEF_RFPOWER=1 

include $(MAKERULES)