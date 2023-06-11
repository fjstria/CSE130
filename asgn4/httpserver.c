/* FJ Tria (@fjstria)
 * CSE130/asgn4/httpserver.c
 */

#include "asgn4_helper_funcs.h"
#include "connection.h"
#include "debug.h"
#include "queue.h"
#include "request.h"
#include "response.h"

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/file.h>

#define BUFSIZE 4096
#define OPTIONS "t:"

queue_t *q;
pthread_mutex_t mutex;

void handle_connection(int);
void handle_get(conn_t *);
void handle_put(conn_t *);
void handle_unsupported(conn_t *);
void *workerThread();

int main(int argc, char **argv) {
    // COMMAND LINE INPUT
    int opt = 0;
    int nt = 4;

    while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
        switch (opt) {
        case 't':
            nt = atoi(optarg);
            if (nt < 0) {
                fprintf(stderr, "Invalid thread size.\n");
                exit(EXIT_FAILURE);
            }
            break;
        default: break;
        }
    }

    char *end = NULL;
    size_t port = (size_t) strtoull(argv[optind], &end, 10);
    if (end && *end != '\0') {
        warnx("invalid port number: %s", argv[optind]);
        return EXIT_FAILURE;
    }

    if (argc < 2) {
        warnx("wrong arguments: %s port_num", argv[optind]);
        fprintf(stderr, "usage: %s <port>\n", argv[optind]);
        return EXIT_FAILURE;
    }

    for (int i = optind + 1; i < argc; i++) {
        fprintf(stderr, "More extraneous arguments\n");
        return EXIT_FAILURE;
    }

    // INITIALIZE SOCKET
    signal(SIGPIPE, SIG_IGN);
    Listener_Socket socket;
    listener_init(&socket, port);

    // INITIALIZE QUEUE
    q = queue_new(nt);

    // INITIALIZE MUTEX LOCK
    pthread_mutex_init(&mutex, NULL);

    // INITIALIZE THREADS
    pthread_t threads[nt];
    for (int i = 0; i < nt; i++) {
        pthread_create(&(threads[i]), NULL, workerThread, NULL);
    }

    // DISPATCHER MAIN THREAD
    while (1) {
        uintptr_t connfd = listener_accept(&socket);
        queue_push(q, (void *) connfd);
    }
    return EXIT_SUCCESS;
}

void *workerThread() {
    while (1) {
        uintptr_t connfd = -1;
        queue_pop(q, (void **) &connfd);
        handle_connection(connfd);
        close(connfd);
    }
}

void handle_connection(int connfd) {
    conn_t *conn = conn_new(connfd);

    const Response_t *res = conn_parse(conn);

    if (res != NULL) {
        conn_send_response(conn, res);
    } else {
        debug("%s", conn_str(conn));
        const Request_t *req = conn_get_request(conn);
        if (req == &REQUEST_GET) {
            handle_get(conn);
        } else if (req == &REQUEST_PUT) {
            handle_put(conn);
        } else {
            handle_unsupported(conn);
        }
    }
    conn_delete(&conn);
}

void handle_get(conn_t *conn) {
    char *uri = conn_get_uri(conn);
    const Response_t *res = NULL;

    // OPEN FILE
    int fd = open(uri, O_RDONLY);
    if (fd < 0) {
        debug("%s: %d", uri, errno);
        if (errno == EACCES) {
            res = &RESPONSE_FORBIDDEN;
            conn_send_response(conn, res);
            char *header = conn_get_header(conn, "Request-Id");
            if (header == NULL) {
                header = "0";
            }
            fprintf(stderr, "GET,/%s,403,%s\n", uri, header);
            return;
        } else if (errno == ENOENT) {
            res = &RESPONSE_NOT_FOUND;
            conn_send_response(conn, res);
            char *header = conn_get_header(conn, "Request-Id");
            if (header == NULL) {
                header = "0";
            }
            fprintf(stderr, "GET,/%s,404,%s\n", uri, header);
            return;
        } else {
            res = &RESPONSE_INTERNAL_SERVER_ERROR;
            conn_send_response(conn, res);
            char *header = conn_get_header(conn, "Request-Id");
            if (header == NULL) {
                header = "0";
            }
            fprintf(stderr, "GET,/%s,500,%s\n", uri, header);
            return;
        }
    }
    flock(fd, LOCK_SH);
    // GET SIZE
    struct stat finfo;
    fstat(fd, &finfo);
    off_t size = finfo.st_size;

    // CHECK IF DIRECTORY
    if (S_ISDIR(finfo.st_mode)) {
        res = &RESPONSE_FORBIDDEN;
        conn_send_response(conn, res);
        char *header = conn_get_header(conn, "Request-Id");
        if (header == NULL) {
            header = "0";
        }
        fprintf(stderr, "GET,/%s,403,%s\n", uri, header);
        close(fd);
        return;
    }

    // SEND FILE
    res = conn_send_file(conn, fd, size);
    char *header = conn_get_header(conn, "Request-Id");
    if (header == NULL) {
        header = "0";
    }
    fprintf(stderr, "GET,/%s,200,%s\n", uri, header);
    close(fd);
    return;
}

void handle_unsupported(conn_t *conn) {
    debug("handling unsupported request");

    // SEND RESPONSE
    conn_send_response(conn, &RESPONSE_NOT_IMPLEMENTED);
    char *uri = conn_get_uri(conn);
    char *header = conn_get_header(conn, "Request-Id");
    if (header == NULL) {
        header = "0";
    }
    fprintf(stderr, "UNSUPPORTED,/%s,501,%s\n", uri, header);
    return;
}

void handle_put(conn_t *conn) {
    char *uri = conn_get_uri(conn);
    const Response_t *res = NULL;
    debug("handling put request for %s", uri);

    // START CRIT REGION
    pthread_mutex_lock(&mutex);

    // CHECK IF EXISTS
    bool existed = access(uri, F_OK) == 0;
    debug("%s existed? %d", uri, existed);

    // CREATE/OPEN FILE
    int fd = open(uri, O_CREAT | O_WRONLY, 0600);
    if (fd < 0) {
        debug("%s: %d", uri, errno);
        if (errno == EACCES || errno == EISDIR || errno == ENOENT) {
            res = &RESPONSE_FORBIDDEN;
            conn_send_response(conn, res);
            char *header = conn_get_header(conn, "Request-Id");
            if (header == NULL) {
                header = "0";
            }
            fprintf(stderr, "PUT,/%s,403,%s\n", uri, header);
            return;
        } else {
            res = &RESPONSE_INTERNAL_SERVER_ERROR;
            conn_send_response(conn, res);
            char *header = conn_get_header(conn, "Request-Id");
            if (header == NULL) {
                header = "0";
            }
            fprintf(stderr, "PUT,/%s,500,%s\n", uri, header);
            return;
        }
    }

    flock(fd, LOCK_EX);
    // END CRIT REGION

    pthread_mutex_unlock(&mutex);

    ftruncate(fd, 0);
    res = conn_recv_file(conn, fd);

    if (res == NULL && existed) {
        res = &RESPONSE_OK;
        conn_send_response(conn, res);
        char *header = conn_get_header(conn, "Request-Id");
        if (header == NULL) {
            header = "0";
        }
        fprintf(stderr, "PUT,/%s,200,%s\n", uri, header);
    } else if (res == NULL && !existed) {
        res = &RESPONSE_CREATED;
        conn_send_response(conn, res);
        char *header = conn_get_header(conn, "Request-Id");
        if (header == NULL) {
            header = "0";
        }
        fprintf(stderr, "PUT,/%s,201,%s\n", uri, header);
    } else {
        char *header = conn_get_header(conn, "Request-Id");
        if (header == NULL) {
            header = "0";
        }
        fprintf(stderr, "PUT,/%s,500,%s\n", uri, header);
    }
    close(fd);
    return;
}
