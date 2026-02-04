#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include "address_map_arm.h"
#include "mmio.h"

#define I2C_DEV "/dev/i2c-0"
#define ADXL_ADDR 0x53

#define REG_DEVID       0x00
#define REG_POWER_CTL   0x2D
#define REG_DATA_FORMAT 0x31
#define REG_DATAX0      0x32

#define LSB_PER_G       256.0f
#define FILTER_ALPHA    0.01f
#define THRESHOLD_G     0.15f
#define MIN_INTERVAL_MS 250

static bool adxl_write_reg(int file, uint8_t reg, uint8_t value) {
    uint8_t buf[2] = { reg, value };
    return write(file, buf, 2) == 2;
}

static bool adxl_read_multi(int file, uint8_t start_reg, uint8_t *data, uint8_t len) {
    if (write(file, &start_reg, 1) != 1)
        return false;
    if (read(file, data, len) != len)
        return false;
    return true;
}

static bool adxl_read_xyz(int file, int16_t *x, int16_t *y, int16_t *z) {
    uint8_t buf[6];
    if (!adxl_read_multi(file, REG_DATAX0, buf, 6))
        return false;

    *x = (int16_t)((buf[1] << 8) | buf[0]);
    *y = (int16_t)((buf[3] << 8) | buf[2]);
    *z = (int16_t)((buf[5] << 8) | buf[4]);
    return true;
}

static unsigned long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long)(ts.tv_sec * 1000UL + ts.tv_nsec / 1000000UL);
}

int main(void) {
    int i2c_fd = open(I2C_DEV, O_RDWR);
    if (i2c_fd < 0) {
        perror("open /dev/i2c-0");
        return 1;
    }

    if (ioctl(i2c_fd, I2C_SLAVE, ADXL_ADDR) < 0) {
        perror("ioctl I2C_SLAVE");
        close(i2c_fd);
        return 1;
    }

    uint8_t reg = REG_DEVID;
    uint8_t dev_id = 0;
    if (write(i2c_fd, &reg, 1) == 1 && read(i2c_fd, &dev_id, 1) == 1) {
        printf("ADXL345 DEVID = 0x%02X (expect 0xE5)\n", dev_id);
    } else {
        printf("Warning: could not read DEVID\n");
    }

    if (!adxl_write_reg(i2c_fd, REG_DATA_FORMAT, 0x08)) {
        printf("Failed to write DATA_FORMAT\n");
    }
    if (!adxl_write_reg(i2c_fd, REG_POWER_CTL, 0x08)) {
        printf("Failed to write POWER_CTL\n");
    }

    int mem_fd = open_physical(-1);
    if (mem_fd == -1) {
        printf("Failed to open /dev/mem\n");
        close(i2c_fd);
        return 1;
    }

    void *lw = map_physical(mem_fd, LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
    if (!lw) {
        printf("Failed to mmap LW bridge\n");
        close_physical(mem_fd);
        close(i2c_fd);
        return 1;
    }

    volatile uint32_t *led = (uint32_t *)((char *)lw + LEDR_BASE);

    float baseline = 1.0f;
    int step_count = 0;
    unsigned long last_step_ms = 0;
    unsigned long last_print_ms = now_ms();

    printf("Starting step detector (Ctrl+C to stop)...\n");

    while (1) {
        int16_t xr, yr, zr;
        if (!adxl_read_xyz(i2c_fd, &xr, &yr, &zr)) {
            printf("Failed to read XYZ\n");
            usleep(20000);
            continue;
        }

        float xg = xr / LSB_PER_G;
        float yg = yr / LSB_PER_G;
        float zg = zr / LSB_PER_G;

        float mag = sqrtf(xg * xg + yg * yg + zg * zg);

        baseline = (1.0f - FILTER_ALPHA) * baseline + FILTER_ALPHA * mag;
        float dyn = mag - baseline;

        unsigned long t = now_ms();

        if (dyn > THRESHOLD_G && (t - last_step_ms) > MIN_INTERVAL_MS) {
            step_count++;
            last_step_ms = t;
            *led = step_count;
        }

        if (t - last_print_ms >= 200) {
            printf("t=%lu ms  steps=%d  mag=%.3f  base=%.3f  dyn=%.3f  (x=%.3f y=%.3f z=%.3f)\n",
                   t, step_count, mag, baseline, dyn, xg, yg, zg);
            fflush(stdout);
            last_print_ms = t;
        }

        usleep(20000);
    }

    unmap_physical(lw, LW_BRIDGE_SPAN);
    close_physical(mem_fd);
    close(i2c_fd);
    return 0;
}
