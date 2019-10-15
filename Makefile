CROSS-COMPILE:=arm-linux-gnueabihf-
build:
	${CROSS-COMPILE}gcc -Wall -g -D_DEFAULT_SOURCE -std=c99 -Iheaders/ *.c protocols/*.c -o ping

