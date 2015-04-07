#ifndef SHELL_H
#define SHELL_H

int parse_command(char *str, char *argv[]);

typedef void cmdfunc(int, char *[]);

cmdfunc *do_command(const char *str);

void *get_queue_handle(void); //call from main.c

#endif
