#include <client_registry.h>
#include <string.h>
#include <csapp.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <debug.h>
#include <sys/socket.h>
#include <sys/select.h>

/*
 * A client registry keeps track of the file descriptors for clients
 * that are currently connected.  Each time a client connects,
 * its file descriptor is added to the registry.  When the thread servicing
 * a client is about to terminate, it removes the file descriptor from
 * the registry.  The client registry also provides a function for shutting
 * down all client connections and a function that can be called by a thread
 * that wishes to wait for the client count to drop to zero.  Such a function
 * is useful, for example, in order to achieve clean termination:
 * when termination is desired, the "main" thread will shut down all client
 * connections and then wait for the set of registered file descriptors to
 * become empty before exiting the program.
 */
typedef struct client_registry {
	sem_t sem;                 // Semaphore to wait for transaction to commit or abort.
 	pthread_mutex_t mutex;     // Mutex to protect fields.
 	int client_set[FD_SETSIZE];
	int client_num;
}CLIENT_REGISTRY;



/*
 * Initialize a new client registry.
 *
 * @return  the newly initialized client registry, or NULL if initialization
 * fails.
 */
CLIENT_REGISTRY *creg_init() {
	CLIENT_REGISTRY *initialize = Malloc(sizeof(CLIENT_REGISTRY));
	initialize->client_num = 0;
	for (int i = 0; i < FD_SETSIZE; i++) {
		initialize->client_set[i] = 0;
	}

	Sem_init(&initialize->sem, 0, 1);
	int result = pthread_mutex_init(&initialize->mutex, NULL);

	if (!result) {
		debug("Initialize client registry");
		return initialize;
	}

	else {
		debug("Initialize client failed");
		return NULL;
	}
}

/*
 * Finalize a client registry, freeing all associated resources.
 * This method should not be called unless there are no currently
 * registered clients.
 *
 * @param cr  The client registry to be finalized, which must not
 * be referenced again.
 */
void creg_fini(CLIENT_REGISTRY *cr) {
	sem_destroy(&cr->sem);
	pthread_mutex_destroy(&cr->mutex);
	Free(cr);
}

/*
 * Register a client file descriptor.
 *
 * @param cr  The client registry.
 * @param fd  The file descriptor to be registered.
 * @return 0 if registration is successful, otherwise -1.
 */
int creg_register(CLIENT_REGISTRY *cr, int fd) {
	P(&cr->sem);
	pthread_mutex_lock(&cr->mutex);

	if (cr->client_num >= FD_SETSIZE) {
		debug("Number of client is greater than max size");
		pthread_mutex_unlock(&cr->mutex);
		V(&cr->sem);
		return -1;
	}

	cr->client_num++;

	for (int a = 0; a < cr->client_num; a++) {
		if (cr->client_set[a] == 0) {
			cr->client_set[a] = fd;
			// debug("client set[%d] is %d", a, cr->client_set[a]);
			break;
		}
	}

	debug("Register client fd %d (total connected: %d)", fd, cr->client_num);
	// debug("registration succeeded");
	pthread_mutex_unlock(&cr->mutex);
	V(&cr->sem);
	return 0;
}

/*
 * Unregister a client file descriptor, removing it from the registry.
 * If the number of registered clients is now zero, then any threads that
 * are blocked in creg_wait_for_empty() waiting for this situation to occur
 * are allowed to proceed.  It is an error if the CLIENT is not currently
 * registered when this function is called.
 *
 * @param cr  The client registry.
 * @param fd  The file descriptor to be unregistered.
 * @return 0  if unregistration succeeds, otherwise -1.
 */
int creg_unregister(CLIENT_REGISTRY *cr, int fd) {
	P(&cr->sem);
	pthread_mutex_lock(&cr->mutex);

	int found = 0;
	// debug("fd is %d", fd);
	for (int i = 0; i < FD_SETSIZE; i++) {
		// debug("client_set[%d] is %d", i, cr->client_set[i]);
		if (cr->client_set[i] == fd) {
			// eliminate
			cr->client_set[i] = 0;
			found = 1;
			cr->client_num--;

			// debug("client num is %d", cr->client_num);
		}
	}
	if (!found) {
		// debug("client num is %d", cr->client_num);
		pthread_mutex_unlock(&cr->mutex);
		V(&cr->sem);
		return -1;
	}

	// debug("client num is %d", cr->client_num);
	debug("Unregister client fd %d (total connected: %d)", fd, cr->client_num);
	// debug("unregistration succeeded");
	pthread_mutex_unlock(&cr->mutex);
	V(&cr->sem);
	return 0;
}

/*
 * A thread calling this function will block in the call until
 * the number of registered clients has reached zero, at which
 * point the function will return.  Note that this function may be
 * called concurrently by an arbitrary number of threads.
 *
 * @param cr  The client registry.
 */
void creg_wait_for_empty(CLIENT_REGISTRY *cr) {
	while(1) {
		P(&cr->sem);
		pthread_mutex_lock(&cr->mutex);
		if (cr->client_num == 0)
			break;
	}

	pthread_mutex_unlock(&cr->mutex);
	V(&cr->sem);
}

/*
 * Shut down (using shutdown(2)) all the sockets for connections
 * to currently registered clients.  The file descriptors are not
 * unregistered by this function.  It is intended that the file
 * descriptors will be unregistered by the threads servicing their
 * connections, once those server threads have recognized the EOF
 * on the connection that has resulted from the socket shutdown.
 *
 * @param cr  The client registry.
 */
void creg_shutdown_all(CLIENT_REGISTRY *cr) {
	P(&cr->sem);
	pthread_mutex_lock(&cr->mutex);

	for (int i = 0; i < FD_SETSIZE; i++) {
		if (cr->client_set[i] != 0) {
			if (cr->client_num != 0)
				cr->client_num = 0;
			shutdown(cr->client_set[i], SHUT_RD);
		}
	}

	pthread_mutex_unlock(&cr->mutex);
	V(&cr->sem);
}