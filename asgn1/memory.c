/* FJ Tria (@fjstria)
 * CSE130/asgn1/memory.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int main(void) {
    char buf[8192]; // universal buffer
    int command_size = 4; // max sizes
    int location_size = PATH_MAX + 1024;
    int bytes_read = 1;
    int total_read = 0;

    // read command
    while ((total_read < command_size) && (bytes_read > 0)) {
        bytes_read = read(STDIN_FILENO, buf + total_read, command_size - total_read);
        total_read += bytes_read;
    }

    // replace space with null
    if (buf[3] == ' ') {
        buf[3] = '\0';
    } else {
        fprintf(stderr, "Invalid Command\n");
        return -1;
    }

    // command == get
    if (strcmp(buf, "get") == 0) {
        // read location
        bytes_read = 1;
        total_read = 0;
        while ((total_read < (location_size - 1)) && (bytes_read > 0)) {
            bytes_read = read(STDIN_FILENO, buf + total_read, location_size - total_read - 1);
            total_read += bytes_read;
        }

        // place null char at end
        buf[location_size] = '\0';

        // check for content after buf
        int i;
        for (i = 0; buf[i] != '\n'; i++) {
            if (buf[i] == ' ' || buf[i] == '\0') {
                fprintf(stderr, "Invalid Command\n");
                return -1;
            }
        }

        // place null char at newline
        buf[i] = '\0';

        // check for content after buf
        if ((total_read - (i + 1)) > 0) {
            fprintf(stderr, "Invalid Command\n");
            return -1;
        }

        // open file
        int fd;
        if ((fd = open(buf, O_RDONLY)) == -1) {
            fprintf(stderr, "Invalid Command\n");
            return -1;
        }

        // write contents to stdout
        int bytes_read;
        while ((bytes_read = read(fd, buf, sizeof(buf))) > 0) {
            int written_bytes, total_written = 0;
            while (total_written < bytes_read) {
                written_bytes
                    = write(STDOUT_FILENO, buf + total_written, bytes_read - total_written);
                if (written_bytes == -1) {
                    fprintf(stderr, "Operation Failed\n");
                    close(fd);
                    return -1;
                }
                total_written += written_bytes;
            }
        }

        // error if bad read
        if (bytes_read == -1) {
            fprintf(stderr, "Invalid Command\n");
            close(fd);
            return -1;
        }

        // close file
        close(fd);
        return 0;
    }

    // command == set
    else if (strcmp(buf, "set") == 0) {
        // read location
        bytes_read = 1;
        total_read = 0;
        while ((total_read < (location_size - 1)) && (bytes_read > 0)) {
            bytes_read = read(STDIN_FILENO, buf + total_read, location_size - total_read - 1);
            total_read += bytes_read;
        }

        // place null char at end
        buf[location_size] = '\0';

        // check for content after buf
        int i;
        for (i = 0; buf[i] != '\n'; i++) {
            // return error
            if (buf[i] == ' ' || buf[i] == '\0') {
                fprintf(stderr, "Invalid Command\n");
                return -1;
            }
        }

        // place null char at newline
        buf[i] = '\0';

        // open file
        int fd;
        if ((fd = open(buf, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1) {
            fprintf(stderr, "Invalid Command\n");
            return -1;
        }

        // write contents to file
        int written_bytes = 0;
        int total_written = 0;
        total_read = total_read - i - 1;
        char *contents = buf + i + 1;
        while (total_written < total_read) {
            written_bytes = write(fd, contents + total_written, total_read - total_written);

            // check for invalid write
            if (written_bytes == -1) {
                fprintf(stderr, "Operation Failed\n");
                close(fd);
                return -1;
            }
            total_written += written_bytes;
        }

        // read other input
        while ((bytes_read = read(STDIN_FILENO, buf, 8192)) > 0) {
            int written_bytes, total_written = 0;
            while (total_written < bytes_read) {
                written_bytes = write(fd, buf + total_written, bytes_read - total_written);
                // check for invalid write
                if (written_bytes == -1) {
                    fprintf(stderr, "Operation Failed\n");
                    close(fd);
                    return -1;
                }
                total_written += written_bytes;
            }
        }

        // error if bad read
        if (bytes_read == -1) {
            fprintf(stderr, "Invalid Command\n");
            close(fd);
            return -1;
        }

        // write OK if good
        write(STDOUT_FILENO, "OK\n", sizeof(char) * 3);

        // close file
        close(fd);
        return 0;
    }

    // invalid command
    else {
        fprintf(stderr, "Invalid Command\n");
        return -1;
    }
}
