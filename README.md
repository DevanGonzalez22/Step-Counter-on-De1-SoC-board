Step Counter on DE1-SoC (Embedded Linux + ADXL345 + MMIO):

A simple step-counter demo on the Terasic DE1-SoC developement board that runs on the HPS (ARM) under embedded Linux.  
It reads acceleration data from the on-board ADXL345 accelerometer over I2C, performs lightweight filtering + threshold step detection in C, and writes the live step count to the FPGA-connected LEDs via memory-mapped I/O (/dev/mem).


Highlights:

- Embedded Linux user-space I2C access via `/dev/i2c-0` (`linux/i2c-dev.h`)
- ADXL345 configuration + multi-byte reads (X/Y/Z)
- Simple step detection using magnitude of acceleration vector, exponential moving average baseline (high-pass style), and threshold + minimum time between steps
- HPS -> FPGA communication using lightweight bridge MMIO


Project Files:

- `read_mpu.c` — quick sensor reader/validator (prints raw X/Y/Z)
- `step_detector.c` — step detection via MMIO
- `mmio.h` — helper functions for mapping physical memory (`/dev/mem`)
- `address_map_arm.h` — DE1-SoC lightweight bridge + peripheral offsets
- `Makefile` — builds both binaries

> Note: Despite the filename `read_mpu.c`, the sensor used is ADXL345, not an MPU. I was too lazy to change it.


Requirements: 

Hardware / OS:
- Terasic DE1-SoC
- Embedded Linux running on HPS
- I2C enabled and ADXL345 accessible on `/dev/i2c-0`

Software:
- GCC toolchain on target (or cross-compile)
- Permissions to access `/dev/i2c-0` and `/dev/mem`


Outputs: read_mpu, step_detector


Run:

1) Read accelerometer values (sanity check)
sudo ./read_mpu
Expected startup message includes:
ADXL345 DEVID = 0xE5
Then it prints raw readings:
AX= ... AY= ... AZ= ...

2) Run step detector (prints live debug + updates LEDs)
sudo ./step_detector

Behavior:
- increments step counter when dynamic acceleration exceeds threshold
- writes step_count to LEDR via MMIO
- prints debug lines like:
  -steps=... mag=... base=... dyn=... (x=... y=... z=...)

Stop with:
- Ctrl+C


How Step Detection Works (high level): 
- Read raw X/Y/Z from ADXL345
- Convert to g’s using a scale factor
- Compute magnitude: sqrt(x^2 + y^2 + z^2)
- Track a slow-moving baseline (EMA)
- Use the dynamic component: dyn = mag - baseline
- Count a step when:
  - dyn > THRESHOLD_G
  - at least MIN_INTERVAL_MS since last step (debounce)


Tunable parameters (in step_detector.c):
- FILTER_ALPHA
- THRESHOLD_G
- MIN_INTERVAL_MS


Safety / Notes:

/dev/mem mapping is powerful; only map the needed span and run on trusted images.
This is a teaching/demo implementation (not a production pedometer algorithm). This was done to be an introduction for a future project.


Author: Devan Jesus Gonzalez
