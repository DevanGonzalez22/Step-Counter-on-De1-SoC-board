#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <stdbool.h>

#define I2C_DEV "/dev/i2c-0"
#define ADXL_ADDR 0x53

#define REG_DEVID       0x00
#define REG_POWER_CTL   0x2D
#define REG_DATA_FORMAT 0x31
#define REG_DATAX0      0x32

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

int main(void) {
    int file = open(I2C_DEV, O_RDWR);
    if (file < 0) {
        perror("open /dev/i2c-0");
        return 1;
    }

    if (ioctl(file, I2C_SLAVE, ADXL_ADDR) < 0) {
        perror("ioctl I2C_SLAVE");
        close(file);
        return 1;
    }

    uint8_t dev_id_reg = REG_DEVID;
    uint8_t dev_id;
    if (write(file, &dev_id_reg, 1) == 1 && read(file, &dev_id, 1) == 1) {
        printf("ADXL345 DEVID = 0x%02X (expect 0xE5)\n", dev_id);
    } else {
        printf("Warning: could not read DEVID\n");
    }

    if (!adxl_write_reg(file, REG_DATA_FORMAT, 0x08)) {
        printf("Failed to write DATA_FORMAT\n");
    }
    if (!adxl_write_reg(file, REG_POWER_CTL, 0x08)) {
        printf("Failed to write POWER_CTL\n");
    }

    printf("Reading ADXL345 acceleration (Ctrl+C to stop)...\n");

    while (1) {
        uint8_t buf[6];
        if (!adxl_read_multi(file, REG_DATAX0, buf, 6)) {
            printf("Failed to read accel data\n");
            usleep(50000);
            continue;
        }

        int16_t x = (int16_t)((buf[1] << 8) | buf[0]);
        int16_t y = (int16_t)((buf[3] << 8) | buf[2]);
        int16_t z = (int16_t)((buf[5] << 8) | buf[4]);

        printf("AX=%6d  AY=%6d  AZ=%6d\n", x, y, z);
        fflush(stdout);
        usleep(50000);
    }

    close(file);
    return 0;
}
