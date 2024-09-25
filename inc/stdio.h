#ifndef JOS_INC_STDIO_H
#define JOS_INC_STDIO_H

#include <inc/stdarg.h>

#ifndef NULL
#define NULL ((void *)0)
#endif /* !NULL */

// lib/console.c
void cputchar(int c);
int getchar(void);
int iscons(int fd);

// lib/printfmt.c
void printfmt(void (*putch)(int, void *), void *putdat, const char *fmt, ...);
void vprintfmt(void (*putch)(int, void *), void *putdat, const char *fmt, va_list);
int snprintf(char *str, int size, const char *fmt, ...);
int vsnprintf(char *str, int size, const char *fmt, va_list);

// lib/printf.c
int cprintf(const char *fmt, ...);
int vcprintf(const char *fmt, va_list);

// lib/fprintf.c
int printf(const char *fmt, ...);
int fprintf(int fd, const char *fmt, ...);
int vfprintf(int fd, const char *fmt, va_list);

// lib/readline.c
char *readline(const char *prompt);

// Color enum
enum
{
    COLOR_BLACK = 0,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_MAGENTA,
    COLOR_CYAN,
    COLOR_WHITE,
    COLOR_NUM,
};

#endif /* !JOS_INC_STDIO_H */
