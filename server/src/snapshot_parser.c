#include <cjson/cJSON.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "sensor.h"
#include "snapshot_parser.h"

static void log_error(const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  fprintf(stderr, "parse_snapshot_json: ");
  fprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
}

static int parse_sensor_type(const cJSON *item, const char *name,
                             sensor_type_t *out_type)
{
  const cJSON *t = cJSON_GetObjectItemCaseSensitive(item, "t");
  if (!cJSON_IsNumber(t)) {
    log_error("reading '%s' missing or invalid 't' field, skipping", name);
    return -1;
  }

  int type_val = t->valueint;
  if (type_val < SENSOR_TYPE_FIRST || type_val >= SENSOR_TYPE_LAST) {
    log_error("reading '%s' has unknown sensor type %d, skipping", name,
              type_val);
    return -1;
  }

  *out_type = (sensor_type_t)type_val;
  return 0;
}

static int parse_reading(const cJSON *item, parsed_reading_t *out)
{
  const cJSON *n = cJSON_GetObjectItemCaseSensitive(item, "n");
  if (!cJSON_IsString(n) || n->valuestring == NULL) {
    log_error("reading missing 'n' field, skipping");
    return -1;
  }

  const cJSON *v = cJSON_GetObjectItemCaseSensitive(item, "v");
  if (!cJSON_IsNumber(v) && !cJSON_IsString(v)) {
    log_error("reading '%s' missing 'v' field, skipping", n->valuestring);
    return -1;
  }

  sensor_type_t type;
  if (parse_sensor_type(item, n->valuestring, &type) != 0) {
    return -1;
  }

  strncpy(out->name, n->valuestring, SENSOR_NAME_MAX_LEN - 1);
  out->name[SENSOR_NAME_MAX_LEN - 1] = '\0';
  out->type                          = type;

  switch (type) {
  case SENSOR_TYPE_FLOAT:
    out->value.f = (float)v->valuedouble;
    break;
  case SENSOR_TYPE_INT:
    out->value.i = v->valueint;
    break;
  case SENSOR_TYPE_STRING:
    strncpy(out->value.s, v->valuestring, SENSOR_STRING_MAX_LEN - 1);
    out->value.s[SENSOR_STRING_MAX_LEN - 1] = '\0';
    break;
  default:
    break;
  }

  return 0;
}

static int parse_readings_array(const cJSON *readings, parsed_snapshot_t *out)
{
  const cJSON *item = NULL;
  cJSON_ArrayForEach(item, readings)
  {
    if (out->count >= SENSOR_MAX_CHANNELS) {
      log_error("too many readings, truncating");
      break;
    }

    if (parse_reading(item, &out->readings[out->count]) == 0) {
      out->count++;
    }
  }

  return 0;
}

int parse_snapshot_json(const char *buf, size_t len, parsed_snapshot_t *out)
{
  if (!buf || len == 0 || !out) {
    return -1;
  }

  memset(out, 0, sizeof(*out));

  cJSON *root = cJSON_ParseWithLength(buf, len);
  if (!root) {
    const char *err = cJSON_GetErrorPtr();
    log_error("JSON parse failed near: %s", err ? err : "unknown");
    return -1;
  }

  const cJSON *ts = cJSON_GetObjectItemCaseSensitive(root, "ts");
  if (!cJSON_IsNumber(ts)) {
    log_error("missing or invalid 'ts' field");
    cJSON_Delete(root);
    return -1;
  }
  out->timestamp_ms = (int64_t)ts->valuedouble;

  const cJSON *readings = cJSON_GetObjectItemCaseSensitive(root, "readings");
  if (!cJSON_IsArray(readings)) {
    log_error("missing or invalid 'readings' array");
    cJSON_Delete(root);
    return -1;
  }

  parse_readings_array(readings, out);

  cJSON_Delete(root);
  return 0;
}
