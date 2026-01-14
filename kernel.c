char *vidmem = (char *)0xb8000;

void print_string(char *str) {
  unsigned int i = 0, j = 0;

  while (str[j] != '\0') {
    vidmem[i] = str[j];
    vidmem[i + 1] = 0x07;
    j++;
    i += 2;
  }
}

void clean_screen() {
  unsigned int i = 0, j = 0;

  while (j < 80 * 25 * 2) {
    vidmem[j] = ' ';
    vidmem[j + 1] = 0x07;
    j += 2;
  }
}

void os_main(void) {
  clean_screen();
  print_string("Hello from KeprOS!");
}
