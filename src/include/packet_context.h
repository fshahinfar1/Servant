#ifndef PACKET_CONTEXT_H
#define PACKET_CONTEXT_H

// Return values
#define PASS 100
#define DROP 200
#define SEND 300

// Context parameter type
struct pktctx {
	void *data; // in: start of the packet
	void *data_end; // in: end of the packet
	uint64_t pkt_len; // inout: final length of packet
};

#endif
