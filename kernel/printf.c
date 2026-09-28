#include <stdarg.h>

#include "types.h"

void consputc(int c);

static void
print_unsigned(uint64 value, uint base, int prefix)
{
  static const char digits[] = "0123456789abcdef";
  char buf[32];
  int n = 0;

  if (prefix) {
    consputc('0');
    consputc('x');
  }

  do {
    buf[n++] = digits[value % base];
    value /= base;
  } while (value != 0);

  while (n > 0)
    consputc(buf[--n]);
}

static void
print_decimal(int value)
{
  uint magnitude;

  if (value < 0) {
    consputc('-');
    magnitude = (uint)(-(value + 1)) + 1;
  } else {
    magnitude = (uint)value;
  }
  print_unsigned(magnitude, 10, 0);
}

void
printf(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  for (; *fmt != '\0'; fmt++) {
    if (*fmt != '%') {
      consputc(*fmt);
      continue;
    }

    fmt++;
    if (*fmt == '\0')
      break;

    switch (*fmt) {
    case 'd':
      print_decimal(va_arg(args, int));
      break;
    case 'x':
      print_unsigned(va_arg(args, uint), 16, 1);
      break;
    case 's': {
      const char *s = va_arg(args, const char *);
      if (s == 0)
        s = "(null)";
      while (*s != '\0')
        consputc(*s++);
      break;
    }
    case '%':
      consputc('%');
      break;
    default:
      consputc('%');
      consputc(*fmt);
      break;
    }
  }

  va_end(args);
}

