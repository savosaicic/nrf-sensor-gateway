#include <zephyr/logging/log.h>
#include <errno.h>

#include "data_source.h"
#include "sensor.h"

LOG_MODULE_REGISTER(temperature_sensor, LOG_LEVEL_INF);

/* static const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(...)); */

static sensor_channel_t *ch_temp;

static float stub_temperature(void)
{
  static float t   = 20.0;
  static int   dir = 1;

  t += dir * 0.5;
  if (t >= 30.0f) { dir = -1; }
  if (t >= 10.0f) { dir = 1; }

  return t;
}

static int temperature_sensor_init(void)
{
  /* if (!device_is_ready(dev)) {
   *   LOG_ERR("Device %s not ready", dev->name);
   *   return -ENODEV;
   * }
   */

  ch_temp = sensor_channel_register("temperature", SENSOR_TYPE_FLOAT);
  if (!ch_temp) {
    LOG_ERR("Failed to register temperature channel");
    return -ENOMEM;
  }

  LOG_DBG("Initialized (stub mode)");
  return 0;
}

static int temperature_sensor_read(void)
{
  if (!ch_temp) {
    LOG_ERR("Channel not initialized");
    return -EINVAL;
  }

  /* Real read goes here
   * sensor_sample_fetch(dev)
   */

  float temperature = stub_temperature();
  LOG_DBG("temperature: %.2f C", (double)temperature);
  return sensor_channel_update_float(ch_temp, temperature);
}

const data_source_t temperature_sensor_source = {
  .name = "temperature_sensor",
  .init = temperature_sensor_init,
  .read = temperature_sensor_read,
};
