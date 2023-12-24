#include <protocol.h>
#include <string.h>
#include <csapp.h>
#include <stdlib.h>
#include <unistd.h>
#include <debug.h>

/*
 * Send a packet header, followed by an associated data payload, if any.
 * Multi-byte fields in the packet header are stored in network byte oder.
 *
 * @param fd  The file descriptor on which packet is to be sent.
 * @param pkt  The fixed-size part of the packet, with multi-byte fields
 *   in network byte order
 * @param data  The payload for data packet, or NULL.  A NULL value used
 *   here for a data packet specifies the transmission of a special null
 *   data value, which has no content.
 * @return  0 in case of successful transmission, -1 otherwise.  In the
 *   latter case, errno is set to indicate the error.
 */

ssize_t pck_hd_size = sizeof(XACTO_PACKET);

int proto_send_packet(int fd, XACTO_PACKET *pkt, void *data) {

	debug("get into protocol send func");

	int size;
	// convert multi-byte data from host to network type
	// pkt->serial = htonl(pkt->serial);
	size = htonl(pkt->size);
	// pkt->timestamp_sec = htonl(pkt->timestamp_sec);
	// pkt->timestamp_nsec = htonl(pkt->timestamp_nsec);

	if (rio_writen(fd, pkt, pck_hd_size) == -1) {
		return -1;
	}

	if (data != NULL && size > 0) {
		if(rio_writen(fd, data, size) == -1)
			return -1;
	}
	return 0;
}

/*
 * Receive a packet, blocking until one is available.
 * The returned structure has its multi-byte fields in network byte order.
 *
 * @param fd  The file descriptor from which the packet is to be received.
 * @param pkt  Pointer to caller-supplied storage for the fixed-size
 *   portion of the packet.
 * @param datap  Pointer to variable into which to store a pointer to any
 *   payload received.
 * @return  0 in case of successful reception, -1 otherwise.  In the
 *   latter case, errno is set to indicate the error.
 *
 * If the returned payload pointer is non-NULL, then the caller assumes
 * responsibility for freeing the storage.
 */
int proto_recv_packet(int fd, XACTO_PACKET *pkt, void **datap) {

	debug("get into protocol recv func");
	int recv_result = 0;
	if ((recv_result = rio_readn(fd, pkt, pck_hd_size)) <= 0) {
		// debug("recv func failed because of error with %d", recv_result);
		return -1;
	}

	// convert multi-byte data from network to host type
	// pkt->serial = ntohl(pkt->serial);
	// pkt->timestamp_sec = ntohl(pkt->timestamp_sec);
	// pkt->timestamp_nsec = ntohl(pkt->timestamp_nsec);

	// debug("1st read done");


	int size;

	size = ntohl(pkt->size);
	if (size > 0) {
		// debug("recv func before malloc");
		// debug("payload size is %d", size);
		*datap = Malloc(size+1);

		// debug("recv func malloc succeed");
		if (rio_readn(fd, *datap, size) < 0) {
			// debug("one more read failed");
			Free(*datap);
			return -1;
		}
		// debug("one more read succeed");
		// free(*datap);
		return 0;
	}

	else if (size == 0) {
		// debug("packet length (size) is zero");
		return 0;
	}

	return -1;
}
