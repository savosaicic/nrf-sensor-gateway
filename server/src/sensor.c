#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sensor.h"

static sensor_registry_t g_registry = {0};

/**
 * @brief Initialize the sensor registry
 * @return Pointer to the initialized registry, or NULL on failure
 */
sensor_registry_t *sensor_reg_init(void)
{
  g_registry.count = 0;
  return &g_registry;
}

/**
 * @brief Close the sensor registry and reset its state
 * @param reg Pointer to the registry to close
 */
void sensor_reg_close(sensor_registry_t *reg)
{
  reg->count = 0;
}

sensor_registry_t *sensor_reg_get()
{
  return &g_registry;
}

/**
 * @brief Register a new sensor channel in the registry
 *
 * @param reg  Pointer to the sensor registry
 * @param name Unique name for the channel
 * @param type Sensor type to assign to the channel
 *
 * @return Pointer to the registered or existing channel, or NULL if the name
 *         is empty or the registry is full
 */
sensor_channel_t *sensor_channel_register(sensor_registry_t *reg,
                                          const char *name, sensor_type_t type)
{
  sensor_channel_t *ch;

  if (!name || strlen(name) == 0) {
    fprintf(stderr, "Channel name must not be empty\n");
    return NULL;
  }

  for (size_t i = 0; i < reg->count; i++) {
    if (strcmp(reg->channels[i].name, name) == 0) {
      return &reg->channels[i];
    }
  }

  if (reg->count >= SENSOR_MAX_CHANNELS) {
    fprintf(stderr, "Channel registry is full (max %i)", SENSOR_MAX_CHANNELS);
    return NULL;
  }

  ch = &reg->channels[reg->count++];
  memset(ch, 0, sizeof(*ch));
  strncpy(ch->name, name, SENSOR_NAME_MAX_LEN - 1);
  ch->type      = type;
  ch->has_value = false;
  return ch;
}

/**
 * @brief Update the float value of a sensor channel
 *
 * @param ch    Pointer to the sensor channel
 * @param value Float value to set
 *
 * @return 0 on success, -1 on failure
 */
int sensor_channel_update_float(sensor_channel_t *ch, float value)
{
  if (!ch || ch->type != SENSOR_TYPE_FLOAT) {
    return -1;
  }
  ch->value.f   = value;
  ch->has_value = true;
  return 0;
}

/**
 * @brief Update the int value of a sensor channel
 *
 * @param ch    Pointer to the sensor channel
 * @param value Integer value to set
 *
 * @return 0 on success, -1 on failure
 */
int sensor_channel_update_int(sensor_channel_t *ch, int value)
{
  if (!ch || ch->type != SENSOR_TYPE_INT) {
    return -1;
  }
  ch->value.i   = value;
  ch->has_value = true;
  return 0;
}

/**
 * @brief Update the string value of a sensor channel
 *
 * @param ch    Pointer to the sensor channel
 * @param value A pointer to a string to set
 *
 * @return 0 on success, -1 on failure
 */
int sensor_channel_update_string(sensor_channel_t *ch, const char *value)
{
  if (!ch || ch->type != SENSOR_TYPE_STRING || !value) {
    return -1;
  }
  strncpy(ch->value.s, value, SENSOR_STRING_MAX_LEN - 1);
  ch->value.s[SENSOR_STRING_MAX_LEN - 1] = '\0';
  ch->has_value = true;
  return 0;
}
