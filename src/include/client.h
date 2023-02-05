#pragma once
#include "common.h"
#include "params.h"
#include <doca_error.h>
doca_error_t client_benchmark(struct doca_pci_bdf *dev_pcie,
                              struct doca_pci_bdf *rep_pcie,
                              struct Config *config);
