#include <iostream>
#include <sys/neutrino.h>
#include <sys/mman.h>
#include <hw/inout.h>

extern "C" {
#include <pci/pci.h>
}

#include "kernel/drivers/net/can/sja1000/sja1000.h"
#include "kernel/include/linux/pci.h"

extern struct pci_driver adv_pci_driver;
extern struct pci_driver kvaser_pci_driver;

using namespace std;

/**
 * Command:
 *
 * 		pci-tool -vvvvv
 *
 * Output:
 *
 * B000:D05:F00 @ idx 7
 *         vid/did: 13fe/c302
 *                 <vendor id - unknown>, <device id - unknown>
 *         class/subclass/reg: 0c/09/00
 *                 CANbus Serial Bus Controller
 *         revid: 0
 *         cmd/status registers: 103/0
 *         Capabilities list (0):
 *
 *         Address Space list - 2 assigned
 *             [0] I/O, addr=c000, size=400, align: 400, attr: 32bit CONTIG ENABLED
 *             [1] I/O, addr=c400, size=400, align: 400, attr: 32bit CONTIG ENABLED
 *         Interrupt list - 0 assigned
 *         hdrType: 0
 *                 ssvid: 13fe  ?
 *                 ssid:  c302
 *
 *         Device Dependent Registers
 *                 [40] 00000000  00000000  00000000  00000000
 *                   :
 *                 [f0] 00000000  00000000  00000000  00000000
 *
 */

// NETDEVICE
#include "linux/netdevice.h"

void netif_wake_queue(struct net_device *dev)
{
}

int netif_rx(struct sk_buff *skb)
{
	return 0;
}

void netif_start_queue(struct net_device *dev)
{
}

void netif_carrier_on(struct net_device *dev)
{
	// Not called from SJA1000 driver
}

void netif_carrier_off(struct net_device *dev)
{
}

bool netif_queue_stopped(const struct net_device *dev)
{
	return false;
}

void netif_stop_queue(struct net_device *dev)
{
}
// NETDEVICE

// DEV
#include "linux/can/dev.h"

int register_candev(struct net_device *dev) {
	return 0;
}

void unregister_candev(struct net_device *dev) {
}

int open_candev(struct net_device *dev)
{
	return 0;
}

void close_candev(struct net_device *dev)
{
}
// DEV

// INTERRUPT
#include "linux/interrupt.h"

extern int
request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,
	    const char *name, void *dev)
{
	// TODO: install IRQ handler
	return 0;
}

void free_irq(unsigned int irq, void *dev_id)
{
}
// INTERRUPT

// PCI
int pci_enable_device(struct pci_dev *dev) {
	return 0;
}

void pci_disable_device(struct pci_dev *dev) {
}

uintptr_t pci_iomap(struct pci_dev *dev, int bar, unsigned long max) {
	return dev->mmap_base;
}

void pci_iounmap(struct pci_dev *dev, uintptr_t p) {
}

int pci_request_regions(struct pci_dev *, const char *) {
	return 0;
}

void pci_release_regions(struct pci_dev *) {
}
// PCI

void print_card (const pci_driver& driver) {
	std::cout << "  Name of driver: " << driver.name << std::endl;
	std::cout << "  Supported cards:" << std::endl;

	if (driver.id_table != nullptr) {
		const pci_device_id *id_table = driver.id_table;

		while (id_table->vendor != 0) {
			std::cout << std::hex << "    { "
					<< "vendor: " << id_table->vendor
					<< ", devide: " << id_table->device
					<< ", subvendor: " << id_table->subvendor
					<< ", subdevice: " << id_table->subdevice
					<< ", class: " << id_table->class_
					<< ", class_mask: " << id_table->class_mask
					<< " }" << std::endl;
			++id_table;
		}
	}
}

int main(void) {
	std::cout << "Running" << std::endl;

	std::cout << "Advantech CAN cards:" << std::endl;
	print_card(adv_pci_driver);

	std::cout << "Kvaser CAN cards:" << std::endl;
	print_card(kvaser_pci_driver);

	pci_dev pdev = {
			.vendor = 0x13fe,
			.device = 0xc302,
			.dev = { .driver_data = nullptr },
			.irq = 10
	};

	adv_pci_driver.probe(&pdev, nullptr);

	ThreadCtl(_NTO_TCTL_IO, 0);

	pci_vid_t PCI_VID_xxx = 0x13fe;

	uint_t idx = 0;
	pci_bdf_t bdf = 0;

	while ((bdf = pci_device_find(idx, PCI_VID_xxx, PCI_DID_ANY, PCI_CCODE_ANY)) != PCI_BDF_NONE)
	{
	    pci_did_t did;
	    pci_err_t r = pci_device_read_did(bdf, &did);

	    if (r == PCI_ERR_OK)
	    {
	    	std::cout << "DID: " << std::hex << did << std::endl;

	    	/* does this driver handle this device ? */
	    	if (did == 0xc302) {
	    		pci_devhdl_t hdl = pci_device_attach(bdf, pci_attachFlags_EXCLUSIVE_OWNER, &r);

	            if (hdl == NULL) {
	    			switch (r) {
	    			case PCI_ERR_EINVAL:
	    				std::cout << "Invalid flags." << std::endl;
	    				break;
	    			case PCI_ERR_ENODEV:
	    				std::cout << "The bdf argument doesn't refer to a valid device." << std::endl;
	    				break;
	    			case PCI_ERR_ATTACH_EXCLUSIVE:
	    				std::cout << "The device identified by bdf is already exclusively owned." << std::endl;
	    				break;
	    			case PCI_ERR_ATTACH_SHARED:
	    				std::cout << "The request was for exclusive attachment, but the device identified by bdf has already been successfully attached to." << std::endl;
	    				break;
	    			case PCI_ERR_ATTACH_OWNED:
	    				std::cout << "The request was for ownership of the device, but the device is already owned." << std::endl;
	    				break;
	    			case PCI_ERR_ENOMEM:
	    				std::cout << "Memory for internal resources couldn't be obtained. This may be a temporary condition." << std::endl;
	    				break;
	    			case PCI_ERR_LOCK_FAILURE:
	    				std::cout << "There was an error related to the creation, acquisition, or use of a synchronization object." << std::endl;
	    				break;
	    			case PCI_ERR_ATTACH_LIMIT:
	    				std::cout << "There have been too many attachments to the device identified by bdf." << std::endl;
	    				break;
	    			default:
	    				std::cout << "Unknown error: " << r << std::endl;
	    				break;
	    			}
	            }
	            else {
	            	std::cout << "PCI attach successful" << std::endl;

	                /* optionally determine capabilities of device */
	                uint_t capid_idx = 0;
	                pci_capid_t capid;

	                /* instead of looping could use pci_device_find_capid() to select which capabilities to use */
	                while ((r = pci_device_read_capid(bdf, &capid, capid_idx)) == PCI_ERR_OK)
	                {
	                	std::cout << "CAPID: " << std::hex << capid << std::endl;

	                    /* get next capability ID */
	                    ++capid_idx;
	                }

	                pci_ba_t ba[7];    // the maximum number of entries that can be returned
	                uintptr_t mmap_base[7];
	                int_t nba = NELEMENTS(ba);

	                /* read the address space information */
	                r = pci_device_read_ba(hdl, &nba, ba, pci_reqType_e_UNSPECIFIED);
	                if ((r == PCI_ERR_OK) && (nba > 0))
	                {
	                    for (int_t i=0; i<nba; i++)
	                    {
	                        /* mmap() the address space(s) */
	                    	std::cout << "ba[" << i << "] { addr: " << std::hex << ba[i].addr;
	                    	std::cout << ", size: " << std::hex << ba[i].size;
	                    	std::cout << ", bar_num: " << std::hex << ba[i].bar_num << " }" << std::endl;

	                    	if ((mmap_base[i] = mmap_device_io(ba[i].size, ba[i].addr)) == MAP_DEVICE_FAILED) {
	                    		switch (errno) {
	                    		case EINVAL:
	                    			std::cout << "Invalid flags type, or len is 0." << std::endl;
	                    			break;
	                    		case ENOMEM:
	                    			std::cout << "The address range requested is outside of the allowed process address range, or there wasn't enough memory to satisfy the request." << std::endl;
	                    			break;
	                    		case ENXIO:
	                    			std::cout << "The address from io for len bytes is invalid." << std::endl;
	                    			break;
	                    		case EPERM:
	                    			std::cout << "The calling process doesn't have the required permission; see procmgr_ability()." << std::endl;
	                    			break;
	        	    			default:
	        	    				std::cout << "Unknown error: " << r << std::endl;
	        	    				break;
								}
	                    	}

	                    	uint8_t x = in8(mmap_base[i]);
	                    	std::cout << "x: " << std::hex << x << std::endl;
	                    }
	                }

	                pci_irq_t irq[10];    // the maximum number of expected entries based on capabilities
	                int_t nirq = NELEMENTS(irq);

	                /* read the irq information */
                    r = pci_device_read_irq(hdl, &nirq, irq);
                    if ((r == PCI_ERR_OK) && (nirq > 0))
                    {
                        for (int_t i=0; i<nirq; i++)
                        {
                            //int_t iid = InterruptAttach(irq[i], my_isr, NULL, 0, 0);
                        	std::cout << "irq[" << i << "]: " << std::dec << irq[i] << std::endl;
                        }
                    }
	            }

	            pci_device_detach(hdl);
	    	}
	    }

	    /* get next device instance */
	    ++idx;
	}

	return EXIT_SUCCESS;
}
