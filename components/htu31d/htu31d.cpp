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

#include "htu31d.h"

#include "esphome/core/log.h"

namespace esphome {
namespace htu31d {

static const char *const TAG = "htu31d";

static const uint8_t MGOS_HTU31D_DEFAULT_I2CADDR = (0x40);

static const uint8_t MGOS_HTU31D_READTEMPHUM = (0x00);
static const uint8_t MGOS_HTU31D_CONVERSION = (0x40);
static const uint8_t MGOS_HTU31D_HEATERON = (0x04);
static const uint8_t MGOS_HTU31D_HEATEROFF = (0x02);
static const uint8_t MGOS_HTU31D_READREG = (0x0A);
static const uint8_t MGOS_HTU31D_RESET = (0x1E);

void HTU31DComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up HTU31D...");
  parent_->dump_config();

  if (!write_bytes(MGOS_HTU31D_RESET, nullptr, 0)) {
    this->mark_failed();
    return;
  }

  // Wait for software reset to complete
  delay(15);
  // Get version
  ESP_LOGCONFIG(TAG, "Get version - write_bytes_raw");
  if (!write_bytes_raw(&MGOS_HTU31D_READREG, 1)) {
    this->status_set_warning();
    return;
  }
  ESP_LOGCONFIG(TAG, "Get version - read_bytes_raw");
  uint8_t version[4]{0};
  if (!read_bytes_raw(version, 4)) {
    this->status_set_warning();
  }
  if (version[3] == crc8(version, 3)) {
    ESP_LOGI(TAG,
             "HTU31D serial number 0x%02hhx%02hhx%02hhx created at I2C 0x%02x",
             version[0], version[1], version[2], this->address_);
    return;
  } else {
    ESP_LOGE(TAG, "CRC error on version data");
  }
}

void HTU31DComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "HTU31D:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with HTU31D failed!");
  }
  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "Temperature", this->temperature_);
  LOG_SENSOR("  ", "Humidity", this->humidity_);
}

/// Update the sensor values (temperature+humidity).
void HTU31DComponent::update() {
  /* start conversion */
  if (!write_bytes(MGOS_HTU31D_CONVERSION, nullptr, 0)) {
    ESP_LOGE(TAG, "Failed to start conversion");
    this->status_set_warning();
    return;
  }
  delay(20);
  uint8_t data[6]{0};
  if (!read_bytes(MGOS_HTU31D_READTEMPHUM, data, 6, 20)) {
    ESP_LOGE(TAG, "Failed to read 6 bytes of data");
    this->status_set_warning();
    return;
  }

  uint8_t tmp[3]{data[0], data[1], data[2]};
  if (tmp[2] != crc8(data, 2)) {
    ESP_LOGE(TAG, "CRC error on temperature data");
    return;
  }
  uint16_t temp = (tmp[0] << 8) + tmp[1];
  float temperature = temp;

  temperature /= 65535.0;
  temperature *= 165;
  temperature -= 40;

  uint8_t hum[3]{data[3], data[4], data[5]};
  if (hum[2] != crc8(hum, 2)) {
    ESP_LOGE(TAG, ("CRC error on temperature data"));
    return;
  }

  uint16_t hmdty = (hum[0] << 8) + hum[1];
  float humidity = hmdty;
  humidity /= 65535.0;
  humidity *= 100;

  if (this->temperature_ != nullptr) {
    this->temperature_->publish_state(temperature);
  }
  if (this->humidity_ != nullptr) {
    this->humidity_->publish_state(humidity);
  }
  this->status_clear_warning();
}

float HTU31DComponent::get_setup_priority() const {
  return setup_priority::DATA;
}

}  // namespace htu31d
}  // namespace esphome
