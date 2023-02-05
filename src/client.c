#include "include/client.h"
#include "include/params.h"
#include <doca_comm_channel.h>
#include <doca_log.h>
#include <stdlib.h>
#include <unistd.h>
#define MAX_MSG_SIZE 4080
#define CC_MAX_QUEUE_SIZE 10

DOCA_LOG_REGISTER(client.c);
static doca_error_t
ping_pong_benchmark(struct doca_comm_channel_ep_t *ep,
                    struct doca_comm_channel_addr_t *peer_addr,
                    struct Config *config);

/// @brief The client will connect to the server and do ping-pong benchmark.
/// @param dev_pcie [in]
/// @param rep_pcie [in]
/// @param config [in]
/// @return
doca_error_t client_benchmark(struct doca_pci_bdf *dev_pcie,
                              struct doca_pci_bdf *rep_pcie,
                              struct Config *config) {

  doca_error_t result = DOCA_SUCCESS;
  // create ep
  struct doca_comm_channel_ep_t *ep = NULL;
  result = doca_comm_channel_ep_create(&ep);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to create Comm Channel client endpoint: %s",
                 doca_get_error_string(result));
    goto fail_create_ep;
  }
  // open DOCA device
  struct doca_dev *cc_dev = NULL;
  result = open_doca_device_with_pci(dev_pcie, NULL, &cc_dev);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR(
        "Failed to open Comm Channel DOCA device based on PCI address");
    goto fail_open_device;
  }
  // Set all endpoint properties
  result = doca_comm_channel_ep_set_device(ep, cc_dev);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to set device property");
    goto fail_connect;
  }

  result = doca_comm_channel_ep_set_max_msg_size(ep, MAX_MSG_SIZE);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to set max_msg_size property");
    goto fail_connect;
  }
  uint16_t max_msg_size = 0;
  result = doca_comm_channel_ep_get_max_msg_size(ep, &max_msg_size);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to get max_msg_size property");
    goto fail_connect;
  }
  DOCA_LOG_DBG("max_msg_size is: %d", max_msg_size);
  result = doca_comm_channel_ep_set_send_queue_size(ep, CC_MAX_QUEUE_SIZE);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to set snd_queue_size property");
    goto fail_connect;
  }

  result = doca_comm_channel_ep_set_recv_queue_size(ep, CC_MAX_QUEUE_SIZE);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to set rcv_queue_size property");
    goto fail_connect;
  }

  // Connect to server node
  struct doca_comm_channel_addr_t *peer_addr = NULL;
  result = doca_comm_channel_ep_connect(ep, config->server_name, &peer_addr);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Couldn't establish a connection with the server: %s",
                 doca_get_error_string(result));
    goto fail_connect;
  }
  // Make sure peer address is valid
  while ((result = doca_comm_channel_peer_addr_update_info(peer_addr)) ==
         DOCA_ERROR_CONNECTION_INPROGRESS) {
    usleep(1);
  }
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to validate the connection with the DPU: %s",
                 doca_get_error_string(result));
    goto fail_connect;
  }
  // connection successfully.
  DOCA_LOG_INFO("Connection to server was established successfully");

  // ping pong benchmark
  result = ping_pong_benchmark(ep, peer_addr, config);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("ping pong benchmark fail: %s", doca_get_error_string(result));
    goto fail_ping_pong;
  }
fail_ping_pong:
  doca_comm_channel_ep_disconnect(ep, peer_addr);
fail_connect:
  doca_dev_close(cc_dev);
fail_open_device:
  doca_comm_channel_ep_destroy(ep);
fail_create_ep:
  return result;
}

/// @brief ping pong.
/// @param ep [in]
/// @param peer_addr [in]
/// @param config [in]
/// @return
static doca_error_t
ping_pong_benchmark(struct doca_comm_channel_ep_t *ep,
                    struct doca_comm_channel_addr_t *peer_addr,
                    struct Config *config) {
  doca_error_t result = DOCA_SUCCESS;
  int size = config->byte_size;
  int total_iter = config->warm_up + config->iterations;
  // alloc buffer
  uint8_t *data = (uint8_t *)malloc(size);
  if (data == NULL) {
    result = DOCA_ERROR_NO_MEMORY;
    goto fail_alloc_buf;
  }
  *((int *)data) = size;
  *((int *)(data + sizeof(int))) = config->warm_up;
  *((int *)(data + sizeof(int) * 2)) = config->iterations;
  int first_msg_len = sizeof(int) * 3;
  // send first msg
  result = send_msg(ep, peer_addr, data, first_msg_len);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Message was not sent: %s", doca_get_error_string(result));
    goto fail_transfer_msg;
  }
  // ping-pong
  for (int i = 0; i < total_iter; ++i) {
    // recv a msg
    result = recv_msg(ep, peer_addr, data, size);
    if (result != DOCA_SUCCESS) {
      DOCA_LOG_ERR("Recv message error: %s", doca_get_error_string(result));
      goto fail_transfer_msg;
    }
    // send a msg
    result = send_msg(ep, peer_addr, data, size);
    if (result != DOCA_SUCCESS) {
      DOCA_LOG_ERR("Send message error: %s", doca_get_error_string(result));
      goto fail_transfer_msg;
    }
  }
fail_transfer_msg:
  free(data);

fail_alloc_buf:
  return result;
}