#ifndef KERNEL_IO_H
#define KERNEL_IO_H

#include "types.h"

static inline uint8_t port_inb(uint16_t port) {
  uint8_t result;
  __asm__ volatile("inb %w1, %b0" : "=a"(result) : "Nd"(port));
  return result;
}

static inline uint16_t port_inw(uint16_t port) {
  uint16_t result;
  __asm__ volatile("inw %w1, %w0" : "=a"(result) : "Nd"(port));
  return result;
}

static inline uint32_t port_inl(uint16_t port) {
  uint32_t result;
  __asm__ volatile("inl %w1, %e0" : "=a"(result) : "Nd"(port));
  return result;
}

// Запись в порты
static inline void port_outb(uint8_t value, uint16_t port) {
  __asm__ volatile("outb %b0, %w1" : : "a"(value), "Nd"(port));
}

static inline void port_outw(uint16_t value, uint16_t port) {
  __asm__ volatile("outw %w0, %w1" : : "a"(value), "Nd"(port));
}

static inline void port_outl(uint32_t value, uint16_t port) {
  __asm__ volatile("outl %e0, %w1" : : "a"(value), "Nd"(port));
}

#endif
