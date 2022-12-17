#ifndef TSPING_TSPING_H
#define TSPING_TSPING_H

#include <stdint.h>
#include "args.h"

struct icmp_echo_hdr
{
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint16_t identifier;
	uint16_t sequence;
	uint32_t payload;
};

struct icmp_timestamp_hdr
{
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint16_t identifier;
	uint16_t sequence;
	uint32_t originateTime;
	uint32_t receiveTime;
	uint32_t transmitTime;
};

typedef struct
{
	uint16_t sequence;
	uint32_t originateTime;
	uint32_t receiveTime;
	uint32_t transmitTime;
	uint32_t finishedTime;
} icmp_result;

struct thread_data
{
	struct arguments * args;
	uint16_t id; // would have used icmp_id here, but it's a macro
	int sock_fd;
};

#endif //TSPING_TSPING_H
