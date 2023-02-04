#include "include/common.h"
#include <stdlib.h>
#include <string.h>
#include <doca_log.h>
#include <doca_dma.h>
DOCA_LOG_REGISTER(common.c);

/// @brief parse the PCIe address to the out_buf
/// @param pci_addr [in]: PCIe address.
/// @param out_bdf [out]: store the PCIe address.
/// @return
doca_error_t parse_pci_addr(char const *pci_addr,
                            struct doca_pci_bdf *out_bdf) {
  unsigned int bus_bitmask = 0xFFFFFF00;
  unsigned int dev_bitmask = 0xFFFFFFE0;
  unsigned int func_bitmask = 0xFFFFFFF8;
  uint32_t tmpu;
  char tmps[4];

  if (pci_addr == NULL || strlen(pci_addr) != 7 || pci_addr[2] != ':' ||
      pci_addr[5] != '.')
    return DOCA_ERROR_INVALID_VALUE;

  tmps[0] = pci_addr[0];
  tmps[1] = pci_addr[1];
  tmps[2] = '\0';
  tmpu = strtoul(tmps, NULL, 16);
  if ((tmpu & bus_bitmask) != 0)
    return DOCA_ERROR_INVALID_VALUE;
  out_bdf->bus = tmpu;

  tmps[0] = pci_addr[3];
  tmps[1] = pci_addr[4];
  tmps[2] = '\0';
  tmpu = strtoul(tmps, NULL, 16);
  if ((tmpu & dev_bitmask) != 0)
    return DOCA_ERROR_INVALID_VALUE;
  out_bdf->device = tmpu;

  tmps[0] = pci_addr[6];
  tmps[1] = '\0';
  tmpu = strtoul(tmps, NULL, 16);
  if ((tmpu & func_bitmask) != 0)
    return DOCA_ERROR_INVALID_VALUE;
  out_bdf->function = tmpu;

  return DOCA_SUCCESS;
}

/// @brief Open a DOCA device according to a given PCIe adderss.
/// @param value [in]: PCIe address.
/// @param func [in]: pointer to a function that checks if the device have some
/// job capabilities (Ignored if set to NULL).
/// @param retval [out]: pointer to doca_dev struct, NULL if not found.
/// @return DOCA_SUCCESS on success and DOCA_ERROR otherwise.
doca_error_t open_doca_device_with_pci(const struct doca_pci_bdf *value,
                                       jobs_check func,
                                       struct doca_dev **retval) {
  struct doca_devinfo **dev_list;
  uint32_t nb_devs;
  struct doca_pci_bdf buf = {};
  int res;
  size_t i;

  /* Set default return value */
  *retval = NULL;

  res = doca_devinfo_list_create(&dev_list, &nb_devs);
  if (res != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to load doca devices list. Doca_error value: %d", res);
    return res;
  }

  /* Search */
  for (i = 0; i < nb_devs; i++) {
    res = doca_devinfo_get_pci_addr(dev_list[i], &buf);
    // 如果我们想用的那个的pci是可用的
    if (res == DOCA_SUCCESS && buf.raw == value->raw) {
      /* If any special capabilities are needed */
      if (func != NULL && func(dev_list[i]) != DOCA_SUCCESS)
        continue;

      /* if device can be opened */
      res = doca_dev_open(dev_list[i], retval);
      if (res == DOCA_SUCCESS) {
        doca_devinfo_list_destroy(dev_list);
        return res;
      }
    }
  }

  DOCA_LOG_ERR("Matching device not found.");
  res = DOCA_ERROR_NOT_FOUND;

  doca_devinfo_list_destroy(dev_list);
  return res;
}
doca_error_t
dma_jobs_is_supported(struct doca_devinfo *devinfo)
{
	return doca_dma_job_get_supported(devinfo, DOCA_DMA_JOB_MEMCPY);
}