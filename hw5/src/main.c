#include "debug.h"
#include "client_registry.h"
#include "transaction.h"
#include "store.h"
#include "server.h"
#include <string.h>
#include <csapp.h>
#include <stdlib.h>
#include <unistd.h>

static void terminate(int status);

CLIENT_REGISTRY *client_registry;
int listenfd, *connfdp;

void signal_handler(int sign, siginfo_t* info, void* dump) {
    if (sign == SIGHUP) {
        terminate(EXIT_SUCCESS);
    }
}

int main(int argc, char* argv[]){
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.

    int p_check = 0;
    char* port = NULL;

    for (int i = 1; i < argc; i += 2) {
        if (!strcmp(argv[i], "-p")) {
            if (argv[i+1] != NULL && atoi(argv[i+1]) != 0) {
                p_check = 1;
                port = strdup(argv[i+1]);
            }
        }
    }

    // Perform required initializations of the client_registry,
    // transaction manager, and object store.

    client_registry = creg_init();
    trans_init();
    store_init();

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function xacto_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.

    if(!p_check)
        terminate(EXIT_SUCCESS);

    struct sigaction new;

    new.sa_sigaction = signal_handler;
    new.sa_flags = SA_SIGINFO;
    sigemptyset(&new.sa_mask);
    sigaction(SIGHUP, &new, NULL);

    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    listenfd = Open_listenfd(port);

    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen);
        Pthread_create(&tid, NULL, xacto_client_service, connfdp);
     }

    // fprintf(stderr, "You have to finish implementing main() "
	//     "before the Xacto server will function.\n");

    terminate(EXIT_SUCCESS);
}

/*
 * Function called to cleanly shut down the server.
 */
void terminate(int status) {
    // Shutdown all client connections.
    // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);

    debug("Waiting for service threads to terminate...");
    creg_wait_for_empty(client_registry);
    debug("All service threads terminated.");

    // Finalize modules.
    creg_fini(client_registry);
    trans_fini();
    store_fini();

    shutdown(listenfd, SHUT_RD);
    Close(listenfd);

    debug("Xacto server terminating");
    exit(status);
}
