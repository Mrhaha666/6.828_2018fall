#ifndef JOS_KERN_MONITOR_H
#define JOS_KERN_MONITOR_H
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif

struct Trapframe;

// Activate the kernel monitor,
// optionally providing a trap frame indicating the current state
// (NULL if none).
void monitor(struct Trapframe *tf);


// Functions implementing monitor commands.
int mon_help(int argc, char **argv, struct Trapframe *tf);
int mon_kerninfo(int argc, char **argv, struct Trapframe *tf);
int mon_backtrace(int argc, char **argv, struct Trapframe *tf);
int mon_jdb(int argc, char **argv, struct Trapframe *tf);
int mon_e1000test(int argc, char **argv, struct Trapframe *tf);

// Activate the kernel debugger monitor,
// optionally providing a trap frame indicating the current state
// (NULL if none).
int monitor_jdb(struct Trapframe *tf);

int jdb_con(struct Trapframe *tf);
int jdb_si(struct Trapframe *tf);
int jdb_help(struct Trapframe *tf);
int jdb_quit(struct Trapframe *tf);
#endif	// !JOS_KERN_MONITOR_H
