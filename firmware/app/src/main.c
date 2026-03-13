#include <zephyr/logging/log.h>
#include <stdio.h>
#include <cJSON.h>

#include "modem.h"
#include "network_events.h"
#include "sensor.h"
#include "coap_backend.h"
#include "sensor_reader.h"

#define JSON_BUF_SIZE 1024

LOG_MODULE_REGISTER(nrf_sensor_gateway, LOG_LEVEL_DBG);

K_EVENT_DEFINE(network_events);

static const coap_backend_t *coap = &coap_backend_libcoap;

static int snapshot_to_json(const sensor_snapshot_t *snapshot, char *buf,
                            size_t buf_len)
{
  int ret = -ENOMEM;

  cJSON *root = cJSON_CreateObject();
  if (!root) {
    return -ENOMEM;
  }

  cJSON_AddNumberToObject(root, "ts", (double)snapshot->timestamp_ms);

  cJSON *readings = cJSON_AddArrayToObject(root, "readings");
  if (!readings) {
    goto cleanup;
  }

  for (size_t i = 0; i < snapshot->count; i++) {
    const sensor_reading_t *r = &snapshot->readings[i];

    cJSON *entry = cJSON_CreateObject();
    if (!entry) {
      goto cleanup;
    }

    cJSON_AddStringToObject(entry, "n", r->name);
    cJSON_AddNumberToObject(entry, "t", r->type);

    switch (r->type) {
    case SENSOR_TYPE_FLOAT:
      cJSON_AddNumberToObject(entry, "v", (double)r->value.f);
      break;
    case SENSOR_TYPE_INT:
      cJSON_AddNumberToObject(entry, "v", r->value.i);
      break;
    default:
      cJSON_Delete(entry);
      ret = -EINVAL;
      goto cleanup;
    }

    cJSON_AddItemToArray(readings, entry);
  }

  if (!cJSON_PrintPreallocated(root, buf, (int)buf_len, false)) {
    goto cleanup;
  }

  ret = (int)strlen(buf);

cleanup:
  cJSON_Delete(root);
  return ret;
}

int main(void)
{
  int               err;
  sensor_snapshot_t snapshot;
  char              json_buf[JSON_BUF_SIZE];

  /* Connect to lte-m (blocking function) */
  err = modem_configure();
  if (err) {
    LOG_ERR("Modem configuration failed: %d", err);
    return err;
  }

  /* Post an event to threads waiting for network to be configured */
  k_event_post(&network_events, NET_EVENT_LTE_CONNECTED);

  if (coap->init() != 0) {
    LOG_ERR("CoAP backend init failed — thread exiting");
    return -1;
  }

  while (1) {
    k_msgq_get(&sensor_msgq, &snapshot, K_FOREVER);
    int len = snapshot_to_json(&snapshot, json_buf, sizeof(json_buf));
    if (len < 0) {
      LOG_ERR("JSON encoding failed (%d) — dropping snapshot", len);
      continue;
    }

    LOG_DBG("Sending snapshot: %zu readings, %d bytes", snapshot.count, len);
    LOG_DBG("%s", json_buf);

    err = coap->send((const uint8_t *)json_buf, (size_t)len);
    if (err) {
      LOG_ERR("CoAP send failed (%d) — dropping snapshot", err);
      continue;
    }

    err = coap->recv();
    if (err == -ETIMEDOUT) {
      LOG_WRN("CoAP ACK timeout — snapshot may be lost");
    } else if (err) {
      LOG_ERR("CoAP recv error (%d)", err);
    }
  }

  return 0;
}
