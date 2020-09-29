#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include<kern/pci.h>

#define BUFFSIZE 1518
#define PCI_E1000_VENDOR 0x8086
#define PCI_E1000_DEVICE 0x100E
#define E1000_REGISER(OFFSET) (*(uint32_t *)(e1000addr+(OFFSET)))
#define TXRING_LEN 64
#define RXRING_LEN 128 
#define E1000_TXD_STATUS_DD  0x1
#define E1000_TXD_CMD_RS     0x8
#define E1000_TXD_CMD_EOP    0x1
#define E1000_TCTL_EN        0x2
#define E1000_TCTL_PSP       0x8
#define E1000_TCTL_CT_ETHERNET 0x800
#define E1000_TCTL_COLD_FULL_DUPLEX 0x40000 
#define E1000_RXD_STATUS_DD  0x1
#define E1000_RCTL_EN  	           0x2
#define E1000_RCTL_LBM             0x0
#define E1000_RCTL_SECRC           0x4000000
#define E1000_RCTL_BSIZE           0x0
#define E1000_RAH_AV               0x80000000

  
#define STATUS 0x00000008
#define TDBAL  0x00003800
#define TDBAH  0x00003804
#define TDLEN  0x00003808
#define TDH    0x00003810
#define TDT    0x00003818
#define TCTL   0x00000400
#define TIPG   0x00000410
#define RAL    0x00005400
#define RAH    0x00005404
#define MTA    0x00005200
#define IMS    0x000000d0
#define ICS    0x000000c8
#define RDBAL  0x00002800
#define RDBAH  0x00002804
#define RDLEN  0x00002808  
#define RDH    0x00002810
#define RDT    0x00002818
#define RCTL   0x00000100

struct tx_desc
{
	uint64_t addr;
	uint16_t length;
	uint8_t cso;
	uint8_t cmd;
	uint8_t status;
	uint8_t css;
	uint16_t special;
}__attribute__((packed));

struct rx_desc
{
	uint64_t addr;
	uint16_t length;
	uint16_t pc;
	uint8_t status;
	uint8_t err;
	uint16_t special;	
}__attribute__((packed));

int pci_e1000_attach(struct pci_func * pcif);
int e1000_transmit(void *addr, size_t len);
int e1000_receive(void *addr, size_t *len);



#endif  // SOL >= 6


