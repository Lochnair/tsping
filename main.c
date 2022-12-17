#include <argp.h>
#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <asm/socket.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <linux/net_tstamp.h>
#include <linux/time_types.h>

#include "tsping.h"

unsigned long sentICMP = 0;
unsigned long receivedICMP = 0;

unsigned long get_time_since_midnight_ms()
{
	struct __kernel_timespec time;
	clock_gettime(CLOCK_REALTIME, (struct timespec *) &time);

	return (time.tv_sec % 86400 * 1000) + (time.tv_nsec / 1000000);
}

unsigned short calculate_checksum(void *b, int len)
{
	unsigned short *buf = b;
	unsigned int sum = 0;
	unsigned short result;

	for (sum = 0; len > 1; len -= 2)
		sum += *buf++;
	if (len == 1)
		sum += *(unsigned char *)buf;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return result;
}

int send_icmp_echo_request(int sock_fd, struct sockaddr_in *reflector, int id, int seq)
{
	struct icmp_echo_hdr hdr = {0};

	hdr.type = ICMP_ECHO;
	hdr.identifier = id;
	hdr.sequence = seq;
	hdr.payload = htonl(get_time_since_midnight_ms());

	hdr.checksum = calculate_checksum(&hdr, sizeof(hdr));

	int t;

	if ((t = sendto(sock_fd, &hdr, sizeof(hdr), 0, (const struct sockaddr *)reflector, sizeof(*reflector))) == -1)
	{
		printf("something wrong: %d\n", t);
		return 1;
	}

	sentICMP++;

	return 0;
}

int send_icmp_timestamp_request(int sock_fd, struct sockaddr_in *reflector, int id, int seq)
{
	struct icmp_timestamp_hdr hdr;

	memset(&hdr, 0, sizeof(hdr));

	hdr.type = ICMP_TIMESTAMP;
	hdr.identifier = id;
	hdr.sequence = seq;
	hdr.originateTime = htonl(get_time_since_midnight_ms());

	hdr.checksum = calculate_checksum(&hdr, sizeof(hdr));

	int t;

	if ((t = sendto(sock_fd, &hdr, sizeof(hdr), 0, (const struct sockaddr *)reflector, sizeof(*reflector))) == -1)
	{
		printf("something wrong: %d\n", t);
		return 1;
	}

	sentICMP++;

	return 0;
}

int parse_icmp_echo_reply(uint16_t id, char * buff, int len, int recv, icmp_result * result) {
	if (len + sizeof(struct icmp_echo_hdr) > recv)
	{
		return -1;
	}

	struct icmp_echo_hdr * hdr = (struct icmp_echo_hdr *) buff;

	if (id != hdr->identifier)
		return -1;

	result->sequence = ntohs(hdr->sequence);
	result->originateTime = ntohl(hdr->payload);
	result->finishedTime = get_time_since_midnight_ms();

	return 0;
}

int parse_icmp_timestamp_reply(uint16_t id, char * buff, int len, int recv, icmp_result * result) {
	if (len + sizeof(struct icmp_timestamp_hdr) > recv)
	{
		return -1;
	}

	struct icmp_timestamp_hdr * hdr = (struct icmp_timestamp_hdr *) buff;

	if (id != hdr->identifier)
		return -1;

	result->sequence = ntohs(hdr->sequence);
	result->originateTime = ntohl(hdr->originateTime);
	result->transmitTime = ntohl(hdr->transmitTime);
	result->receiveTime = ntohl(hdr->receiveTime);
	result->finishedTime = get_time_since_midnight_ms();

	return 0;
}

void * receiver_loop(void *data)
{
	struct thread_data * thread_data = (struct thread_data *) data;
	struct arguments * args = thread_data->args;
	int sock_fd = thread_data->sock_fd;
	uint16_t id = htons(getpid() & 0xFFFF);

	while (1)
	{
		char * buff = malloc(100);
		struct sockaddr_in remote_addr;
		socklen_t addr_len = sizeof(remote_addr);
		int recv = recvfrom(sock_fd, buff, 100, 0, (struct sockaddr *) &remote_addr, &addr_len);

		if (recv < 0)
			continue;

		// Find the length of the IP header
		// so we can figure out where in
		// the buffer the ICMP header starts
		int len = (*buff & 0x0F) * 4;
		char * hdr = (char *) (buff + len);

		icmp_result result = {0};

		char ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(remote_addr.sin_addr), ip, INET_ADDRSTRLEN);

		int res;
		uint32_t rtt;
		switch (*hdr) {
			case 0:
				res = parse_icmp_echo_reply(id, hdr, len, recv, &result);

				if (res != 0)
					continue;

				rtt = result.finishedTime - result.originateTime;

				if (args->machine_readable) {
					printf("%s,%u,%u\n", ip, result.sequence, rtt);
				}
				else {
					printf("%15s : [%u], %u ms\n", ip, result.sequence, rtt);
				}

				receivedICMP++;

				break;
			case 14:
				res = parse_icmp_timestamp_reply(id, hdr, len, recv, &result);

				if (res != 0)
					continue;

				uint32_t down_time = result.finishedTime - result.transmitTime;
				uint32_t up_time = result.receiveTime - result.originateTime;
				rtt = result.finishedTime - result.originateTime;

				if (args->machine_readable) {
					printf("%s,%u,%u,%u,%u,%u,%u,%u,%u\n", ip, result.sequence, result.originateTime, result.receiveTime, result.transmitTime, result.finishedTime, rtt, down_time, up_time);
				}
				else {
					printf("%15s : [%u] Down: %u, Up: %u, RTT: %u, Originate: %u, Received: %u, Transmit: %u, Finished: %u\n", ip, result.sequence, down_time, up_time, rtt, result.originateTime, result.receiveTime, result.transmitTime, result.finishedTime);
				}

				receivedICMP++;

				break;
			default:
				continue;
		}

		

		free(buff);		
	}

	pthread_exit(NULL);
}

static void milliseconds_to_timespec(struct timespec *timespec, unsigned long ms)
{
    timespec->tv_sec = ms / 1000;
    timespec->tv_nsec = (ms % 1000) * 1000000;
}

void *sender_loop(void *data)
{
	struct thread_data * thread_data = (struct thread_data *)data;
	struct arguments * args = thread_data->args;
	int sock_fd = thread_data->sock_fd;
	struct sockaddr_in * targets = args->targets;
	int targets_len = args->targets_len;
	

	struct timespec sleep_time = {0}, spacing_time = {0};

	if (args->sleep_time > 0)
		milliseconds_to_timespec(&sleep_time, args->sleep_time);

	if (args->target_spacing > 0 && args->targets_len > 1)
		milliseconds_to_timespec(&spacing_time, args->target_spacing);

	uint16_t seq = 0;

	while (1)
	{
		for (int i = 0; i < targets_len; i++)
		{
			switch (args->icmp_type) {
				case 8:
					send_icmp_echo_request(sock_fd, &targets[i], thread_data->id, htons(seq));
					break;
				case 13:
					send_icmp_timestamp_request(sock_fd, &targets[i], thread_data->id, htons(seq));
					break;
				default:
					printf("sender: Invalid ICMP type: %u, exiting..\n", args->icmp_type);
					raise(SIGTERM);
			}

			if (args->target_spacing > 0 && args->targets_len > 1)
				nanosleep(&spacing_time, NULL);
		}

		seq++;
		if (args->sleep_time > 0)
			nanosleep(&sleep_time, NULL);
	}

	pthread_exit(NULL);
}

int main (int argc, char **argv)
{
	struct arguments arguments;

	/* Default values. */
	arguments.icmp_type = 13; // ICMP timestamps
	arguments.machine_readable = 0;
	arguments.sleep_time = 100;
	arguments.target_spacing = 0;
	arguments.targets = malloc(sizeof(struct sockaddr_in));
	arguments.targets_len = 0;

	/* Parse our arguments; every option seen by parse_opt will
	   be reflected in arguments. */
	int ret = argp_parse (&argp, argc, argv, 0, 0, &arguments);

	if (ret != 0)
		return ret;

	pthread_t receiver_thread;
	pthread_t sender_thread;

	int sock_fd;

	if ((sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1)
	{
		printf("Couldn't open raw socket (are you root?)\n");
		return 1;
	}

	struct thread_data data;
	data.args = &arguments;
	data.id = htons(getpid() & 0xFFFF);
	data.sock_fd = sock_fd;

	int t;
	if ((t = pthread_create(&receiver_thread, NULL, receiver_loop, (void *)&data)) != 0)
	{
		printf("Failed to create receiver thread: %d\n", t);
		return 1;
	}

	if ((t = pthread_create(&sender_thread, NULL, sender_loop, (void *)&data)) != 0)
	{
		printf("Failed to create sender thread: %d\n", t);
		return 1;
	}

	// Wait for threads to shut down
	pthread_join(receiver_thread, NULL);
	pthread_join(sender_thread, NULL);

	return 0;
}
