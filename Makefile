CC = gcc
CFLAGS = -O2 -Wall

all: read_mpu step_detector

read_mpu: read_mpu.c
	$(CC) $(CFLAGS) read_mpu.c -o read_mpu

step_detector: step_detector.c
	$(CC) $(CFLAGS) step_detector.c -o step_detector -lm -lrt

clean:
	rm -f read_mpu step_detector
