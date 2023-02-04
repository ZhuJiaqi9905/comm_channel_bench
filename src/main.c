#include "include/common.h"
#include "include/params.h"
#include "include/client.h"
#include "include/server.h"
#include <doca_argp.h>
#include <doca_buf.h>
#include <doca_dev.h>
#include <doca_dma.h>
#include <doca_error.h>
#include <doca_log.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
DOCA_LOG_REGISTER(main.c);

int main(int argc, char **argv) {
  // Read params
  struct Config config = {
      .byte_size = 1024,
      .iterations = 1,
      .dev_pci_addr = "03:00.0",
      .rep_pci_addr = "31:00.0",
      .warm_up = 1,
      .is_client = false,
  };
  doca_error_t result = DOCA_SUCCESS;
  result = doca_argp_init("comm_channel_bench", &config);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to init ARGP resources: %s",
                 doca_get_error_string(result));
    return EXIT_FAILURE;
  }
  result = register_params();
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to register DMA sample parameters: %s",
                 doca_get_error_string(result));
    return EXIT_FAILURE;
  }
  result = doca_argp_start(argc, argv);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to parse sample input: %s",
                 doca_get_error_string(result));
    return EXIT_FAILURE;
  }
  DOCA_LOG_INFO("dev_pci_addr: %s, rep_pci_addr: %s, byte_size: %d, warm up "
                "%d, iterations %d, is_client %d",
                config.dev_pci_addr, config.rep_pci_addr, config.byte_size,
                config.warm_up, config.iterations, config.is_client);
  // parse dev PCIe address
  struct doca_pci_bdf dev_pcie;
  result = parse_pci_addr(config.dev_pci_addr, &dev_pcie);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to parse pci address: %s",
                 doca_get_error_string(result));
    doca_argp_destroy();
    return EXIT_FAILURE;
  }
  // parse representor PCIe address
  struct doca_pci_bdf rep_pcie;
  result = parse_pci_addr(config.rep_pci_addr, &rep_pcie);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to parse pci address: %s",
                 doca_get_error_string(result));
    doca_argp_destroy();
    return EXIT_FAILURE;
  }
  // start benchmark
  if (config.is_client) {
    result = client_benchmark(&dev_pcie, &rep_pcie, &config);
    if (result != DOCA_SUCCESS) {
      doca_argp_destroy();
      return EXIT_FAILURE;
    }
  } else {
    result = server_benchmark(&dev_pcie, &rep_pcie, &config);
    if (result != DOCA_SUCCESS) {
      doca_argp_destroy();
      return EXIT_FAILURE;
    }
  }
  doca_argp_destroy();
  return EXIT_SUCCESS;
}
