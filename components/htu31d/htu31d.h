/*
 * Copyright (c) 2021 Liviu Nicolescu <nliviu@gmail.com>
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

namespace esphome {
namespace htu31d {

class HTU31DComponent : public PollingComponent, public i2c::I2CDevice {
 public:
  void set_temperature(sensor::Sensor *temperature) {
    temperature_ = temperature;
  }
  void set_humidity(sensor::Sensor *humidity) {
    humidity_ = humidity;
  }

  /// Setup (reset) the sensor and check connection.
  void setup() override;
  void dump_config() override;
  /// Update the sensor values (temperature+humidity).
  void update() override;

  float get_setup_priority() const override;

 protected:
  sensor::Sensor *temperature_{nullptr};
  sensor::Sensor *humidity_{nullptr};
};

}  // namespace htu31d
}  // namespace esphome
