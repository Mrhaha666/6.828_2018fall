// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>
#include <kern/env.h>
#include <kern/e1000.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

struct Command_jdb{
	const char *name;
	const char *desc;
	// return -1 to force jdb monitor to exit
	int (*func)(struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Display information about function-call stack", mon_backtrace},
	{ "jdb", "user's program debugger in JOS", mon_jdb },
	{ "e1000test", "test e1000 transmit", mon_e1000test },
};

static struct Command_jdb commands_jdb[] = {
	{ "help", "Display this list of commands", jdb_help },
        { "si", "Single Step", jdb_si },
        { "c", "Continue execution", jdb_con },
        { "q", "Exit debugger", jdb_quit },
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_e1000test(int argc, char **argv, struct Trapframe *tf){
	char *test = "hello world\n";
	return e1000_transmit(test, strlen(test));
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
	unsigned int cur_ebp;
	unsigned int last_ebp;
	unsigned int eip;
	struct Eipdebuginfo info;
	cprintf("Stack backtrace:\n");
	cur_ebp = read_ebp();
        while(cur_ebp){
		last_ebp = *(unsigned int *)cur_ebp;
      		eip = *((unsigned int *)cur_ebp + 1);
		cprintf("  ebp %08x  ", cur_ebp);
		cprintf("eip %08x  ", eip);
		cprintf("args ");
		int i;
		for(i = 0; i < 5; ++i){
			cprintf("%08x ", *((unsigned int *)cur_ebp + i + 2));
		}
		cprintf("\n");
		debuginfo_eip(eip, &info);
		cprintf("       %s:%d: %.*s+%d\n", info.eip_file, info.eip_line, info.eip_fn_namelen, info.eip_fn_name, eip - info.eip_fn_addr);
		cur_ebp = last_ebp;
	}
	return 0;
}

int mon_jdb(int argc, char **argv, struct Trapframe *tf)
{
	return monitor_jdb(tf);
}

/***** Implementations of basic kernel debugger monitor commands *****/
int 
jdb_con(struct Trapframe *tf)
{
	if(!curenv){
		cprintf("there isn't any user's env is running\n");
		return -1;	
	}
	if(!tf || !(tf->tf_trapno != T_BRKPT || tf->tf_trapno != T_DEBUG)){
		cprintf("curenv %d's trap isn't a breakpoint or debug trap\n", curenv->env_id);
		return -1;	
	}
	tf->tf_eflags &= ~FL_TF;
    return -1;
}

int 
jdb_si(struct Trapframe *tf)
{
	if(!curenv){
		cprintf("there isn't any user's env is running\n");
		return -1;	
	}
	if(!tf || !(tf->tf_trapno != T_BRKPT || tf->tf_trapno != T_DEBUG)){
		cprintf("curenv %d's trap isn't a breakpoint or debug trap\n", curenv->env_id);
		return -1;	
	}
	tf->tf_eflags |= FL_TF;
    return -1;
}

int 
jdb_help(struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands_jdb); i++)
		cprintf("%s - %s\n", commands_jdb[i].name, commands_jdb[i].desc);
	return 0;
}

int 
jdb_quit(struct Trapframe *tf)
{
	return jdb_con(tf);
}

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

/***** Kernel debugger monitor command interpreter *****/
static int runcmd_jdb(char *buf, struct Trapframe *tf){
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands_jdb); i++) {
		if (strcmp(argv[0], commands_jdb[i].name) == 0)
			return commands_jdb[i].func(tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

int
monitor_jdb(struct Trapframe *tf)
{
    char *buf;
    if (tf)
        cprintf("=> 0x%08x\n", tf->tf_eip);
    while(1) {
        buf = readline("(jdb) ");
        if (buf)
            if (runcmd_jdb(buf, tf) < 0)
                break;
    }
    return -1;
}

/***** Kernel monitor command interpreter *****/



static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
