#ifndef SOP_PROJEKT1_WAIT_INPUT_H
#define SOP_PROJEKT1_WAIT_INPUT_H
#define MAX_CLI_INPUT 256
#define MAX_COMMAND_ARRAY 128
#include <stddef.h>
char* wait_for_input(size_t length);
int create_command_array(char* command, char* arr[]);
void make_path_absolute(char*, char*);
#endif  // SOP_PROJEKT1_WAIT_INPUT_H

