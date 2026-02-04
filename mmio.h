#ifndef MMIO_H
#define MMIO_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>

static inline int open_physical(int fd) {
    if (fd == -1) {
        fd = open("/dev/mem", O_RDWR | O_SYNC);
        if (fd == -1) {
            perror("open /dev/mem");
            return -1;
        }
    }
    return fd;
}

static inline void close_physical(int fd) {
    if (fd != -1) close(fd);
}

static inline void *map_physical(int fd, unsigned int base, unsigned int span) {
    void *virtual_base = mmap(NULL, span, PROT_READ | PROT_WRITE, MAP_SHARED, fd, base);
    if (virtual_base == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return NULL;
    }
    return virtual_base;
}

static inline int unmap_physical(void *virtual_base, unsigned int span) {
    if (munmap(virtual_base, span) != 0) {
        perror("munmap");
        return -1;
    }
    return 0;
}

#endif
