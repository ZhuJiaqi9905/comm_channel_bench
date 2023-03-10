#pragma once
#include <doca_buf_inventory.h>
#include <doca_comm_channel.h>
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
#define MAX_ARG_LEN 1024

// Function to check if a given device is capable of executing some job
typedef doca_error_t (*jobs_check)(struct doca_devinfo *);
struct Config {
  char dev_pci_addr[PCI_ADDR_LEN];
  char rep_pci_addr[PCI_ADDR_LEN];
  int iterations;
  int byte_size;
  int warm_up;
  bool is_client;
  char server_name[MAX_ARG_LEN];
};
void generate_random_data(uint8_t *buf, size_t len);
doca_error_t parse_pci_addr(char const *pci_addr, struct doca_pci_bdf *out_bdf);

doca_error_t open_doca_device_with_pci(const struct doca_pci_bdf *value,
                                       jobs_check func,
                                       struct doca_dev **retval);
doca_error_t open_doca_device_rep_with_pci(struct doca_dev *local,
                                           enum doca_dev_rep_filter filter,
                                           struct doca_pci_bdf *pci_bdf,
                                           struct doca_dev_rep **retval);
doca_error_t dma_jobs_is_supported(struct doca_devinfo *devinfo);
doca_error_t recv_msg(struct doca_comm_channel_ep_t *ep,
                      struct doca_comm_channel_addr_t *peer_addr, uint8_t *msg,
                      size_t len);
doca_error_t send_msg(struct doca_comm_channel_ep_t *ep,
                      struct doca_comm_channel_addr_t *peer_addr,
                      const uint8_t *msg, size_t len);