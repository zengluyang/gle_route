all:root/build/ leaf/build/ source/build/
root/build/:root/*.nc RadioMsg.h
	cd root && make telosb
leaf/build/:leaf/*.nc RadioMsg.h
	cd leaf && make telosb
source/build/:source/*.nc RadioMsg.h
	cd source && make telosb
.PHONY : clean
clean:
	rm -rf root/build
	rm -rf leaf/build
	rm -rf source/build