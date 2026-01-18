#include "disk_driver.h"
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
#define MAX_FILES 3
#define MAX_FILENAME 32
#define BLOCK_SIZE 512
#define SECTOR_SIZE 512
#include "types.h"

#define ATA_PORT_DATA 0x1F
#define ATA_PORT_ERROR 0x1F1
#define ATA_PORT_FEATURES 0x1F1
#define ATA_PORT_SECTOR_COUNT 0x1F2
#define ATA_PORT_LBA_LOW 0x1F3
#define ATA_PORT_LBA_MID 0x1F4
#define ATA_PORT_LBA_HIGH 0x1F5
#define ATA_PORT_DEVICE 0x1F6
#define ATA_PORT_COMMAND 0x1F7
#define ATA_PORT_STATUS 0x1F7

#define ATA_STATUS_BUSY 0x80
#define ATA_STATUS_DRQ 0x08
#define ATA_STATUS_ERR 0x01

// VGA DRIVER INIT
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

// hard drive basic functions
void ata_wait() {
  while (inb(ATA_PORT_STATUS) && ATA_STATUS_BUSY)
    ;
}

void ata_read(uint32_t lba, uint8_t *buffer, uint32_t sector_count) {
  ata_wait();

  for (uint32_t i = 0; i < sector_count; i++) {
    ata_wait();
    uint16_t *ptr = (uint16_t *)buffer;
    for (int j = 0; j < 256; j++) {
      *ptr++ = inb(ATA_PORT_DATA);
    }
    buffer += 512;
  }
}

void ata_write(uint32_t lba, uint8_t *buffer, uint32_t sector_count) {
  ata_wait();

  outb(ATA_PORT_SECTOR_COUNT, sector_count);
  outb(ATA_PORT_LBA_LOW, lba & 0xFF);
  outb(ATA_PORT_LBA_MID, (lba >> 8) & 0xFF);
  outb(ATA_PORT_LBA_HIGH, (lba >> 16) & 0xFF);
  outb(ATA_PORT_DEVICE, 0xE0 | ((lba >> 24) & 0x0F));

  ata_wait();

  for (uint32_t i = 0; i < sector_count; i++) {

    ata_wait();
    const uint16_t *ptr = (const uint16_t *)buffer;
    for (int j = 0; j < 256; j++) {
      outb(ATA_PORT_DATA, *ptr++);
    }
    buffer += SECTOR_SIZE;
  }
}

int ata_init() {
  ata_wait();
  outb(ATA_PORT_DEVICE, 0xA0);
  outb(ATA_PORT_COMMAND, 0xEC);

  ata_wait();

  if (inb(ATA_PORT_STATUS) == 0) {
    return -1;
  }
  return 0;
}

// basic functions

char *strcpy(char *dest, const char *src) {
  char *saved = dest;
  while ((*dest++ = *src++) != '\0') {
  }
  return saved;
}

size_t str_len(const char *str) {
  const char *s = str;
  while (*s) {
    s++;
  }
  return (size_t)(s - str);
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

void *mem_cpy(void *dest, void *src, size_t n) {
  unsigned char *d = (unsigned char *)dest;
  const unsigned char *s = (const unsigned char *)src;

  while (n--) {
    *d++ = *s++;
  }
  return dest;
}

// File system(RAM)
struct File {
  char name[MAX_FILENAME];
  int size;
  char data[MAX_FILES];
  int is_used;
};

struct File filesystem[MAX_FILES];

void fs_init() {
  for (int i = 0; i < MAX_FILES; i++) {
    filesystem->is_used = 0;
    filesystem->size = 0;
  }
}

int fs_find_file(char *name) {
  for (int i = 0; i < MAX_FILES; i++) {
    if (filesystem[i].is_used && !(strcmp(filesystem[i].name, name))) {
      return i;
    }
  }
  return -1;
}

int fs_create_file(char *name) {
  if (fs_find_file(name) != -1) {
    return -2;
  }
  for (int i = 0; i < MAX_FILES; i++) {
    if (!filesystem[i].is_used) {
      strcpy(filesystem[i].name, name);
      filesystem[i].size = 0;
      filesystem[i].is_used = 1;
      return i;
    }
  }
  return -1;
}

int fs_delete_file(int index) {

  filesystem[index].is_used = 0;
  filesystem[index].size = 0;
  filesystem[index].name[0] = '\0';
  for (int i = 0; i < BLOCK_SIZE; i++) {
    filesystem[index].data[i] = 0;
  }
  return 0;
}

int fs_write_file(char *name, char *content) {
  int index = fs_find_file(name);
  if (index == -1)
    index = fs_create_file(name);
  if (index >= 0) {
    int len = str_len(content);
    if (len > BLOCK_SIZE)
      len = BLOCK_SIZE;
    mem_cpy(filesystem[index].data, content, len);
    filesystem[index].size = len;
    return 0;
  }
  return -1;
}

char *fs_read_file(char *name) {
  int index = fs_find_file(name);
  if (index != -1) {
    return filesystem[index].data;
  }
  return 0;
}

void fs_list_files(char arr[MAX_FILES][MAX_FILENAME]) {
  int file_index = 0;

  for (int i = 0; i < MAX_FILES; i++) {
    arr[i][0] = '\0';
  }

  for (int i = 0; i < MAX_FILES; i++) {
    if (filesystem[i].is_used) {
      strcpy(arr[file_index], filesystem[i].name);
      file_index++;
    }
  }
}

// Keyboard functions
struct keyboard_state {
  uint8_t left_shift_pressed : 1;
  uint8_t right_shift_pressed : 1;
  uint8_t capslock_pressed : 1;
  uint8_t ctr_pressed : 1;
  uint8_t alt_pressed : 1;
};

struct keyboard_state kdb_state = {0};
;

static const char scan_code_table_normal[128] = {
    [0x02] = '1',  [0x03] = '2',  [0x04] = '3', [0x05] = '4', [0x06] = '5',
    [0x07] = '6',  [0x08] = '7',  [0x09] = '8', [0x0A] = '9', [0x0B] = '0',
    [0x0C] = '-',  [0x0D] = '=',  [0x10] = 'q', [0x11] = 'w', [0x12] = 'e',
    [0x13] = 'r',  [0x14] = 't',  [0x15] = 'y', [0x16] = 'u', [0x17] = 'i',
    [0x18] = 'o',  [0x19] = 'p',  [0x1A] = '[', [0x1B] = ']', [0x1E] = 'a',
    [0x1F] = 's',  [0x20] = 'd',  [0x21] = 'f', [0x22] = 'g', [0x23] = 'h',
    [0x24] = 'j',  [0x25] = 'k',  [0x26] = 'l', [0x27] = ';', [0x28] = '\'',
    [0x29] = '`',  [0x2B] = '\\', [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c',
    [0x2F] = 'v',  [0x30] = 'b',  [0x31] = 'n', [0x32] = 'm', [0x33] = ',',
    [0x34] = '.',  [0x35] = '/',
    [0x39] = ' ',  // Пробел
    [0x1C] = '\n', // Enter
    [0x0E] = '\b', // Backspace
    [0x0F] = '\t', // Tab
};

// Таблица для символов с нажатым Shift
static const char scan_code_table_shift[128] = {
    [0x02] = '!', [0x03] = '@', [0x04] = '#', [0x05] = '$', [0x06] = '%',
    [0x07] = '^', [0x08] = '&', [0x09] = '*', // Shift+8 = *
    [0x0A] = '(', [0x0B] = ')', [0x0C] = '_', [0x0D] = '+', [0x10] = 'Q',
    [0x11] = 'W', [0x12] = 'E', [0x13] = 'R', [0x14] = 'T', [0x15] = 'Y',
    [0x16] = 'U', [0x17] = 'I', [0x18] = 'O', [0x19] = 'P', [0x1A] = '{',
    [0x1B] = '}', [0x1E] = 'A', [0x1F] = 'S', [0x20] = 'D', [0x21] = 'F',
    [0x22] = 'G', [0x23] = 'H', [0x24] = 'J', [0x25] = 'K', [0x26] = 'L',
    [0x27] = ':', [0x28] = '"', [0x29] = '~', [0x2B] = '|', [0x2C] = 'Z',
    [0x2D] = 'X', [0x2E] = 'C', [0x2F] = 'V', [0x30] = 'B', [0x31] = 'N',
    [0x32] = 'M', [0x33] = '<', [0x34] = '>', [0x35] = '?',
};

char scancode_to_ascii(uint8_t scancode) {

  if (scancode & 0x80 || scancode >= 128)
    return 0;
  int shift_active =
      kdb_state.left_shift_pressed || kdb_state.right_shift_pressed;

  if (scancode >= 0x10 && scancode <= 0x32) {
    char base_char = scan_code_table_normal[scancode];
    if (base_char >= 'a' && base_char <= 'z') {
      int uppercase = shift_active ^ kdb_state.capslock_pressed;
      if (uppercase) {
        return base_char - 32;
      } else {
        return base_char;
      }
    }
  }
  const char *active_table;
  if (shift_active) {
    active_table = scan_code_table_shift;
  } else {
    active_table = scan_code_table_normal;
  }

  return active_table[scancode];
}

// Screen functions

void scroll_screen();
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
    scroll_screen();
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
  cursor_x = 0;
  cursor_y = 0;
}

void scroll_screen() {

  for (int y = 1; y < VGA_HEIGHT; y++) {
    for (int x = 0; x < VGA_WIDTH; x++) {
      int offset_from = (y * VGA_WIDTH + x) * 2;
      int offset_to = ((y - 1) * VGA_WIDTH + x) * 2;
      vidmem[offset_to] = vidmem[offset_from];
      vidmem[offset_to + 1] = vidmem[offset_from + 1];
    }
  }
  int last_row_start = (VGA_HEIGHT - 1) * VGA_WIDTH * 2;
  for (int x = 0; x < VGA_WIDTH; x++) {
    vidmem[last_row_start + x * 2] = ' ';
    vidmem[last_row_start + x * 2 + 1] = terminal_color;
  }

  if (cursor_y == VGA_HEIGHT) {
    cursor_y = VGA_HEIGHT - 1;
    cursor_x = 0;
    update_cursor();
  }
}

// hard drive struct
typedef struct {
  char name[256];
  uint32_t size;
  uint32_t start_block;
  uint32_t attributes;
} FileEntry;

typedef struct {
  FileEntry entries[128];
  uint32_t entry_count;
} Directory;

typedef struct {
  Directory root;
  uint32_t total_blocks;
  uint32_t free_blocks;
} FileSystem;

// hard drive driver functions

FileEntry *fs_find_file_new(FileSystem *fs, const char *filename) {
  for (uint32_t i = 0; i < fs->root.entry_count; i++) {
    if (strcmp(fs->root.entries[i].name, filename)) {
      return &fs->root.entries[i];
    }
  }
  return NULL;
}

void fs_read(FileSystem *fs, const char *filename, const uint8_t *buffer,
             uint32_t size) {
  FileEntry *file = fs_find_file_new(fs, filename);
  if (file == NULL) {
    print_string("not found file\n");
    return;
  }
  size = file->size;

  static int ata_initialized = 0;
  if (!ata_initialized) {
    if (ata_init() != 0) {
      print_string("error ata init\n");
      return;
    }
    ata_initialized = 1;
  }
  uint32_t sector_count = (size + SECTOR_SIZE - 1) / SECTOR_SIZE;
  ata_read(file->start_block, (uint8_t *)buffer, sector_count);
}
void fs_write(FileSystem *fs, const char *filename, const uint8_t *data,
              uint32_t size) {
  FileEntry *file = fs_find_file_new(fs, filename);
  if (file == NULL) {
    print_string("error: file not found\n");
    return;
  }
  if (size > SECTOR_SIZE) {
    print_string("error data size exceeds sector size\n");
    return;
  }
  file->size = size;
  // ata_write(file->start_block);

  uint8_t buffer[SECTOR_SIZE] = {0};
  mem_cpy(buffer, (char *)data, size);
  uint32_t lba = 0;
  uint32_t sector_count = 1;

  static int ata_initialized = 0;
  if (!ata_init()) {
    if (ata_init() != 0) {
      print_string("ошибка инициализации ata драйвера");
      return;
    }
    ata_initialized = 1;
  }
  ata_write(file->start_block, buffer, 1);
  file->size = size;
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
  int is_quote = 0;

  while (*str) {
    while (*str == ' ') {
      *str++ = '\0';
    }
    if (*str == '"') {
      is_quote = 1;
      str++;
    }
    if (!is_quote && *str == '\0') {
      break;
    }
    argv[argc++] = str;

    if (is_quote) {
      while (*str && *str != '"') {
        str++;
      }
      if (*str == '"') {
        *str++ = '\0';
        is_quote = 0;
      }
    } else {
      while (*str && *str != ' ') {
        str++;
      }
      if (*str) {
        *str++ = '\0';
      }
    }
  }

  argv[argc] = NULL;
  return argc;
}
// cmd functions templates
void cmd_clear(int argc, char **argv) { clean_screen(); }
void cmd_help(int argc, char **argv);
void cmd_echo(int argc, char **argv);
void cmd_cat(int argc, char **argv);
void cmd_write(int argc, char **argv);
void cmd_touch(int argc, char **argv);
void cmd_ls(int argc, char **argv);
void cmd_rm(int argc, char **argv);

command_t cmd_table[] = {{"help", "show all commands", cmd_help},
                         {"clear", "clear screen", cmd_clear},
                         {"echo", "printing text to screen", cmd_echo},
                         {"cat", "read the file", cmd_cat},
                         {"write", "write data in file", cmd_write},
                         {"touch", "creating new file", cmd_touch},
                         {"ls", "list all files", cmd_ls},
                         {"rm", "remove(delete) file", cmd_rm},
                         {NULL, NULL, NULL}};

// cmd functions full

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

void cmd_write(int argc, char **argv) {
  if (argc < 2) {
    print_string("need date to write(write <filename> <data>\n)");
    return;
  }
  int result_code = fs_write_file(argv[1], argv[2]);
  if (result_code == -1) {
    print_string("\nerror write data in file\n");
    print_string(argv[1]);
    print_char('\n');
  } else {
    print_string("\n data was successfully writed in file ");
    print_string(argv[1]);
    print_char('\n');
  }
}

void cmd_cat(int argc, char **argv) {
  char *date = fs_read_file(argv[1]);
  print_char('\n');
  print_string(date);
  print_char('\n');
}

void cmd_touch(int argc, char **argv) {
  if (argc < 2) {
    print_string("need file name(touch <file_name>\n");
    return;
  }
  int result_code = fs_create_file(argv[1]);
  if (result_code != -2) {
    print_string("\nfile ");
    print_string(argv[1]);
    print_string(" successfully created!\n");
  } else {
    print_string("file aldery exist");
  }
}

void cmd_rm(int argc, char **argv) {
  if (argc < 2) {
    print_string("need file name(rm <filename>\n");
  }
  int index = fs_find_file(argv[1]);
  if (index == -1) {
    print_string("error: file not exist");
  } else {
    fs_delete_file(index);
    print_string("file successfully deleted");
  }
}

void cmd_ls(int argc, char **argv) {
  char names[MAX_FILES][MAX_FILENAME];
  fs_list_files(names);

  for (int i = 0; i < MAX_FILES; i++) {
    if (names[i][0] != '\0') {
      print_string(names[i]);
      print_char('\n');
    }
  }
}

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

char get_char() {
  unsigned char scancode;
  while (1) {
    if (inb(STATUS_REGISTER) & 0x01) {
      scancode = inb(DATA_PORT);
      uint8_t key_released = scancode & 0x80;
      switch (scancode & 0x7F) {
      case 0x2A:
        kdb_state.left_shift_pressed = !key_released;
        break;
      case 0x36:
        kdb_state.right_shift_pressed = !key_released;
        break;
      case 0x1D:
        kdb_state.ctr_pressed = !key_released;
        break;
      case 0x38:
        kdb_state.alt_pressed = !key_released;
        break;
      case 0x3A:
        if (!key_released) {
          kdb_state.capslock_pressed = !kdb_state.capslock_pressed;
          break;
        }
      }
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
  FileSystem fs;
  if (ata_init() != 0) {
    print_string("error init new FS");
    return;
  }
  print_string("new FS successfully init");
  fs_init();
  clean_screen();
  print_string("KeprOS is running!\n");

  while (1) {
    print_string("\nroot@keprOS> ");
    char *line = read_line(buffer, 1000000);
    shell_execute(line);
  }
}
