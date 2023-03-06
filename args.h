#include <argp.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <stdlib.h>
#include "strtonum.h"

static char doc[] =
		"tsping -- a simple application to send ICMP echo/timestamp requests";

static char args_doc[] = "IP1 IP2 IP3 ...";

static struct argp_option options [] = {
		{"fw-mark",				'f',	"MARK",			0,						"Firewall mark to set on outgoing packets",							4 },
		{"icmp-echo",           'e',    0,  			0,						"Use ICMP echo requests",											1 },
		{"icmp-ts",             't',    0,  			0,						"Use ICMP timestamp requests (default)",							1 },
		{"interface",			'i',	"INTERFACE",	0,						"Interface to bind to",												4 },
		{"machine-readable",    'm',    "DELIMITER",	OPTION_ARG_OPTIONAL,	"Output results in a machine readable format",						3 },
		{"print-timestamps",	'D',	0,				0,						"Print UNIX timestamps for responses",								3 },
        {"sleep-time",          's',    "TIME",			0,						"Time to wait between each round of pinging in ms (default 100)",	2 },
		{"target-spacing",		'r',    "TIME",			0,						"Time to wait between pinging each target in ms (default 0)",		2 },
		{ 0 }
};

struct arguments
{
	char					delimiter;
	unsigned int			fw_mark;
	unsigned int			icmp_type;
	char * 					interface;
	unsigned int			machine_readable;
	unsigned int			print_timestamps;
	unsigned int			sleep_time;
	unsigned int			target_spacing;
	struct sockaddr_in *	targets;
	unsigned int			targets_len;
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
		case 'D':
			arguments->print_timestamps = 1;
			break;
		case 'e':
			arguments->icmp_type = 8;
			break;
		case 'f':
			arguments->fw_mark = strtonum(arg, 0, UINT32_MAX, &errstr);

            if (errstr != NULL) {
                printf("Invalid argument: %s\n", errstr);
                return -EINVAL;
            }

            break;
		case 'i':
			arguments->interface = arg;
			break;
		case 'm':
			arguments->machine_readable = 1;
			
			if (arg != NULL) {
				if(strlen(arg) > 1) {
					printf("Too long delimiter: %s\n", arg);
					return -EINVAL;
				}

				arguments->delimiter = *arg;
			}

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
		case 't':
			arguments->icmp_type = 13;
			break;
		
		case ARGP_KEY_ARG:
			arguments->targets = realloc(arguments->targets, (arguments->targets_len + 1) * sizeof(struct sockaddr_in));
			arguments->targets[arguments->targets_len].sin_family = AF_INET;
			arguments->targets[arguments->targets_len].sin_port = 0;

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