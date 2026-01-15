#include "types.h"

#define STATUS_REGISTER 0x64
#define DATA_PORT 0x60
#define VGA_CTRL_PORT 0x3D4
#define VGA_DATA_PORT 0x3D5
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_ADDRESS 0xB8000
#define BACKSPACE_KEY 0x8E
#define VGA_COLOR_YELLOW 0xE
#define VGA_COLOR_WHITE 0xF
#define VGA_COLOR_BLACK 0x0
#define VGA_COLOR_BLUE 0x1
#define VGA_COLOR_GREEN 0x2
#define NULL ((void *)0)
// #define COLOR_BLACK 0
// #define COLOR_LIGHT_GREY 7
// #define COLOR_WHITE 15
char *vidmem = (char *)VGA_ADDRESS;
uint8_t terminal_color = 0x07;

int cursor_x = 0;
int cursor_y = 0;

// Assembler functions

static inline unsigned char inb(unsigned short port) {
  unsigned char result;
  __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
  return result;
}

static inline void outb(uint16_t port, uint8_t val) {
  __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

void set_terminal_color(uint8_t fg, uint8_t bg) {
  terminal_color = (bg << 4) | (fg & 0x0F);
}

void update_cursor() {
  uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;

  outb(0x3D4, 0x0F);
  outb(0x3D5, (uint8_t)(pos & 0xFF));
  outb(0x3D4, 0x0E);
  outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void DeleteChar() {

  if (cursor_x > 0) {
    cursor_x--;
    int offset = (cursor_y * VGA_WIDTH + cursor_x) * 2;
    vidmem[offset] = ' ';
    vidmem[offset + 1] = terminal_color;
  } else if (cursor_y > 0) {
    cursor_y--;
    cursor_x = VGA_WIDTH - 1;
    int offset = (cursor_y * VGA_WIDTH + cursor_x) * 2;
    vidmem[offset] = ' ';
    vidmem[offset + 1] = terminal_color;
  }
}

void print_char(char c) {
  if (c == '\n') {
    cursor_x = 0;
    cursor_y++;
    update_cursor();
  } else if (c == '\b') {
    DeleteChar();
    update_cursor();

  } else if (cursor_y > 0 && c == '\b') {
    DeleteChar();
    update_cursor();

  } else {
    if (cursor_x > VGA_WIDTH) {
      cursor_x = 0;
      cursor_y++;
      update_cursor();
    }
    int offset = (cursor_y * VGA_WIDTH + cursor_x) * 2;
    vidmem[offset] = c;
    vidmem[offset + 1] = terminal_color;
    cursor_x++;
    update_cursor();
  }
  if (cursor_y >= VGA_HEIGHT) {
    cursor_y = VGA_HEIGHT - 1;
  }
}
void print_string(char *str) {
  for (int i = 0; str[i] != '\0'; i++) {
    print_char(str[i]);
  }
}

void clean_screen() {
  unsigned int i = 0, j = 0;

  while (j < VGA_WIDTH * VGA_HEIGHT * 2) {
    vidmem[j] = ' ';
    vidmem[j + 1] = 0x07;
    j += 2;
  }
}

static const char scan_code_table[128] = {
    // 0x00-0x0F
    0,
    0,
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    '0',
    '-',
    '=',
    '\b',
    '\t',

    // 0x10-0x1F
    'q',
    'w',
    'e',
    'r',
    't',
    'y',
    'u',
    'i',
    'o',
    'p',
    '[',
    ']',
    '\n',
    0,
    'a',
    's',

    // 0x20-0x2F
    'd',
    'f',
    'g',
    'h',
    'j',
    'k',
    'l',
    ';',
    '\'',
    '`',
    0,
    '\\',
    'z',
    'x',
    'c',
    'v',

    // 0x30-0x3F
    'b',
    'n',
    'm',
    ',',
    '.',
    '/',
    0,
    '*',
    0,
    ' ',
    0,
    0,
    0,
    0,
    0,
    0,

    // 0x40-0x4F
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    '7',
    '8',
    '9',
    '-',
    '4',
    '5',
    '6',
    '+',
    '1',

    // 0x50-0x5F
    '2',
    '3',
    '0',
    '.',
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,

};

char scancode_to_ascii(uint8_t scancode) {

  if (scancode & 0x80)
    return 0;
  if (scancode >= 128)
    return 0;

  return scan_code_table[scancode];
}

// console/terminal/shell

typedef void (*command_handler_t)(int argc, char **argv);

typedef struct {
  const char *name;
  const char *description;
  command_handler_t handler;
} command_t;

int tokinaze(char *str, char **argv) {
  int argc = 0;
  while (*str) {
    while (*str == ' ') {
      *str++ = '\0';
    }
    if (*str == '\n')
      break;
    argv[argc++] = str;

    while (*str != ' ' && *str != '\0') {
      str++;
    }
  }
  return argc;
}
int strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

// clear screen cmd func
void cmd_clear(int argc, char **argv) { clean_screen(); }
void cmd_help(int argc, char **argv);
void cmd_echo(int argc, char **argv);

command_t cmd_table[] = {{"help", "show all commands", cmd_help},
                         {"clear", "clear screen", cmd_clear},
                         {"echo", "printing text to screen", cmd_echo},
                         {NULL, NULL, NULL}};

// parser
void shell_execute(char *input) {
  char *argv[16];
  int argc = tokinaze(input, argv);
  if (argc == 0)
    return;

  for (int i = 0; cmd_table[i].name != NULL; i++) {
    if (strcmp(argv[0], cmd_table[i].name) == 0) {
      cmd_table[i].handler(argc, argv);
      return;
    }
  }
  print_string("error:unknow command: ");
  print_string(argv[0]);
  print_string("\nUse 'help' for view command list\n");
}

void cmd_echo(int argc, char **argv) {
  for (int i = 1; i < argc; i++) {
    print_string(argv[i]);
    print_string(" ");
    print_char('\n');
  }
}

void cmd_help(int argc, char **argv) {
  char *help_str = "Available commands: echo,help,clear\n";
  print_string(help_str);
}

char get_char() {
  unsigned char scancode;
  while (1) {
    if (inb(STATUS_REGISTER) & 0x01) {
      scancode = inb(DATA_PORT);
      if (!(scancode & 0x80)) {
        char c = scancode_to_ascii(scancode);
        if (c > 0) {
          return c;
        }
      }
    }
  }
}

char *read_line(char *buffer, int max_len) {
  int i = 0;
  char c;
  while (1) {
    c = get_char();
    if (c == '\n' || c == '\r') {
      buffer[i] = '\0';
      print_char('\n');
      update_cursor();
      return buffer;
    }
    if (c == '\b') {
      if (i > 0) {
        i--;
        DeleteChar();
        update_cursor();
      }
      continue;
    }
    if (i < max_len - 1) {
      if (c >= 32 && c <= 126) {
        buffer[i] = c;
        i++;
        print_char(c);
        update_cursor();
      }
    }
  }
}

void os_main(void) {
  char *buffer;
  clean_screen();
  print_string("KeprOS is running!\n");

  while (1) {
    print_string("root@keprOS> ");
    char *line = read_line(buffer, 1000000);
    shell_execute(line);
  }
}
