/* FJ Tria (@fjstria)
 * CSE130/asgn2/request.h
 */

#pragma once

#include "asgn2_helper_funcs.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <regex.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

// Request Struct
typedef struct Request {
    char *command;
    char *target_path;
    char *version;
    char *message_body;
    int infd;
    int content_length;
    int remaining_bytes;
} Request;

// Request Handling
int handle_request(Request *R);

// Request Parsing
int parse_request(Request *R, char *buf, ssize_t bytes_read);
