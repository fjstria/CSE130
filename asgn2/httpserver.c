/* FJ Tria (@fjstria)
 * CSE130/asgn2/httpserver.c
 */

#include "request.h"
#include "asgn2_helper_funcs.h"

#define BUFSIZE 4096

int main(int argc, char *argv[]) {
    if (argc != 2) {
        return (EXIT_FAILURE);
    }

    char buf[BUFSIZE + 1] = { '\0' };

    Listener_Socket socket;
    int port = strtol(argv[1], NULL, 10);
    if (errno == EINVAL) {
        fprintf(stderr, "Invalid Port\n");
        return (EXIT_FAILURE);
    }
    int socket_status = listener_init(&socket, port);
    if (socket_status == -1) {
        fprintf(stderr, "Invalid Port\n");
        return (EXIT_FAILURE);
    }

    while (true) {
        int sock_fd = listener_accept(&socket);
        if (sock_fd == -1) {
            fprintf(stderr, "Unable to Establish Connection\n");
            return (EXIT_FAILURE);
        }
        Request R;
        R.infd = sock_fd;
        ssize_t bytes_read = read_until(sock_fd, buf, BUFSIZE, "\r\n\r\n");
        if (bytes_read == -1) {
            dprintf(
                R.infd, "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\n\r\nBad Request\n", 12);
            return (EXIT_FAILURE);
        }
        if (parse_request(&R, buf, bytes_read) != EXIT_FAILURE) {
            handle_request(&R);
        }
        close(sock_fd);
        memset(buf, '\0', sizeof(buf));
    }
    return (EXIT_SUCCESS);
}
