#pragma once
#include "common.h"
#include "params.h"
#include <doca_error.h>
#define MAX_MSG_SIZE 65535
#define CC_MAX_QUEUE_SIZE 10
doca_error_t client_benchmark(struct doca_pci_bdf *dev_pcie,
                              struct doca_pci_bdf *rep_pcie,
                              struct Config *config);
