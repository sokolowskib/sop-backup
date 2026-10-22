#define _GNU_SOURCE
#include "parse_command.h"
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "create_copy.h"
#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

char* wait_for_input(size_t length)
{  // zmien na char pointer zeby zwracac, pamietaj o free bo funckja mallocuje
    char* buffer = NULL;

    if (getline(&buffer, &length, stdin) < 0)
    {
        free(buffer);
        return NULL;
    }
    return buffer;
}

int create_command_array(char* command, char* arr[])
{
    int len = strlen(command);
    if (len <= 0)
    {
        return -1;
    }
    if (command[len - 1] == '\n')
    {
        command[len - 1] = '\0';
        len--;
    }
    bool in_apostrophe = false;
    int start = 0, count = 0;

    for (int i = 0; i <= len; i++)
    {
        if (command[i] == '"')
            in_apostrophe = !in_apostrophe;

        if ((command[i] == ' ' && !in_apostrophe) || command[i] == '\0')
        {
            if (command[start] != ' ' && start != i)
            {
                command[i] = '\0';

                if (command[start] == '"')
                {
                    arr[count] = &command[start + 1];
                    if (command[i - 1] == '"')
                        command[i - 1] = '\0';
                }
                else
                {
                    arr[count] = &command[start];
                }
                count++;
            }
            start = i + 1;
        }
    }
    return count;
}

void make_path_absolute(char* input, char* output)
{
    if (realpath(input, output) != NULL)
    {
        return;
    }

    if (input[0] == '/')
    {
        strncpy(output, input, MAX_PATH);
    }
    else
    {
        char cwd[MAX_PATH];
        if (getcwd(cwd, MAX_PATH) != NULL)
        {
            snprintf(output, MAX_PATH + 1, "%s/%s", cwd, input);
        }
        else
        {
            perror("nie moge znalezc sciezki absolutnej\n");
        }
    }
}

