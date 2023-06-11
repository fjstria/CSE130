/* FJ Tria (@fjstria)
 * CSE130/asgn2/request.c
 */

#include "request.h"
#include "asgn2_helper_funcs.h"

#define REQEX  "^([a-zA-Z]{1,8}) /([a-zA-Z0-9.-]{1,63}) (HTTP/[0-9]\\.[0-9])\r\n"
#define HEADEX "([a-zA-Z0-9.-]{1,128}): ([ -~]{1,128})\r\n"

// ----- Helper Functions -----
// get()
int get(Request *R) {
    if (R->content_length != -1) {
        dprintf(R->infd, "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
        return (EXIT_FAILURE);
    }
    if (R->remaining_bytes > 0) {
        dprintf(R->infd, "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
        return (EXIT_FAILURE);
    }

    int fd;
    if ((fd = open(R->target_path, O_RDONLY | O_DIRECTORY)) != -1) {
        dprintf(R->infd, "HTTP/1.1 403 Forbidden\r\nContent-Length: %d\r\n\r\nForbidden\n", 10);
        return (EXIT_FAILURE);
    }

    if ((fd = open(R->target_path, O_RDONLY)) == -1) {
        if (errno == ENOENT) {
            dprintf(R->infd, "HTTP/1.1 404 Not Found\r\nContent-Length: %d\r\n\r\nNot Found\n", 10);
        } else if (errno == EACCES) {
            dprintf(R->infd, "HTTP/1.1 403 Forbidden\r\nContent-Length: %d\r\n\r\nForbidden\n", 10);
        } else {
            dprintf(R->infd,
                "HTTP/1.1 500 Internal Server Error\r\nContent-Length: %d\r\n\r\nInternal Server "
                "Error\n",
                22);
        }
        return (EXIT_FAILURE);
    }

    struct stat st;
    fstat(fd, &st);
    off_t size = st.st_size;

    dprintf(R->infd, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", size);
    int bytes_written = pass_bytes(fd, R->infd, size);
    if (bytes_written == -1) {
        dprintf(R->infd,
            "HTTP/1.1 500 Internal Server Error\r\nContent-Length: %d\r\n\r\nInternal Server "
            "Error\n",
            22);
        return (EXIT_FAILURE);
    }
    close(fd);
    return (EXIT_SUCCESS);
}

// put()
int put(Request *R) {
    if (R->content_length == -1) {
        dprintf(R->infd, "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
        return (EXIT_FAILURE);
    }
    int fd;
    int status_code = 0;
    // Checks for Directory
    if ((fd = open(R->target_path, O_WRONLY | O_DIRECTORY, 0666)) != -1) {
        dprintf(R->infd, "HTTP/1.1 403 Forbidden\r\nContent-Length: %d\r\n\r\nForbidden\n", 10);
        return (EXIT_FAILURE);
    }

    // Checks if File exists if not create it
    if ((fd = open(R->target_path, O_WRONLY | O_CREAT | O_EXCL, 0666)) == -1) {
        if (errno == EEXIST) {
            status_code = 200;
        } else if (errno == EACCES) {
            dprintf(R->infd, "HTTP/1.1 403 Forbidden\r\nContent-Length: %d\r\n\r\nForbidden\n", 10);
            return (EXIT_FAILURE);
        } else {
            dprintf(R->infd,
                "HTTP/1.1 500 Internal Server Error\r\nContent-Length: %d\r\n\r\nInternal Server "
                "Error\n",
                22);
            return (EXIT_FAILURE);
        }
    } else if (fd != -1) {
        status_code = 201;
    }

    // If file exists
    if (status_code == 200) {
        if ((fd = open(R->target_path, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1) {
            if (errno == EACCES) {
                dprintf(
                    R->infd, "HTTP/1.1 403 Forbidden\r\nContent-Length: %d\r\n\r\nForbidden\n", 10);
                return (EXIT_FAILURE);
            } else {
                dprintf(R->infd,
                    "HTTP/1.1 500 Internal Server Error\r\nContent-Length: %d\r\n\r\nInternal "
                    "Server Error\n",
                    22);
                return (EXIT_FAILURE);
            }
        }
    }

    int bytes_written = write_all(fd, R->message_body, R->remaining_bytes);
    if (bytes_written == -1) {
        dprintf(R->infd,
            "HTTP/1.1 500 Internal Server Error\r\nContent-Length: %d\r\n\r\nInternal Server "
            "Error\n",
            22);
        return (EXIT_FAILURE);
    }

    int total_written = R->content_length - R->remaining_bytes;
    bytes_written = pass_bytes(R->infd, fd, total_written);
    if (bytes_written == -1) {
        dprintf(R->infd,
            "HTTP/1.1 500 Internal Server Error\r\nContent-Length: %d\r\n\r\nInternal Server "
            "Error\n",
            22);
        return (EXIT_FAILURE);
    }

    if (status_code == 201) {
        dprintf(R->infd, "HTTP/1.1 201 Created\r\nContent-Length: %d\r\n\r\nCreated\n", 8);
    } else {
        dprintf(R->infd, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\nOK\n", 3);
    }
    close(fd);
    return (EXIT_SUCCESS);
}

// ----- Function Implementations -----
// handle_request()
int handle_request(Request *R) {
    if (strncmp(R->version, "HTTP/1.1", 8) != 0) {
        dprintf(R->infd,
            "HTTP/1.1 505 Version Not Supported\r\nContent-Length: %d\r\n\r\nVersion Not "
            "Supported\n",
            22);
        return (EXIT_FAILURE);
    } else if (strncmp(R->command, "GET", 3) == 0) {
        return (get(R));
    } else if (strncmp(R->command, "PUT", 3) == 0) {
        return (put(R));
    } else {
        dprintf(R->infd,
            "HTTP/1.1 501 Not Implemented\r\nContent-Length: %d\r\n\r\nNot Implemented\n", 16);
        return (EXIT_FAILURE);
    }
}

int parse_request(Request *R, char *buf, ssize_t bytes_read) {
    int total_offset = 0;
    regex_t re;
    regmatch_t matches[4];
    int rc;
    rc = regcomp(&re, REQEX, REG_EXTENDED);
    rc = regexec(&re, buf, 4, matches, 0);
    if (rc == 0) {
        R->command = buf;
        R->target_path = buf + matches[2].rm_so;
        R->version = buf + matches[3].rm_so;

        buf[matches[1].rm_eo] = '\0';
        R->target_path[matches[2].rm_eo - matches[2].rm_so] = '\0';
        R->version[matches[3].rm_eo - matches[3].rm_so] = '\0';

        buf += matches[3].rm_eo + 2;
        total_offset += matches[3].rm_eo + 2;
    } else {
        dprintf(R->infd, "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
        regfree(&re);
        return (EXIT_FAILURE);
    }

    R->content_length = -1;
    rc = regcomp(&re, HEADEX, REG_EXTENDED);
    rc = regexec(&re, buf, 3, matches, 0);
    while (rc == 0) {
        buf[matches[1].rm_eo] = '\0';
        buf[matches[2].rm_eo] = '\0';
        if (strncmp(buf, "Content-Length", 14) == 0) {
            int value = strtol(buf + matches[2].rm_so, NULL, 10);
            if (errno == EINVAL) {
                dprintf(R->infd,
                    "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
            }
            R->content_length = value;
        }
        buf += matches[2].rm_eo + 2;
        total_offset += matches[2].rm_eo + 2;
        rc = regexec(&re, buf, 3, matches, 0);
    }

    if ((rc != 0) && (buf[0] == '\r' && buf[1] == '\n')) {
        R->message_body = buf + 2;
        total_offset += 2;
        R->remaining_bytes = bytes_read - total_offset;
    } else if (rc != 0) {
        dprintf(R->infd, "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
        regfree(&re);
        return (EXIT_FAILURE);
    }
    regfree(&re);
    return (EXIT_SUCCESS);
}
