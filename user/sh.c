#include <inc/lib.h>

#define BUFSIZ 1024 /* Find the buffer overrun bug! */
#define MAX_ENV_VARS 128
#define MAX_HISTORY 100
#define BUFLEN 1024
char bufs[BUFLEN];

int debug = 0;

char history[MAX_HISTORY][BUFSIZ];
int history_count = 0;
int history_index = 0;

void add_to_history(const char *cmd)
{
	if (history_count < MAX_HISTORY)
	{
		strncpy(history[history_count], cmd, BUFSIZ);
		history_count++;
	}
	else
	{
		for (int i = 1; i < MAX_HISTORY; i++)
		{
			strncpy(history[i - 1], history[i], BUFSIZ);
		}
		strncpy(history[MAX_HISTORY - 1], cmd, BUFSIZ);
	}
	history_index = history_count;
}

char *read_line(const char *prompt)
{
	int i, c, echoing;
	int cur_pos = 0; // current position of the cursor

#if JOS_KERNEL
	if (prompt != NULL)
		cprintf("%s", prompt);
#else
	if (prompt != NULL)
		fprintf(1, "%s", prompt);
#endif

	i = 0;
	echoing = iscons(0);
	history_index = history_count; // Start at the end of the history

	while (1)
	{
		c = getchar();
		if (c < 0)
		{
			if (c != -E_EOF)
				cprintf("read error: %e\n", c);
			return NULL;
		}
		else if ((c == '\b' || c == '\x7f') && cur_pos > 0)
		{ // handle backspace
			cur_pos--;
			i--;

			for (int j = cur_pos; j < i; j++)
				bufs[j] = bufs[j + 1];
			bufs[i] = '\0'; // Null-terminate the string

			if (echoing)
			{
				printf("\r%s", prompt);
				printf("%s ", bufs); // Print the updated buffer
				for (int j = i + 1; j > cur_pos; j--)
					cputchar('\b'); // Move the cursor back to the correct position
			}
		}
		else if (c == 27)
		{			   // handle arrow keys
			getchar(); // skip the '[' character
			switch (getchar())
			{
			case 'A': // up arrow
				if (history_index > 0)
				{
					history_index--;
					strcpy(bufs, history[history_index]);
					i = cur_pos = strlen(bufs);
					if (echoing)
					{
						printf("\r%s", prompt);
						printf("%s", bufs);
						for (int j = i; j < BUFLEN - 1; j++)
							cputchar(' ');
						for (int j = i; j < BUFLEN - 1; j++)
							cputchar('\b');
					}
				}
				break;
			case 'B': // down arrow
				if (history_index < history_count - 1)
				{
					history_index++;
					strcpy(bufs, history[history_index]);
					i = cur_pos = strlen(bufs);
					if (echoing)
					{
						printf("\r%s", prompt);
						printf("%s", bufs);
						for (int j = i; j < BUFLEN - 1; j++)
							cputchar(' ');
						for (int j = i; j < BUFLEN - 1; j++)
							cputchar('\b');
					}
				}
				else
				{
					history_index = history_count;
					i = cur_pos = 0;
					bufs[0] = '\0';
					if (echoing)
					{
						printf("\r%s", prompt);
						for (int j = 0; j < BUFLEN - 1; j++)
							cputchar(' ');
						printf("\r%s", prompt);
					}
				}
				break;
			case 'C': // left arrow
				if (cur_pos < i && echoing)
				{
					cur_pos++;
					cputchar(bufs[cur_pos - 1]);
				}
				break;
			case 'D': // right arrow
				if (cur_pos > 0 && echoing)
				{
					cur_pos--;
					printf("\b");
				}
				break;
			}
		}
		else if (c >= ' ' && i < BUFLEN - 1)
		{
			if (cur_pos < i)
			{
				for (int j = i; j > cur_pos; j--)
					bufs[j] = bufs[j - 1];
				bufs[cur_pos] = c;
				if (echoing)
				{
					printf("\r%s", prompt);
					printf("%s", bufs);
					for (int j = cur_pos; j < i; j++)
						cputchar('\b');
				}
			}
			else
			{
				bufs[cur_pos] = c;
				if (echoing)
					cputchar(c);
			}
			i++;
			cur_pos++;
		}
		else if (c == '\n' || c == '\r')
		{ // handle enter
			if (echoing)
				cputchar('\n');
			bufs[i] = '\0';
			return bufs;
		}
	}
}

// A new read_line function that supports command history and editing.
char *old_read_line(const char *prompt)
{
	int i, c, echoing;

#if JOS_KERNEL
	if (prompt != NULL)
		cprintf("%s", prompt);
#else
	if (prompt != NULL)
		fprintf(1, "%s", prompt);
#endif

	i = 0;
	echoing = iscons(0);
	history_index = history_count; // Start at the end of the history

	while (1)
	{
		c = getchar();
		if (c < 0)
		{
			if (c != -E_EOF)
				cprintf("read error: %e\n", c);
			return NULL;
		}
		else if ((c == '\b' || c == '\x7f') && i > 0)
		{
			if (echoing)
			{
				cputchar('\b');
				cputchar(' ');
				cputchar('\b');
			}
			i--;
		}
		else if (c == 27) // Handle arrow keys
		{
			getchar(); // Skip the '[' character
			switch (getchar())
			{
			case 'A': // Up arrow
				if (history_index > 0)
				{
					history_index--;
					strcpy(bufs, history[history_index]);
					i = strlen(bufs);
					if (echoing)
					{
						printf("\r%s%s", prompt, bufs);
						for (int j = i; j < BUFLEN - 1; j++)
							cputchar(' ');
						for (int j = i; j < BUFLEN - 1; j++)
							cputchar('\b');
					}
				}
				break;
			case 'B': // Down arrow
				if (history_index < history_count - 1)
				{
					history_index++;
					strcpy(bufs, history[history_index]);
					i = strlen(bufs);
					if (echoing)
					{
						printf("\r%s%s", prompt, bufs);
						for (int j = i; j < BUFLEN - 1; j++)
							cputchar(' ');
						for (int j = i; j < BUFLEN - 1; j++)
							cputchar('\b');
					}
				}
				else
				{
					history_index = history_count;
					i = 0;
					bufs[0] = '\0';
					if (echoing)
					{
						printf("\r%s", prompt);
						for (int j = 0; j < BUFLEN - 1; j++)
							cputchar(' ');
						for (int j = 0; j < BUFLEN - 1; j++)
							cputchar('\b');
					}
				}
				break;
			}
		}
		else if (c >= ' ' && i < BUFLEN - 1)
		{
			if (echoing)
				cputchar(c);
			bufs[i++] = c;
		}
		else if (c == '\n' || c == '\r')
		{
			if (echoing)
				cputchar('\n');
			bufs[i] = 0;
			return bufs;
		}
	}
}

// gettoken(s, 0) prepares gettoken for subsequent calls and returns 0.
// gettoken(0, token) parses a shell token from the previously set string,
// null-terminates that token, stores the token pointer in '*token',
// and returns a token ID (0, '<', '>', '|', or 'w').
// Subsequent calls to 'gettoken(0, token)' will return subsequent
// tokens from the string.
int gettoken(char *s, char **token);
void run_single_cmd(char *s);

typedef struct
{
	char name[BUFSIZ];
	char value[BUFSIZ];
} EnvVar;

static EnvVar env_vars[MAX_ENV_VARS];
static int env_var_count = 0;

// Set an environment variable
void set_env(const char *name, const char *value)
{
	for (int i = 0; i < env_var_count; i++)
	{
		if (strcmp(env_vars[i].name, name) == 0)
		{
			strncpy(env_vars[i].value, value, BUFSIZ);
			return;
		}
	}
	if (env_var_count < MAX_ENV_VARS)
	{
		strncpy(env_vars[env_var_count].name, name, BUFSIZ);
		strncpy(env_vars[env_var_count].value, value, BUFSIZ);
		env_var_count++;
	}
	// cprintf("env_var_count: %d\n", env_var_count);
	// cprintf("name: '%s', value: '%s'\n", env_vars[env_var_count - 1].name, env_vars[env_var_count - 1].value);
}

// Get the value of an environment variable
char *get_env(const char *name)
{
	// cprintf("env_var_count: %d\n", env_var_count);
	for (int i = 0; i < env_var_count; i++)
	{
		if (strcmp(env_vars[i].name, name) == 0)
		{
			return env_vars[i].value;
		}
	}
	return NULL;
}

int is_alnum(char c)
{
	return ('0' <= c && c <= '9') || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

// Expand environment variables in the input string.
void expand_env_vars(char *input, char *output, int output_size)
{
	char *p = input;
	char *q = output;
	char var_name[BUFSIZ];
	char *var_value;
	int var_len;

	while (*p && q - output < output_size - 1)
	{
		if (*p == '$')
		{
			p++;
			char *v = var_name;
			while (*p && (is_alnum(*p) || *p == '_'))
			{
				*v++ = *p++;
			}
			*v = '\0';
			var_value = get_env(var_name);
			if (var_value)
			{
				var_len = strlen(var_value);
				if (q - output + var_len < output_size - 1)
				{
					strcpy(q, var_value);
					q += var_len;
				}
			}
		}
		else
		{
			*q++ = *p++;
		}
	}
	// cprintf("output: %s\n", output);
	*q = '\0';
}

// Run multiple shell commands from 's'.
void runcmd(char *s)
{
	char *cmd;
	// detect if there is command grouping
	// parse the command string into multiple commands by ';'
	while (true)
	{
		cmd = s;
		s = strchr(s, ';');
		if (s)
			*s++ = '\0';
		while (*cmd == ' ')
			cmd++;
		int r;
		if (*cmd)
		{
			// cprintf("cmd: %s\n", cmd);
			if ((r = fork()) == 0)
			{
				run_single_cmd(cmd);
				exit();
			}
			wait(r);
		}
		if (!s)
			break;
	}

	// done!
	exit();
}

// Parse a shell command from string 's' and execute it.
// Do not return until the shell command is finished.
// runcmd() is called in a forked child,
// so it's OK to manipulate file descriptor state.
#define MAXARGS 16
void run_single_cmd(char *s)
{
	char *argv[MAXARGS], *t, argv0buf[BUFSIZ];
	int argc, c, i, r, p[2], fd, pipe_child;
	char expanded_cmd[BUFSIZ];

	pipe_child = 0;

	// Challenge: implement background commands.
	int background = 0;

	// Expand environment variables in the command string
	expand_env_vars(s, expanded_cmd, BUFSIZ);
	gettoken(expanded_cmd, 0);

again:
	argc = 0;
	while (1)
	{
		switch ((c = gettoken(0, &t)))
		{

		case 'w': // Add an argument
			if (argc == MAXARGS)
			{
				cprintf("too many arguments\n");
				exit();
			}
			argv[argc++] = t;
			break;

		case '<': // Input redirection
			// Grab the filename from the argument list
			if (gettoken(0, &t) != 'w')
			{
				cprintf("syntax error: < not followed by word\n");
				exit();
			}
			// Open 't' for reading as file descriptor 0
			// (which environments use as standard input).
			// We can't open a file onto a particular descriptor,
			// so open the file as 'fd',
			// then check whether 'fd' is 0.
			// If not, dup 'fd' onto file descriptor 0,
			// then close the original 'fd'.

			// LAB 5: Your code here.
			if ((fd = open(t, O_RDONLY)) < 0)
			{
				cprintf("open %s for read: %e", t, fd);
				exit();
			}
			if (fd != 0)
			{
				dup(fd, 0);
				close(fd);
			}

			break;

		case '>': // Output redirection
			// Grab the filename from the argument list
			if (gettoken(0, &t) != 'w')
			{
				cprintf("syntax error: > not followed by word\n");
				exit();
			}
			if ((fd = open(t, O_WRONLY | O_CREAT | O_TRUNC)) < 0)
			{
				cprintf("open %s for write: %e", t, fd);
				exit();
			}
			if (fd != 1)
			{
				dup(fd, 1);
				close(fd);
			}
			break;

		case '|': // Pipe
			if (debug)
				cprintf("PIPE in\n");
			if ((r = pipe(p)) < 0)
			{
				cprintf("pipe: %e", r);
				exit();
			}
			if (debug)
				cprintf("PIPE: %d %d\n", p[0], p[1]);
			if ((r = fork()) < 0)
			{
				cprintf("fork: %e", r);
				exit();
			}
			if (r == 0)
			{
				if (p[0] != 0)
				{
					dup(p[0], 0);
					close(p[0]);
				}
				close(p[1]);
				goto again;
			}
			else
			{
				pipe_child = r;
				if (p[1] != 1)
				{
					dup(p[1], 1);
					close(p[1]);
				}
				close(p[0]);
				goto runit;
			}
			panic("| not implemented");
			break;

		// Challenge: implement background commands.
		case '&': // Background
			background = 1;
			if ((r = fork()) == 0)
			{
				// Child process
				close(p[0]);
				close(p[1]);
				goto runit;
			}
			else if (r > 0)
			{
				// Parent process
				// Do not wait for the child process
				cprintf("[%08x] background process %s %08x\n", thisenv->env_id, argv[0], r);
				close(p[0]);
				close(p[1]);
				goto again;
			}
			else
			{
				// Fork failed
				panic("fork");
			}

		case 0: // String is complete
			// Run the current command!
			if (background)
			{
				// Wait for background process to complete
				wait(r);
				background = 0;
				cprintf("[%08x] background process %s %08x exited\n", thisenv->env_id, argv[0], r);
			}
			goto runit;

		default:
			panic("bad return %d from gettoken", c);
			break;
		}
	}

runit:
	// Return immediately if command line was empty.
	if (argc == 0)
	{
		if (debug)
			cprintf("EMPTY COMMAND\n");
		return;
	}

	// Check for setenv command
	if (strcmp(argv[0], "setenv") == 0)
	{
		if (argc != 3)
		{
			cprintf("usage: setenv <name> <value>\n");
			return;
		}
		cprintf("Setting environment variable %s to %s\n", argv[1], argv[2]);
		set_env(argv[1], argv[2]);
		return;
	}

	// Clean up command line.
	// Read all commands from the filesystem: add an initial '/' to
	// the command name.
	// This essentially acts like 'PATH=/'.
	if (argv[0][0] != '/')
	{
		argv0buf[0] = '/';
		strcpy(argv0buf + 1, argv[0]);
		argv[0] = argv0buf;
	}
	argv[argc] = 0;

	// Print the command.
	if (debug)
	{
		cprintf("[%08x] SPAWN:", thisenv->env_id);
		for (i = 0; argv[i]; i++)
			cprintf(" %s", argv[i]);
		cprintf("\n");
	}

	// Spawn the command!
	if ((r = spawn(argv[0], (const char **)argv)) < 0)
		cprintf("spawn %s: %e\n", argv[0], r);

	// In the parent, close all file descriptors and wait for the
	// spawned command to exit.
	close_all();
	if (r >= 0)
	{
		if (debug)
			cprintf("[%08x] WAIT %s %08x\n", thisenv->env_id, argv[0], r);

		wait(r);
		if (debug)
			cprintf("[%08x] wait finished\n", thisenv->env_id);
	}

	// If we were the left-hand part of a pipe,
	// wait for the right-hand part to finish.
	if (pipe_child)
	{
		if (debug)
			cprintf("[%08x] WAIT pipe_child %08x\n", thisenv->env_id, pipe_child);
		wait(pipe_child);
		if (debug)
			cprintf("[%08x] wait finished\n", thisenv->env_id);
	}

	// Done!
	// exit();
}

// Get the next token from string s.
// Set *p1 to the beginning of the token and *p2 just past the token.
// Returns
//	0 for end-of-string;
//	< for <;
//	> for >;
//	| for |;
//	w for a word.
//  " for a quoted string.
//
// Eventually (once we parse the space where the \0 will go),
// words get nul-terminated.
#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()\""

int _gettoken(char *s, char **p1, char **p2)
{
	int t;
	bool in_quotes = false;

	if (s == 0)
	{
		if (debug > 1)
			cprintf("GETTOKEN NULL\n");
		return 0;
	}

	if (debug > 1)
		cprintf("GETTOKEN: %s\n", s);

	*p1 = 0;
	*p2 = 0;

	while (strchr(WHITESPACE, *s))
		*s++ = 0;
	if (*s == 0)
	{
		if (debug > 1)
			cprintf("EOL\n");
		return 0;
	}
	if (strchr(SYMBOLS, *s) && *s != '"')
	{
		t = *s;
		*p1 = s;
		*s++ = 0;
		*p2 = s;
		if (debug > 1)
			cprintf("TOK %c\n", t);
		return t;
	}
	if (*s == '"')
	{
		in_quotes = true;
		s++;
	}
	*p1 = s;
	while (*s && (in_quotes || !strchr(WHITESPACE SYMBOLS, *s)))
	{
		if (*s == '"')
		{
			in_quotes = false;
			*s = 0;
			break;
		}
		s++;
	}
	*p2 = s;
	if (debug > 1)
	{
		t = **p2;
		**p2 = 0;
		cprintf("WORD: %s\n", *p1);
		**p2 = t;
	}
	return 'w';
}

int gettoken(char *s, char **p1)
{
	static int c, nc;
	static char *np1, *np2;

	if (s)
	{
		nc = _gettoken(s, &np1, &np2);
		return 0;
	}
	c = nc;
	*p1 = np1;
	nc = _gettoken(np2, &np1, &np2);
	return c;
}

void usage(void)
{
	cprintf("usage: sh [-dix] [command-file]\n");
	exit();
}

void umain(int argc, char **argv)
{
	int r, interactive, echocmds;
	struct Argstate args;

	set_env("HOME", "/");
	set_env("PATH", "/");
	set_env("USER", "jos");
	set_env("hello", "world");

	interactive = '?';
	echocmds = 0;
	argstart(&argc, argv, &args);
	while ((r = argnext(&args)) >= 0)
		switch (r)
		{
		case 'd':
			debug++;
			break;
		case 'i':
			interactive = 1;
			break;
		case 'x':
			echocmds = 1;
			break;
		default:
			usage();
		}

	if (argc > 2)
		usage();
	if (argc == 2)
	{
		close(0);
		if ((r = open(argv[1], O_RDONLY)) < 0)
			panic("open %s: %e", argv[1], r);
		assert(r == 0);
	}
	if (interactive == '?')
		interactive = iscons(0);

	while (1)
	{
		char *buf;

		buf = read_line(interactive ? "$ " : NULL);
		if (buf == NULL)
		{
			if (debug)
				cprintf("EXITING\n");
			exit(); // end of file
		}
		if (strlen(buf) > 0)
		{
			add_to_history(buf);
		}
		if (debug)
			cprintf("LINE: %s\n", buf);
		if (buf[0] == '#')
			continue;
		if (echocmds)
			printf("# %s\n", buf);
		if (debug)
			cprintf("BEFORE FORK\n");
		if ((r = fork()) < 0)
			panic("fork: %e", r);
		if (debug)
			cprintf("FORK: %d\n", r);
		if (r == 0)
		{
			runcmd(buf);
			exit();
		}
		else
			wait(r);
	}
}
