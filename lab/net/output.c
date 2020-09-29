#include <inc/lib.h>
#include "ns.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	envid_t whom;
	int perm;
	union Nsipc *buf = (union Nsipc *)REQVA;
	while(1){
		if(ipc_recv(&whom, buf, &perm) != NSREQ_OUTPUT)
			continue;			
		while(sys_netpacket_send(buf->pkt.jp_data, buf->pkt.jp_len) < 0)
			sys_yield();
	}
}
