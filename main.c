#include <argp.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "tsping.h"

#ifndef _TSPING_VERSION
#define _TSPING_VERSION "0.0.0"
#endif

unsigned long sentICMP = 0;
unsigned long receivedICMP = 0;

int char_replace(char *str, char find, char rep) {
    char *current_pos = strchr(str,find);
	int n = 0;

    while (current_pos) {
        *current_pos = rep;
		n++;
        current_pos = strchr(current_pos,find);
    }

    return n;
}

unsigned long get_time_since_midnight_ms()
{
	struct timespec time;
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
		char ip[INET_ADDRSTRLEN];
		
		int err = errno;

		if (inet_ntop(AF_INET, &(reflector->sin_addr), ip, INET_ADDRSTRLEN) == NULL)
			fprintf(stderr, "Something went wrong while sending: %s\n", strerror(err));
		else
			fprintf(stderr, "Something went wrong while sending to %s: %s\n", ip, strerror(err));
		return 1;
	}

	sentICMP++;

	return 0;
}

int send_icmp_timestamp_request(int sock_fd, struct sockaddr_in *reflector, int id, int seq)
{
	struct icmp_timestamp_hdr hdr = {0};

	hdr.type = ICMP_TIMESTAMP;
	hdr.identifier = id;
	hdr.sequence = seq;
	hdr.originateTime = htonl(get_time_since_midnight_ms());

	hdr.checksum = calculate_checksum(&hdr, sizeof(hdr));

	int t;

	if ((t = sendto(sock_fd, &hdr, sizeof(hdr), 0, (const struct sockaddr *)reflector, sizeof(*reflector))) == -1)
	{
		char ip[INET_ADDRSTRLEN];
		
		int err = errno;

		if (inet_ntop(AF_INET, &(reflector->sin_addr), ip, INET_ADDRSTRLEN) == NULL)
			fprintf(stderr, "Something went wrong while sending: %s\n", strerror(err));
		else
			fprintf(stderr, "Something went wrong while sending to %s: %s\n", ip, strerror(err));
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

static void print_timestamp(char * fmt) {
	struct timeval recv_time;
	gettimeofday(&recv_time, NULL);
	printf(fmt, (unsigned long) recv_time.tv_sec, (unsigned long) recv_time.tv_usec);
}

void * receiver_loop(void *data)
{
	struct thread_data * thread_data = (struct thread_data *) data;
	struct arguments * args = thread_data->args;
	int sock_fd = thread_data->sock_fd;
	uint16_t id = htons(getpid() & 0xFFFF);

	char FMT_ICMP_ECHO_HUMAN[] = "%-15s : [%u], %d ms\n";
	char FMT_ICMP_ECHO_MACHINE[] = "%s,%u,%d\n";
	char FMT_ICMP_TIMESTAMP_HUMAN[] = "%-15s : [%u] Originate: %u, Received: %u, Transmit: %u, Finished: %u, RTT: %d, Down: %d, Up: %d,\n";
	char FMT_ICMP_TIMESTAMP_MACHINE[] = "%s,%u,%u,%u,%u,%u,%d,%d,%d\n";
	char FMT_TIMESTAMPS_HUMAN[] = "[%lu.%06lu] ";
	char FMT_TIMESTAMPS_MACHINE[] = "%lu.%06lu,";

	char * FMT_OUTPUT;
	char * FMT_TIMESTAMP;

	if (args->icmp_type == 8 && args->machine_readable == 0)
		FMT_OUTPUT = FMT_ICMP_ECHO_HUMAN;
	else if (args->icmp_type == 8 && args->machine_readable == 1)
		FMT_OUTPUT = FMT_ICMP_ECHO_MACHINE;
	else if (args->icmp_type == 13 && args->machine_readable == 0)
		FMT_OUTPUT = FMT_ICMP_TIMESTAMP_HUMAN;
	else if (args->icmp_type == 13 && args->machine_readable == 1)
		FMT_OUTPUT = FMT_ICMP_TIMESTAMP_MACHINE;
	else {
		fprintf(stderr, "Error: No output formatting matched - exiting");
		exit(1);
	}

	if (args->print_timestamps == 1 && args->machine_readable == 0)
		FMT_TIMESTAMP = FMT_TIMESTAMPS_HUMAN;
	else if (args->print_timestamps == 1 && args->machine_readable == 1)
		FMT_TIMESTAMP = FMT_TIMESTAMPS_MACHINE;

	if (args->machine_readable == 1 && args->delimiter != ',') {
		char_replace(FMT_OUTPUT, ',', args->delimiter);

		if (FMT_TIMESTAMP != NULL)
			char_replace(FMT_TIMESTAMP, ',', args->delimiter);
	}

	while (1)
	{
		char * buff = malloc(100);
		struct sockaddr_in remote_addr;
		socklen_t addr_len = sizeof(remote_addr);
		int recv = recvfrom(sock_fd, buff, 100, 0, (struct sockaddr *) &remote_addr, &addr_len);

		if (recv < 0) {
			perror("Error while reading from socket: ");
			goto skip;
		}

		// Find the length of the IP header
		// so we can figure out where in
		// the buffer the ICMP header starts
		int len = (*buff & 0x0F) * 4;
		char * hdr = (char *) (buff + len);

		icmp_result result = {0};

		char ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(remote_addr.sin_addr), ip, INET_ADDRSTRLEN);

		int res;
		int32_t rtt;
		switch (*hdr) {
			case 0:
				if (args->icmp_type != 8)
					goto skip;

				res = parse_icmp_echo_reply(id, hdr, len, recv, &result);

				if (res != 0)
					goto skip;

				rtt = result.finishedTime - result.originateTime;

				if (args->print_timestamps)
					print_timestamp(FMT_TIMESTAMP);
				printf(FMT_OUTPUT, ip, result.sequence, rtt);

				receivedICMP++;

				break;
			case 14:
				if (args->icmp_type != 13)
					goto skip;

				res = parse_icmp_timestamp_reply(id, hdr, len, recv, &result);

				if (res != 0)
					goto skip;

				int32_t down_time = result.finishedTime - result.transmitTime;
				int32_t up_time = result.receiveTime - result.originateTime;
				rtt = result.finishedTime - result.originateTime;

				if (args->print_timestamps)
					print_timestamp(FMT_TIMESTAMP);
				printf(FMT_OUTPUT, ip, result.sequence, result.originateTime, result.receiveTime, result.transmitTime, result.finishedTime, rtt, down_time, up_time);

				receivedICMP++;

				break;
			default:
				goto skip;
		}

		

		skip: free(buff);		
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
		if (sentICMP > 0 && sentICMP % 100 == 0 && receivedICMP == 0) {
			fprintf(stderr, "Warning: We've sent %lu packets, but received none.\n", sentICMP);
		}

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
					fprintf(stderr, "sender: Invalid ICMP type: %u, exiting..\n", args->icmp_type);
					pthread_exit(NULL);
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
	arguments.delimiter = ',';
	arguments.fw_mark = 0;
	arguments.icmp_type = 13; // ICMP timestamps
	arguments.interface = NULL;
	arguments.machine_readable = 0;
	arguments.print_timestamps = 0;
	arguments.sleep_time = 100;
	arguments.target_spacing = 0;
	arguments.targets = malloc(sizeof(struct sockaddr_in));
	arguments.targets_len = 0;

	/* Parse our arguments; every option seen by parse_opt will
	   be reflected in arguments. */
	int ret = argp_parse (&argp, argc, argv, 0, 0, &arguments);

	if (ret != 0)
		return ret;

	fprintf(stderr, "Starting tsping %s - pinging %u targets\n", _TSPING_VERSION, arguments.targets_len);

	pthread_t receiver_thread;
	pthread_t sender_thread;

	int sock_fd;

	if ((sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1)
	{
		perror("Couldn't open raw socket (are you root?)\n");
		return 1;
	}

	if (arguments.fw_mark > 0) {
		int err = setsockopt(sock_fd, SOL_SOCKET, SO_MARK, &arguments.fw_mark, sizeof(arguments.fw_mark));

		if (err != 0) {
			fprintf(stderr, "Couldn't set fw mark %u on socket: %s\n", arguments.fw_mark, strerror(errno));
			exit(1);
		}
	}

	if (arguments.interface != NULL) {
		int err = setsockopt(sock_fd, SOL_SOCKET, SO_BINDTODEVICE, arguments.interface, strlen(arguments.interface));

		if (err != 0) {
			fprintf(stderr, "Couldn't bind socket to interface %s: %s\n", arguments.interface, strerror(errno));
			exit(1);
		}
	}

	// disable buffering on stdout
	if (setvbuf(stdout, NULL, _IONBF, 0) != 0)
    {
        perror("Something wen't wrong while disabling buffering on standard output");
        return EXIT_FAILURE;
    }

	struct thread_data data;
	data.args = &arguments;
	data.id = htons(getpid() & 0xFFFF);
	data.sock_fd = sock_fd;

	if (pthread_create(&receiver_thread, NULL, receiver_loop, (void *)&data) != 0)
	{
		perror("Failed to create receiver thread: ");
		return 1;
	}

	if (pthread_create(&sender_thread, NULL, sender_loop, (void *)&data) != 0)
	{
		perror("Failed to create receiver thread: ");
		return 1;
	}

	pthread_setname_np(receiver_thread, "receiver");
	pthread_setname_np(sender_thread, "sender");

	// Wait for threads to shut down
	pthread_join(receiver_thread, NULL);
	pthread_join(sender_thread, NULL);

	return 0;
}
