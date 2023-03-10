#include "include/common.h"
#include <doca_dev.h>
#include <doca_dma.h>
#include <doca_log.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
DOCA_LOG_REGISTER(common.c);
const static unsigned char all_char[] =
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
#define ALL_CHAR_LEN sizeof(all_char)

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
    // ??????????????????????????????pci????????????
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
/// @brief Open a DOCA device representor according to a given PCIe adderss.
/// @param local [in] a local DOCA device with access to representors.
/// @param  filter [in] filter Bitmap filter of representor types.
/// @param pci_bdf [in] PCIe address of the representor.
/// @param retval [out] Initialized representor doca device instance on success.
/// @return
doca_error_t open_doca_device_rep_with_pci(struct doca_dev *local,
                                           enum doca_dev_rep_filter filter,
                                           struct doca_pci_bdf *pci_bdf,
                                           struct doca_dev_rep **retval) {
  uint32_t nb_rdevs = 0;
  struct doca_devinfo_rep **rep_dev_list = NULL;
  struct doca_pci_bdf queried_pci_bdf;
  doca_error_t result;
  size_t i;

  *retval = NULL;

  /* Search */
  result =
      doca_devinfo_rep_list_create(local, filter, &rep_dev_list, &nb_rdevs);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to create devinfo representors list. Representor "
                 "devices are available only on DPU, do not run on Host.");
    return DOCA_ERROR_INVALID_VALUE;
  }

  for (i = 0; i < nb_rdevs; i++) {
    result = doca_devinfo_rep_get_pci_addr(rep_dev_list[i], &queried_pci_bdf);

    DOCA_LOG_INFO("pci_buf %x%x%x", queried_pci_bdf.bus, queried_pci_bdf.device,
                  queried_pci_bdf.function);
    if (result == DOCA_SUCCESS && queried_pci_bdf.raw == pci_bdf->raw &&
        doca_dev_rep_open(rep_dev_list[i], retval) == DOCA_SUCCESS) {
      doca_devinfo_rep_list_destroy(rep_dev_list);
      return DOCA_SUCCESS;
    }
  }

  DOCA_LOG_ERR("Matching device not found.");
  doca_devinfo_rep_list_destroy(rep_dev_list);
  return DOCA_ERROR_NOT_FOUND;
}
doca_error_t dma_jobs_is_supported(struct doca_devinfo *devinfo) {
  return doca_dma_job_get_supported(devinfo, DOCA_DMA_JOB_MEMCPY);
}

doca_error_t send_msg(struct doca_comm_channel_ep_t *ep,
                      struct doca_comm_channel_addr_t *peer_addr,
                      const uint8_t *msg, size_t len) {
  doca_error_t result = DOCA_SUCCESS;
  while (
      (result = doca_comm_channel_ep_sendto(ep, msg, len, DOCA_CC_MSG_FLAG_NONE,
                                            peer_addr)) == DOCA_ERROR_AGAIN) {
  }
  return result;
}
doca_error_t recv_msg(struct doca_comm_channel_ep_t *ep,
                      struct doca_comm_channel_addr_t *peer_addr, uint8_t *msg,
                      size_t len) {
  doca_error_t result = DOCA_SUCCESS;
  size_t remain_len = len;
  while (remain_len > 0) {
    size_t recv_len = remain_len;
    while ((result = doca_comm_channel_ep_recvfrom(
                ep, msg, &recv_len, DOCA_CC_MSG_FLAG_NONE, &peer_addr)) ==
           DOCA_ERROR_AGAIN) {
      // DOCA_LOG_DBG("recv_len: %ld", recv_len);
      // recv_len = remain_len;
    }
    if (result != DOCA_SUCCESS) {
      return result;
    }
    remain_len -= recv_len;
    msg = msg + recv_len;
  }
  return result;
}

void generate_random_data(uint8_t *buf, size_t len) {
  for (int i = 0; i < len - 1; ++i) {
    buf[i] = all_char[rand() % ALL_CHAR_LEN];
  }
  buf[len - 1] = 0;
}
