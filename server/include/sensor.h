#ifndef SENSOR_H
#define SENSOR_H

#include <stdbool.h>
#include <stddef.h>

#define SENSOR_NAME_MAX_LEN 64
#define SENSOR_MAX_CHANNELS 16

typedef enum {
  SENSOR_TYPE_FIRST = 0,
  SENSOR_TYPE_FLOAT = SENSOR_TYPE_FIRST,
  SENSOR_TYPE_INT,
  SENSOR_TYPE_LAST
} sensor_type_t;

typedef union {
  float f;
  int   i;
} sensor_value_t;

typedef struct {
  char           name[SENSOR_NAME_MAX_LEN];
  sensor_type_t  type;
  sensor_value_t value;
  bool           has_value;

} sensor_channel_t;

typedef struct {
  sensor_channel_t channels[SENSOR_MAX_CHANNELS];
  size_t           count;
} sensor_registry_t;

sensor_registry_t *sensor_reg_init(void);
void               sensor_reg_close(sensor_registry_t *reg);
sensor_registry_t *sensor_reg_get();

sensor_channel_t *sensor_channel_register(sensor_registry_t *reg,
                                          const char *name, sensor_type_t type);
int               sensor_channel_update_float(sensor_channel_t *ch, float value);
int               sensor_channel_update_int(sensor_channel_t *ch, int value);

#endif /* SENSOR_H */
