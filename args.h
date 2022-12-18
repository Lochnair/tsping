#include <argp.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <stdlib.h>
#include "strtonum.h"

static char doc[] =
		"tsping -- a simple application to send ICMP echo/timestamp requests";

static char args_doc[] = "IP1 IP2 IP3 ...";

static struct argp_option options [] = {
		{"machine-readable",    'm',    0,  0, "Output results in a machine readable format"},
		{"icmp-echo",           'e',    0,  0, "Use ICMP echo requests" },
		{"icmp-ts",             't',    0,  0, "Use ICMP timestamp requests (default)" },
        {"target-spacing",   'r',    "TIME",  0, "Time to wait between pinging each target in ms (default 0)"},
        {"sleep-time",          's',    "TIME",  0, "Time to wait between each round of pinging in ms (default 100)"},
		{ 0 }
};

struct arguments
{
	struct sockaddr_in * targets;
	unsigned int targets_len, machine_readable, icmp_type, sleep_time, target_spacing;
};

/* Parse a single option. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
	/* Get the input argument from argp_parse, which we
	   know is a pointer to our arguments structure. */
	struct arguments *arguments = state->input;
    const char * errstr;

	switch (key)
	{
		case 'm':
			arguments->machine_readable = 1;
			break;
		case 'e':
			arguments->icmp_type = 8;
			break;
		case 't':
			arguments->icmp_type = 13;
			break;
        case 'r':
            arguments->target_spacing = strtonum(arg, 0, UINT32_MAX, &errstr);

            if (errstr != NULL) {
                printf("Invalid argument: %s\n", errstr);
                return -EINVAL;
            }

            break;
        case 's':
            arguments->sleep_time = strtonum(arg, 0, UINT32_MAX, &errstr);

            if (errstr != NULL) {
                printf("Invalid argument: %s\n", errstr);
                return -EINVAL;
            }

            break;
		case ARGP_KEY_ARG:
			arguments->targets = realloc(arguments->targets, (arguments->targets_len + 1) * sizeof(struct sockaddr_in));

			if (inet_pton(AF_INET, arg, &arguments->targets[arguments->targets_len].sin_addr) != 1) {
				printf("Invalid IP address: %s\n", arg);
				return -EINVAL;
			}

			arguments->targets_len += 1;
			break;
		case ARGP_KEY_END:
			if (state->arg_num < 1)
				/* Not enough arguments. */
				argp_usage (state);
			break;

		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc };