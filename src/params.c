#include "include/params.h"
#include "include/common.h"
#include <doca_argp.h>
#include <doca_log.h>
#include <string.h>
DOCA_LOG_REGISTER(params.c);
static doca_error_t dev_pci_callback(void *param, void *config) {
  struct Config *conf = (struct Config *)config;
  const char *addr = (char *)param;
  int addr_len = strlen(addr);
  if (addr_len >= PCI_ADDR_LEN) {
    DOCA_LOG_ERR("Entered dev pci address exceeded buffer size of: %d",
                 PCI_ADDR_LEN - 1);
    return DOCA_ERROR_INVALID_VALUE;
  }
  strcpy(conf->dev_pci_addr, addr);
  return DOCA_SUCCESS;
}
static doca_error_t rep_pci_callback(void *param, void *config) {
  struct Config *conf = (struct Config *)config;
  const char *addr = (char *)param;
  int addr_len = strlen(addr);
  if (addr_len >= PCI_ADDR_LEN) {
    DOCA_LOG_ERR("Entered rep pci address exceeded buffer size of: %d",
                 PCI_ADDR_LEN - 1);
    return DOCA_ERROR_INVALID_VALUE;
  }
  strcpy(conf->rep_pci_addr, addr);
  return DOCA_SUCCESS;
}
static doca_error_t iteration_callback(void *param, void *config) {
  struct Config *conf = (struct Config *)config;
  int iteration = *(int *)param;
  if (iteration <= 0) {
    DOCA_LOG_ERR("Iteration needs to be positive, but got %d", iteration);
    return DOCA_ERROR_INVALID_VALUE;
  }
  conf->iterations = iteration;
  return DOCA_SUCCESS;
}
static doca_error_t warm_up_callback(void *param, void *config) {
  struct Config *conf = (struct Config *)config;
  int warm_up = *(int *)param;
  if (warm_up <= 0) {
    DOCA_LOG_ERR("Iteration needs to be positive, but got %d", warm_up);
    return DOCA_ERROR_INVALID_VALUE;
  }
  conf->warm_up = warm_up;
  return DOCA_SUCCESS;
}
static doca_error_t byte_size_callback(void *param, void *config) {
  struct Config *conf = (struct Config *)config;
  int byte_size = *(int *)param;
  if (byte_size < 8) {
    DOCA_LOG_ERR("Data size needs to > 8, but got %d", byte_size);
    return DOCA_ERROR_INVALID_VALUE;
  }
  conf->byte_size = byte_size;
  return DOCA_SUCCESS;
}
static doca_error_t is_client_callback(void *param, void *config) {
  struct Config *conf = (struct Config *)config;
  conf->is_client = true;
  return DOCA_SUCCESS;
}
static doca_error_t server_name_callback(void *param, void *config) {
  struct Config *conf = (struct Config *)config;
  const char *name = (char *)param;
  int len = strlen(name);
  if (len >= MAX_ARG_LEN) {
    DOCA_LOG_ERR("Entered server name exceeded buffer size of: %d",
                 MAX_ARG_LEN - 1);
    return DOCA_ERROR_INVALID_VALUE;
  }
  strcpy(conf->server_name, name);
  return DOCA_SUCCESS;
}
doca_error_t register_params() {

  doca_error_t result = DOCA_SUCCESS;
  // Create and register PCI address param
  struct doca_argp_param *dev_pci_addr_param;
  result = doca_argp_param_create(&dev_pci_addr_param);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to create ARGP param: %s",
                 doca_get_error_string(result));
    return result;
  }
  doca_argp_param_set_short_name(dev_pci_addr_param, "d");
  doca_argp_param_set_long_name(dev_pci_addr_param, "dev-pci");
  doca_argp_param_set_description(dev_pci_addr_param,
                                  "Comm Channel DOCA device PCI address");
  doca_argp_param_set_callback(dev_pci_addr_param, dev_pci_callback);
  doca_argp_param_set_type(dev_pci_addr_param, DOCA_ARGP_TYPE_STRING);
  result = doca_argp_register_param(dev_pci_addr_param);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to register program param: %s",
                 doca_get_error_string(result));
    return result;
  }

  // Create and register representor PCI address param
  struct doca_argp_param *rep_pci_addr_param;
  result = doca_argp_param_create(&rep_pci_addr_param);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to create ARGP param: %s",
                 doca_get_error_string(result));
    return result;
  }
  doca_argp_param_set_short_name(rep_pci_addr_param, "r");
  doca_argp_param_set_long_name(rep_pci_addr_param, "rep-pci");
  doca_argp_param_set_description(
      rep_pci_addr_param,
      "Comm Channel DOCA device representor PCI address (needed only on DPU)");
  doca_argp_param_set_callback(rep_pci_addr_param, rep_pci_callback);
  doca_argp_param_set_type(rep_pci_addr_param, DOCA_ARGP_TYPE_STRING);
  result = doca_argp_register_param(rep_pci_addr_param);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to register program param: %s",
                 doca_get_error_string(result));
    return result;
  }

  // Create and register iteration param
  struct doca_argp_param *iteration_param;
  result = doca_argp_param_create(&iteration_param);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to create ARGP param: %s",
                 doca_get_error_string(result));
    return result;
  }
  doca_argp_param_set_short_name(iteration_param, "i");
  doca_argp_param_set_long_name(iteration_param, "iteration");
  doca_argp_param_set_description(iteration_param,
                                  "DMA iteration times for benchmark");
  doca_argp_param_set_callback(iteration_param, iteration_callback);
  doca_argp_param_set_type(iteration_param, DOCA_ARGP_TYPE_INT);
  result = doca_argp_register_param(iteration_param);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to register program param: %s",
                 doca_get_error_string(result));
    return result;
  }

  // Create and register warm_up param
  struct doca_argp_param *warm_up_param;
  result = doca_argp_param_create(&warm_up_param);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to create ARGP param: %s",
                 doca_get_error_string(result));
    return result;
  }
  doca_argp_param_set_short_name(warm_up_param, "w");
  doca_argp_param_set_long_name(warm_up_param, "warm-up");
  doca_argp_param_set_description(warm_up_param, "Warm up times for benchmark");
  doca_argp_param_set_callback(warm_up_param, warm_up_callback);
  doca_argp_param_set_type(warm_up_param, DOCA_ARGP_TYPE_INT);
  result = doca_argp_register_param(warm_up_param);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to register program param: %s",
                 doca_get_error_string(result));
    return result;
  }
  // Create and register byte_size param
  struct doca_argp_param *byte_size_param;
  result = doca_argp_param_create(&byte_size_param);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to create ARGP param: %s",
                 doca_get_error_string(result));
    return result;
  }
  doca_argp_param_set_short_name(byte_size_param, "s");
  doca_argp_param_set_long_name(byte_size_param, "size");
  doca_argp_param_set_description(byte_size_param, "DMA data size");
  doca_argp_param_set_callback(byte_size_param, byte_size_callback);
  doca_argp_param_set_type(byte_size_param, DOCA_ARGP_TYPE_INT);
  result = doca_argp_register_param(byte_size_param);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to register program param: %s",
                 doca_get_error_string(result));
    return result;
  }
  // Create and register is_client param
  struct doca_argp_param *is_client_param;
  result = doca_argp_param_create(&is_client_param);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to create ARGP param: %s",
                 doca_get_error_string(result));
    return result;
  }
  doca_argp_param_set_short_name(is_client_param, "c");
  doca_argp_param_set_long_name(is_client_param, "is-client");
  doca_argp_param_set_description(is_client_param, "Is client or server");
  doca_argp_param_set_callback(is_client_param, is_client_callback);
  doca_argp_param_set_type(is_client_param, DOCA_ARGP_TYPE_BOOLEAN);
  result = doca_argp_register_param(is_client_param);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to register program param: %s",
                 doca_get_error_string(result));
    return result;
  }
  // Create and register server_name param
  struct doca_argp_param *server_name_param;
  result = doca_argp_param_create(&server_name_param);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to create ARGP param: %s",
                 doca_get_error_string(result));
    return result;
  }
  doca_argp_param_set_short_name(server_name_param, "n");
  doca_argp_param_set_long_name(server_name_param, "server-name");
  doca_argp_param_set_description(server_name_param, "The server name");
  doca_argp_param_set_callback(server_name_param, server_name_callback);
  doca_argp_param_set_type(server_name_param, DOCA_ARGP_TYPE_STRING);
  result = doca_argp_register_param(server_name_param);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to register program param: %s",
                 doca_get_error_string(result));
    return result;
  }
  return DOCA_SUCCESS;
}