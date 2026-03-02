#include <coap3/coap.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "coap_server.h"
#include "sensor.h"
#include "snapshot_parser.h"
#include "db.h"

static coap_context_t *g_ctx = NULL;

static void handle_snapshot_post(coap_resource_t     *resource,
                                 coap_session_t      *session,
                                 const coap_pdu_t    *request,
                                 const coap_string_t *query,
                                 coap_pdu_t          *response)
{
  size_t         len    = 0;
  size_t         offset = 0;
  size_t         total  = 0;
  const uint8_t *data   = NULL;

  (void)resource;
  (void)session;
  (void)query;

  /* COAP_BLOCK_SINGLE_BODY is set so this is always the complete body */
  if (!coap_get_data_large(request, &len, &data, &offset, &total)) {
    fprintf(stderr, "handle_snapshot_post: failed to get payload\n");
    coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
    return;
  }

  if (len == 0 || data == NULL) {
    fprintf(stderr, "handle_snapshot_post: empty payload\n");
    coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
    return;
  }

  fprintf(stdout, "Received snapshot: %.*s\n", (unsigned int)len, data);

  /* Parse the snapshot */
  parsed_snapshot_t snap;
  if (parse_snapshot_json((const char *)data, len, &snap) != 0) {
    fprintf(stderr, "handle_snapshot_post: JSON parse failed\n");
    coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
    return;
  }

  /* Insert each reading into the DB */
  for (size_t i = 0; i < snap.count; i++) {
    parsed_reading_t *r = &snap.readings[i];

    sensor_registry_t *reg = sensor_reg_get();

    sensor_channel_t *ch = sensor_channel_register(reg, r->name, r->type);
    if (!ch) {
      fprintf(stderr, "handle_snapshot_post: failed to register channel '%s'\n",
              r->name);
      continue;
    }

    switch (r->type) {
    case SENSOR_TYPE_FLOAT:
      sensor_channel_update_float(ch, r->value.f);
      break;
    case SENSOR_TYPE_INT:
      sensor_channel_update_int(ch, r->value.i);
      break;
    case SENSOR_TYPE_LAST:
      break;
    }

    if (db_insert_reading(ch, 0) != 0) {
      fprintf(stderr, "handle_snapshot_post: db insert failed for '%s'\n",
              r->name);
      /* don't abort: best effort for remaining channels */
    }
  }

  coap_pdu_set_code(response, COAP_RESPONSE_CODE_CHANGED);
}

static void init_resources(coap_context_t *ctx)
{
  coap_resource_t *r;

  r = coap_resource_init(coap_make_str_const("sensor/snapshot"), 0);
  if (!r) {
    return;
  }

  coap_register_request_handler(r, COAP_REQUEST_POST, handle_snapshot_post);

  /* Advertise this resource in /.well-known/core */
  coap_add_attr(r, coap_make_str_const("ct"), coap_make_str_const("50"),
                0); /* 50 = application/json */
  coap_add_attr(r, coap_make_str_const("title"),
                coap_make_str_const("\"Sensor Snapshot\""), 0);

  coap_add_resource(ctx, r);
}

/**
 * @brief Initialize the CoAP server and start listening on the given port
 *
 * @param port UDP port to listen on
 *
 * @return 0 on success, -1 on error
 */
int coap_server_init(uint16_t port)
{
  coap_endpoint_t *endpoint;
  coap_address_t   listen_addr;

  coap_startup();

  g_ctx = coap_new_context(NULL);
  if (!g_ctx) {
    fprintf(stderr, "Failed to create CoAP context\n");
    return -1;
  }

  coap_address_init(&listen_addr);
  listen_addr.addr.sa.sa_family        = AF_INET;
  listen_addr.addr.sin.sin_addr.s_addr = INADDR_ANY;
  listen_addr.addr.sin.sin_port        = htons(port);

  endpoint = coap_new_endpoint(g_ctx, &listen_addr, COAP_PROTO_UDP);
  if (!endpoint) {
    fprintf(stderr, "Failed to create CoAP endpoint on port %d\n", port);
    coap_free_context(g_ctx);
    g_ctx = NULL;
    return -1;
  }

  init_resources(g_ctx);

  fprintf(stdout, "CoAP server listening on port %d\n", port);
  return 0;
}

/**
 * @brief Free the CoAP context and shut down the CoAP stack
 */
void coap_server_cleanup(void)
{
  if (g_ctx) {
    coap_free_context(g_ctx);
    g_ctx = NULL;
  }
  coap_cleanup();
}

/**
 * @brief Run the CoAP server I/O loop until *stop is set to true
 *
 * @param stop Pointer to a flag that signals the loop to exit when set to true
 */
void coap_server_loop(volatile bool *stop)
{
  int result;

  while (!*stop) {
    result = coap_io_process(g_ctx, COAP_SERVER_TIMEOUT_MS);

    if (result < 0) {
      fprintf(stderr, "coap_io_process error: %d\n", result);
      break;
    }
  }
}
