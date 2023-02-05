#include "include/server.h"
#include "include/params.h"
#include "include/common.h"
#include <doca_comm_channel.h>
#include <doca_dev.h>
#include <doca_log.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#define MAX_MSG_SIZE 4080
#define CC_MAX_QUEUE_SIZE 10

DOCA_LOG_REGISTER(server.c);
static doca_error_t
ping_pong_benchmark(struct doca_comm_channel_ep_t *ep,
                    struct doca_comm_channel_addr_t *peer_addr,
                    struct Config *config);
doca_error_t server_benchmark(struct doca_pci_bdf *dev_pcie,
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
  // open DOCA device representor
  struct doca_dev_rep *cc_dev_rep = NULL;
  result = open_doca_device_rep_with_pci(cc_dev, DOCA_DEV_REP_FILTER_NET,
                                         rep_pcie, &cc_dev_rep);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to open Comm Channel DOCA device representor based on "
                 "PCI address");
    goto fail_open_dev_rep;
  }
  // Set all endpoint properties
  result = doca_comm_channel_ep_set_device(ep, cc_dev);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to set device property");
    goto fail_listen;
  }

  result = doca_comm_channel_ep_set_max_msg_size(ep, MAX_MSG_SIZE);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to set max_msg_size property");
    goto fail_listen;
  }

  result = doca_comm_channel_ep_set_send_queue_size(ep, CC_MAX_QUEUE_SIZE);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to set snd_queue_size property");
    goto fail_listen;
  }

  result = doca_comm_channel_ep_set_recv_queue_size(ep, CC_MAX_QUEUE_SIZE);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to set rcv_queue_size property");
    goto fail_listen;
  }
  result = doca_comm_channel_ep_set_device_rep(ep, cc_dev_rep);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to set DOCA device representor property");
    goto fail_listen;
  }
  // start listen for new connections
  result = doca_comm_channel_ep_listen(ep, config->server_name);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Comm Channel server couldn't start listening: %s",
                 doca_get_error_string(result));
    goto fail_listen;
  }
  DOCA_LOG_INFO("Server started Listening, waiting for new connections");

  // connected
  uint8_t buf[1024];
  size_t first_len = sizeof(int) * 3;
  struct doca_comm_channel_addr_t *peer_addr;
  while ((result = doca_comm_channel_ep_recvfrom(
              ep, (void *)buf, &first_len, DOCA_CC_MSG_FLAG_NONE,
              &peer_addr)) == DOCA_ERROR_AGAIN) {
    first_len = sizeof(int) * 3;
    ;
  }
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Message was not received: %s", doca_get_error_string(result));
    goto fail_listen;
  }
  config->byte_size = *((int *)buf);
  config->warm_up = *((int *)(buf + sizeof(int)));
  config->iterations = *((int *)(buf + sizeof(int) * 2));
  DOCA_LOG_INFO("size: %d, warm_up: %d, iterations: %d", config->byte_size,
                config->warm_up, config->iterations);
  // ping pong benchmark
  result = ping_pong_benchmark(ep, peer_addr, config);
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("ping pong benchmark fail: %s", doca_get_error_string(result));
    goto fail_ping_pong;
  }

fail_ping_pong:
  doca_comm_channel_ep_disconnect(ep, peer_addr);
fail_listen:
  doca_dev_rep_close(cc_dev_rep);
fail_open_dev_rep:
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
  size_t size = config->byte_size;
  // alloc buffer
  uint8_t *data = (uint8_t *)malloc(size);
  if (data == NULL) {
    result = DOCA_ERROR_NO_MEMORY;
    goto fail_alloc_buf;
  }
  generate_random_data(data, size);
  // ping-pong
  for (int i = 0; i < config->warm_up; ++i) {
    // generate_random_data(data, size);
    // send a msg
    result = send_msg(ep, peer_addr, data, size);
    if (result != DOCA_SUCCESS) {
      DOCA_LOG_ERR("Send message error: %s", doca_get_error_string(result));
      goto fail_transfer_msg;
    }
    // DOCA_LOG_DBG("Send msg: %s", data);
    // recv a msg
    result = recv_msg(ep, peer_addr, data, size);
    if (result != DOCA_SUCCESS) {
      DOCA_LOG_ERR("Recv message error: %s", doca_get_error_string(result));
      goto fail_transfer_msg;
    }
    // DOCA_LOG_DBG("Recv msg: %s", data);
  }
  double avg_duration = 0;
  for (int i = 0; i < config->iterations; ++i) {
    // send a msg
    struct timeval stv, etv;
    gettimeofday(&stv, NULL);
    result = send_msg(ep, peer_addr, data, size);
    if (result != DOCA_SUCCESS) {
      DOCA_LOG_ERR("Send message error: %s", doca_get_error_string(result));
      goto fail_transfer_msg;
    }
    // recv a msg
    result = recv_msg(ep, peer_addr, data, size);
    if (result != DOCA_SUCCESS) {
      DOCA_LOG_ERR("Recv message error: %s", doca_get_error_string(result));
      goto fail_transfer_msg;
    }
    gettimeofday(&etv, NULL);
    long duration =
        (etv.tv_sec - stv.tv_sec) * 1000000 + etv.tv_usec - stv.tv_usec;
    avg_duration += duration;
    DOCA_LOG_INFO("duration %ldus", duration);
  }
  avg_duration /= config->iterations;
  DOCA_LOG_INFO("average duration %lf us", avg_duration);
fail_transfer_msg:
  free(data);

fail_alloc_buf:
  return result;
}