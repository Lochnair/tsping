const char * tsping_program_version = "tsping 1.0";
const char * tsping_bug_address = "contact@lochnair.net";

static char doc[] =
		"tsping -- a simple application to send ICMP echo/timestamp requests";

static char args_doc[] = "IP1 IP2 IP3 ...";

static struct argp_option options [] = {
		{"machine-readable", 'm', 0, 0, "Output results in a machine readable format"},
		{"icmp-echo",  'e', 0,      0,  "Use ICMP echo requests" },
		{"icmp-ts",    't', 0,      0,  "Use ICMP timestamp requests (default)" },
		{ 0 }
};

struct arguments
{
	struct sockaddr_in * targets;
	int targets_len, machine_readable, icmp_type;
};

/* Parse a single option. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
	/* Get the input argument from argp_parse, which we
	   know is a pointer to our arguments structure. */
	struct arguments *arguments = state->input;

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