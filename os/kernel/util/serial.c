#include <myria/types.h>
#include <myria/kapi.h>

// Serial port configuration (COM1)
#define SERIAL_COM1_BASE 0x3F8

#define SERIAL_DATA_PORT(base)          (base)
#define SERIAL_FIFO_COMMAND_PORT(base)  (base + 2)
#define SERIAL_LINE_COMMAND_PORT(base)  (base + 3) 
#define SERIAL_MODEM_COMMAND_PORT(base) (base + 4)
#define SERIAL_LINE_STATUS_PORT(base)   (base + 5)

// Line status bits
#define SERIAL_LINE_ENABLE_DLAB 0x80

void serial_init(void) {
    // Disable all interrupts
    outb(SERIAL_COM1_BASE + 1, 0x00);
    
    // Enable DLAB (set baud rate divisor)
    outb(SERIAL_LINE_COMMAND_PORT(SERIAL_COM1_BASE), SERIAL_LINE_ENABLE_DLAB);
    
    // Set divisor to 3 (lo byte) 38400 baud
    outb(SERIAL_DATA_PORT(SERIAL_COM1_BASE), 0x03);
    outb(SERIAL_COM1_BASE + 1, 0x00);  // (hi byte)
    
    // 8 bits, no parity, one stop bit
    outb(SERIAL_LINE_COMMAND_PORT(SERIAL_COM1_BASE), 0x03);
    
    // Enable FIFO, clear them, with 14-byte threshold
    outb(SERIAL_FIFO_COMMAND_PORT(SERIAL_COM1_BASE), 0xC7);
    
    // IRQs enabled, RTS/DSR set
    outb(SERIAL_MODEM_COMMAND_PORT(SERIAL_COM1_BASE), 0x0B);
}

static int serial_is_transmit_fifo_empty(u16 com) {
    return inb(SERIAL_LINE_STATUS_PORT(com)) & 0x20;
}

void serial_putc(char c) {
    while (serial_is_transmit_fifo_empty(SERIAL_COM1_BASE) == 0);
    outb(SERIAL_COM1_BASE, c);
}

void serial_puts(const char *str) {
    while (*str) {
        serial_putc(*str);
        str++;
    }
}