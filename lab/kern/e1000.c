#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>

// LAB 6: Your driver code here

uint32_t e1000addr; 

struct packet{
	char data[BUFFSIZE];
};

struct tx_desc tx_desc_ring[TXRING_LEN] __attribute__((aligned (PGSIZE)));

struct packet transpacket_buf[TXRING_LEN] __attribute__((aligned (PGSIZE)));

struct rx_desc rx_desc_ring[RXRING_LEN] __attribute__((aligned (PGSIZE)));

struct packet recvpacket_buf[RXRING_LEN] __attribute__((aligned (PGSIZE)));

uint32_t mac_addr[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};



static void
init_desc(){
    int i;

    for(i = 0; i < TXRING_LEN; i++){
        memset(&tx_desc_ring[i], 0, sizeof(struct tx_desc));
        tx_desc_ring[i].addr = PADDR(&transpacket_buf[i]);
        tx_desc_ring[i].status = E1000_TXD_STATUS_DD;
        tx_desc_ring[i].cmd = E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP;
    }

    for(i = 0; i < RXRING_LEN; i++){
        memset(&rx_desc_ring[i], 0, sizeof(struct rx_desc));
        rx_desc_ring[i].addr = PADDR(&recvpacket_buf[i]);
        rx_desc_ring[i].status = 0;
    }
}   


int
pci_e1000_attach(struct pci_func * pcif) 
{
	pci_func_enable(pcif);
	init_desc();
	e1000addr = (uint32_t)mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
	cprintf("e1000: bar0 [%08x] size0 [%08x]\n", pcif->reg_base[0], pcif->reg_size[0]);
    cprintf("e1000: status [%08x]\n",E1000_REGISER(STATUS));


	E1000_REGISER(TDBAL) = PADDR(tx_desc_ring);
	E1000_REGISER(TDBAH) = 0;
	E1000_REGISER(TDLEN) = TXRING_LEN * sizeof(struct tx_desc);
	E1000_REGISER(TDH)= 0;
	E1000_REGISER(TDT) = 0;
	E1000_REGISER(TCTL) = E1000_TCTL_EN | E1000_TCTL_PSP | E1000_TCTL_CT_ETHERNET | E1000_TCTL_COLD_FULL_DUPLEX;
	E1000_REGISER(TIPG) = 10 | (8 << 10) | (12 << 20);

	E1000_REGISER(RAL) = 0x12005452;
	E1000_REGISER(RAH) = 0x00005634 | E1000_RAH_AV;
	memset(&E1000_REGISER(MTA), 0, 128 * 4);
	E1000_REGISER(IMS) = 0;
	E1000_REGISER(ICS) = 0;
	E1000_REGISER(RDBAL) = PADDR(rx_desc_ring);
	E1000_REGISER(RDBAH) = 0;
	E1000_REGISER(RDLEN) = RXRING_LEN * sizeof(struct rx_desc);
	E1000_REGISER(RDH) = 0;
	E1000_REGISER(RDT) = RXRING_LEN - 1;
	E1000_REGISER(RCTL) = E1000_RCTL_EN | E1000_RCTL_LBM | E1000_RCTL_SECRC | E1000_RCTL_BSIZE;      
	return 0;
}

int
e1000_transmit(void *addr, size_t len){
	uint32_t tdt = E1000_REGISER(TDT);
	struct tx_desc *tail_desc = &tx_desc_ring[tdt];
	if(tail_desc->status & E1000_TXD_STATUS_DD){
		if(len > BUFFSIZE)
			len = BUFFSIZE;
		tail_desc->length = len;
		memmove(&transpacket_buf[tdt], addr, tail_desc->length);
		tail_desc->status ^= E1000_TXD_STATUS_DD;
		E1000_REGISER(TDT) = (tdt+1) % TXRING_LEN;
		return 0;
	}
	return -1;
}

int
e1000_receive(void *addr, size_t *len){
	uint32_t rdt = E1000_REGISER(RDT);
	rdt = (rdt + 1) % RXRING_LEN;
	struct rx_desc *tail_desc = &rx_desc_ring[rdt];
	if(tail_desc->status & E1000_RXD_STATUS_DD){
		*len = tail_desc->length;
		memmove(addr, &recvpacket_buf[rdt], tail_desc->length);
		tail_desc->status ^= E1000_RXD_STATUS_DD;
		E1000_REGISER(RDT) = rdt;
		cprintf("hello\n");
		return 0;
	}
	return -1;
}



