#pragma once
#include <doca_buf_inventory.h>
#include <doca_ctx.h>
#include <doca_dev.h>
#include <doca_error.h>
#include <doca_mmap.h>
#include <doca_types.h>
#include <stdbool.h>
#include <unistd.h>
#define PCI_ADDR_LEN 8
// Page size
#define PAGE_SIZE sysconf(_SC_PAGESIZE)
#define WORKQ_DEPTH 32
// Function to check if a given device is capable of executing some job
typedef doca_error_t (*jobs_check)(struct doca_devinfo *);
struct Config {
  char dev_pci_addr[PCI_ADDR_LEN];
  char rep_pci_addr[PCI_ADDR_LEN];
  int iterations;
  int byte_size;
  int warm_up;
  bool is_client;
};

doca_error_t parse_pci_addr(char const *pci_addr, struct doca_pci_bdf *out_bdf);

doca_error_t open_doca_device_with_pci(const struct doca_pci_bdf *value,
                                       jobs_check func,
                                       struct doca_dev **retval);
doca_error_t dma_jobs_is_supported(struct doca_devinfo *devinfo);