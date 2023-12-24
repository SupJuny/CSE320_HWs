#include <stdlib.h>
#include <pthread.h>
#include <csapp.h>
#include <string.h>
#include <unistd.h>
#include <debug.h>

#include "transaction.h"
#include "client_registry.h"
#include "protocol.h"
#include "data.h"
#include "store.h"

/*
 * Client registry that should be used to track the set of
 * file descriptors of currently connected clients.
 */
CLIENT_REGISTRY *client_registry;

/*
 * Thread function for the thread that handles client requests.
 *
 * @param  Pointer to a variable that holds the file descriptor for
 * the client connection.  This pointer must be freed once the file
 * descriptor has been retrieved.
 */
void *xacto_client_service(void *arg) {
	int connfd = *((int*)arg);
	Pthread_detach(Pthread_self());
	Free(arg);

	creg_register(client_registry, connfd);
	TRANSACTION *trans = trans_create();

	while (1) {
		// debug("connfd is %d", connfd);
		XACTO_PACKET *packet = Malloc(sizeof(XACTO_PACKET));
		void** data = Malloc(sizeof(void**));
		int proto = proto_recv_packet(connfd, packet, NULL);

		// protocol recieve failed
		if (proto != 0) {
			Free(packet);
			Free(data);
			// creg_unregister(client_registry, connfd);
			break;
		}

		// protocol recieve success
		else {
			char* key_content = NULL;
			char* value_content = NULL;
			int key_size = 0;
			int value_size = 0;
			// PUT
			if (packet->type == XACTO_PUT_PKT) {
				debug("PUT packet received");
				proto_recv_packet(connfd, packet, data);

				// KEY
				if (packet->type == XACTO_KEY_PKT) {
					key_size = ntohl(packet->size);
					key_content = strndup(*data, key_size);
					debug("Received key, size %d, key %s", key_size, key_content);
					Free(*data);
					proto_recv_packet(connfd, packet, data);
				}

				// VALUE
				if (packet->type == XACTO_VALUE_PKT) {
					value_size = ntohl(packet->size);
					value_content = strndup(*data, value_size);
					Free(*data);
					debug("Received value, size %d, value %s", value_size, value_content);
				}

				BLOB* key_blob = blob_create(key_content, key_size);
				KEY* key_pt = key_create(key_blob);
				BLOB* value_blob = blob_create(value_content, value_size);

				// if((
				packet->status = store_put(trans, key_pt, value_blob);
				// ) == TRANS_PENDING) {
				packet->type = XACTO_REPLY_PKT;
				packet->size = 0;
				proto_send_packet(connfd, packet, NULL);
				Free(data);
				Free(packet);
				Free(key_content);
				Free(value_content);
				// }
				// else {
				// 	trans_abort(trans);
				// }
			}

			// GET
			else if (packet->type == XACTO_GET_PKT) {
				debug("GET packet received");
				proto_recv_packet(connfd, packet, data);

				// KEY
				if (packet->type == XACTO_KEY_PKT) {
					key_size = ntohl(packet->size);
					key_content = strndup(*data, key_size);
					Free(*data);
					debug("Received key, size %d, key %s", key_size, key_content);
				}

				BLOB* key_blob = blob_create(key_content, key_size);
				KEY* key_pt = key_create(key_blob);

				BLOB** value_bl = Malloc(sizeof(BLOB**));

				// if((
				packet->status = store_get(trans, key_pt, value_bl);
				// ) == TRANS_PENDING) {
				if (*value_bl != NULL) {
					debug("[%d] Value is %p [%s]", connfd, value_bl, (*value_bl)->content);
					packet->type = XACTO_REPLY_PKT;
					packet->size = 0;
					proto_send_packet(connfd, packet, NULL);

					packet->type = XACTO_VALUE_PKT;
					packet->size = ntohl((*value_bl)->size);
					proto_send_packet(connfd, packet, (*value_bl)->content);

					blob_unref(*value_bl, "obtained from store_get");

					Free(data);
					Free(packet);
					Free(key_content);
					Free(value_content);
					// Free(*value_bl);
				}
				else {
					debug("value_bl NULL");
					packet->type = XACTO_REPLY_PKT;
					packet->size = 0;
					proto_send_packet(connfd, packet, NULL);

					packet->type = XACTO_VALUE_PKT;
					packet->size = 0;
					packet->null = 1;
					proto_send_packet(connfd, packet, NULL);

					Free(data);
					Free(packet);
					Free(key_content);
					Free(value_content);
					Free(*value_bl);
				}
				// }
			}

			// COMMIT
			else if (packet->type == XACTO_COMMIT_PKT) {
				debug("COMMIT packet received");

				// if((
				packet->status = trans_commit(trans);
				// ) == TRANS_COMMITTED) {
				packet->type = XACTO_REPLY_PKT;
				packet->size = 0;
				proto_send_packet(connfd, packet, NULL);

				Free(data);
				Free(packet);
				Free(key_content);
				Free(value_content);
			}
		}
	}
	creg_unregister(client_registry, connfd);
	Close(connfd);
	return NULL;
}
