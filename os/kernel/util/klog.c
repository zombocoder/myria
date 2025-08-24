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

void klog_init(void) {
    // Serial already initialized by serial_init()
}

// Simple printf implementation for kernel
static void kprint_uint(u64 value, int base) {
    if (value == 0) {
        serial_putc('0');
        return;
    }
    
    char buffer[32];
    int i = 0;
    
    while (value > 0) {
        int digit = value % base;
        if (digit < 10) {
            buffer[i++] = '0' + digit;
        } else {
            buffer[i++] = 'a' + digit - 10;
        }
        value /= base;
    }
    
    // Print in reverse order
    while (i > 0) {
        serial_putc(buffer[--i]);
    }
}

static void kprint_int(i64 value) {
    if (value < 0) {
        serial_putc('-');
        value = -value;
    }
    kprint_uint((u64)value, 10);
}

void kprintf(const char *fmt, ...) {
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 'd': {
                    int val = __builtin_va_arg(args, int);
                    kprint_int(val);
                    break;
                }
                case 'u': {
                    unsigned int val = __builtin_va_arg(args, unsigned int);
                    kprint_uint(val, 10);
                    break;
                }
                case 'x': {
                    unsigned int val = __builtin_va_arg(args, unsigned int);
                    kprint_uint(val, 16);
                    break;
                }
                case 'l': {
                    fmt++;
                    if (*fmt == 'u') {
                        u64 val = __builtin_va_arg(args, u64);
                        kprint_uint(val, 10);
                    } else if (*fmt == 'x') {
                        u64 val = __builtin_va_arg(args, u64);
                        kprint_uint(val, 16);
                    }
                    break;
                }
                case 's': {
                    const char *str = __builtin_va_arg(args, const char*);
                    if (str) {
                        serial_puts(str);
                    } else {
                        serial_puts("(null)");
                    }
                    break;
                }
                case '%':
                    serial_putc('%');
                    break;
                default:
                    serial_putc('%');
                    serial_putc(*fmt);
                    break;
            }
        } else {
            serial_putc(*fmt);
        }
        fmt++;
    }
    
    __builtin_va_end(args);
}

void panic(const char *fmt, ...) {
    cli(); // Disable interrupts
    
    serial_puts("\n[PANIC] ");
    
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    
    // Simplified panic printf
    while (*fmt) {
        if (*fmt == '%' && *(fmt + 1) == 's') {
            fmt += 2;
            const char *str = __builtin_va_arg(args, const char*);
            if (str) {
                serial_puts(str);
            } else {
                serial_puts("(null)");
            }
        } else {
            serial_putc(*fmt);
            fmt++;
        }
    }
    
    __builtin_va_end(args);
    serial_puts("\n[PANIC] System halted.\n");
    
    hang();
    __builtin_unreachable();
}