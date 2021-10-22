#include "st7789.h"

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#include "Esp.h"

namespace esphome {
namespace st7789 {

static const char *const TAG = "st7789";

void ST7789::setup() {
  ESP_LOGCONFIG(TAG, "Setting up SPI ST7789...");
  this->spi_setup();
  this->dc_pin_->setup();  // OUTPUT

  this->init_reset_();

  this->write_command_(ST7789_SLPOUT);  // Sleep out
  delay(120);                           // NOLINT

  this->write_command_(ST7789_NORON);  // Normal display mode on

  // *** display and color format setting ***
  this->write_command_(ST7789_MADCTL);
  this->write_data_(ST7789_MADCTL_COLOR_ORDER);

  // JLX240 display datasheet
  this->write_command_(0xB6);
  this->write_data_(0x0A);
  this->write_data_(0x82);

  /* https://github.com/Bodmer/TFT_eSPI/blob/9e64092f58bc4b2458d51ad5bcb0b2e7fdcb6507/TFT_Drivers/ST7789_Init.h#L24
  writecommand(ST7789_RAMCTRL);
  writedata(0x00);
  writedata(0xE0); // 5 to 6 bit conversion: r0 = r5, b0 = b5
   */
  write_command_(ST7789_RAMCTRL);
  write_data_(0x00);
  write_data_(0xE0);  // 5 to 6 bit conversion: r0 = r5, b0 = b5

  this->write_command_(ST7789_COLMOD);
  this->write_data_(0x55);
  delay(10);

  // *** ST7789 Frame rate setting ***
  this->write_command_(ST7789_PORCTRL);
  this->write_data_(0x0c);
  this->write_data_(0x0c);
  this->write_data_(0x00);
  this->write_data_(0x33);
  this->write_data_(0x33);

  this->write_command_(ST7789_GCTRL);  // Voltages: VGH / VGL
  this->write_data_(0x35);

  // *** ST7789 Power setting ***
  this->write_command_(ST7789_VCOMS);
  this->write_data_(0x28);  // JLX240 display datasheet

  this->write_command_(ST7789_LCMCTRL);
  this->write_data_(0x0C);

  this->write_command_(ST7789_VDVVRHEN);
  this->write_data_(0x01);
  this->write_data_(0xFF);

  this->write_command_(ST7789_VRHS);  // voltage VRHS
  this->write_data_(0x10);

  this->write_command_(ST7789_VDVS);
  this->write_data_(0x20);

  this->write_command_(ST7789_FRCTRL2);
  this->write_data_(0x0f);

  this->write_command_(ST7789_PWCTRL1);
  this->write_data_(0xa4);
  this->write_data_(0xa1);

  // *** ST7789 gamma setting ***
  this->write_command_(ST7789_PVGAMCTRL);
  this->write_data_(0xd0);
  this->write_data_(0x00);
  this->write_data_(0x02);
  this->write_data_(0x07);
  this->write_data_(0x0a);
  this->write_data_(0x28);
  this->write_data_(0x32);
  this->write_data_(0x44);
  this->write_data_(0x42);
  this->write_data_(0x06);
  this->write_data_(0x0e);
  this->write_data_(0x12);
  this->write_data_(0x14);
  this->write_data_(0x17);

  this->write_command_(ST7789_NVGAMCTRL);
  this->write_data_(0xd0);
  this->write_data_(0x00);
  this->write_data_(0x02);
  this->write_data_(0x07);
  this->write_data_(0x0a);
  this->write_data_(0x28);
  this->write_data_(0x31);
  this->write_data_(0x54);
  this->write_data_(0x47);
  this->write_data_(0x0e);
  this->write_data_(0x1c);
  this->write_data_(0x17);
  this->write_data_(0x1b);
  this->write_data_(0x1e);

  this->write_command_(ST7789_INVON);
  write_command_(ST7789_CASET);  // Column address set
  write_data_(0x00);
  write_data_(0x00);
  write_data_(0x00);
  write_data_(0xEF);  // 239

  write_command_(ST7789_RASET);  // Row address set
  write_data_(0x00);
  write_data_(0x00);
  write_data_(0x01);
  write_data_(0x3F);  // 319

  // Clear display - ensures we do not see garbage at power-on
  this->draw_filled_rect_(0, 0, 239, 319, 0x0000);

  delay(120);  // NOLINT

  this->write_command_(ST7789_DISPON);  // Display on
  delay(120);                           // NOLINT

  backlight_(true);

  uint32_t maxAllocHeap = ESP.getMaxAllocHeap();
#if 0
  uint32_t heapSize = ESP.getHeapSize();  // total heap size
  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t minFreeHeap = ESP.getMinFreeHeap();
  ESP_LOGI(TAG,
           " heap - size: %" PRIu32 ", free: %" PRIu32 ", min: %" PRIu32
           ", maxAlloc: %" PRIu32 "",
           heapSize, freeHeap, minFreeHeap, maxAllocHeap);

  // SPI RAM
  uint32_t psramSize = ESP.getPsramSize();
  uint32_t freePsram = ESP.getFreePsram();
  uint32_t minFreePsram = ESP.getMinFreePsram();
  uint32_t maxAllocPsram = ESP.getMaxAllocPsram();
  ESP_LOGI(TAG,
           "psram - size: %" PRIu32 ", free: %" PRIu32 ", min: %" PRIu32
           ", maxAlloc: %" PRIu32 "",
           psramSize, freePsram, minFreePsram, maxAllocPsram);
#endif

  size_t bufferLength = this->get_buffer_length_();
  if ((maxAllocHeap < bufferLength) && psramFound()) {
    this->buffer_ = static_cast<uint8_t *>(
        heap_caps_calloc(bufferLength, 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (this->buffer_ == nullptr) {
      ESP_LOGE(TAG, "PSRAM - Could not allocate buffer for display!");
      mark_failed();
      return;
    }
    ESP_LOGI(TAG, "Display buffer allocated in PSRAM");
  } else {
    buffer_ = new uint8_t[bufferLength];
    if (this->buffer_ == nullptr) {
      ESP_LOGE(TAG, "RAM - Could not allocate buffer for display!");
      mark_failed();
      return;
    }
    ESP_LOGI(TAG, "Display buffer allocated in RAM");
  }
  this->clear();
  ESP_LOGI(TAG, "bufferLength: %zu, buffer_: %p", bufferLength,
           static_cast<void *>(buffer_));

  memset(this->buffer_, 0x00, bufferLength);
}

void ST7789::dump_config() {
  LOG_DISPLAY("", "SPI ST7789", this);
  LOG_PIN("  CS Pin: ", this->cs_);
  LOG_PIN("  DC Pin: ", this->dc_pin_);
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
  LOG_PIN("  B/L Pin: ", this->backlight_pin_);
  LOG_UPDATE_INTERVAL(this);
}

float ST7789::get_setup_priority() const {
  return setup_priority::PROCESSOR;
}

void ST7789::update() {
  this->do_update_();
  this->write_display_data();
}

void ST7789::loop() {
}

void ST7789::write_display_data() {
  uint16_t x1 = 0;    // _offsetx
  uint16_t x2 = 239;  // _offsetx
  uint16_t y1 = 0;    // _offsety
  uint16_t y2 = 239;  // _offsety

  this->enable();

  // set column(x) address
  this->dc_pin_->digital_write(false);
  this->write_byte(ST7789_CASET);
  this->dc_pin_->digital_write(true);
  this->write_addr_(x1, x2);
  // set page(y) address
  this->dc_pin_->digital_write(false);
  this->write_byte(ST7789_RASET);
  this->dc_pin_->digital_write(true);
  this->write_addr_(y1, y2);
  // write display memory
  this->dc_pin_->digital_write(false);
  this->write_byte(ST7789_RAMWR);
  this->dc_pin_->digital_write(true);

  this->write_array(this->buffer_, this->get_buffer_length_());

  this->disable();
}

void ST7789::init_reset_() {
  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->setup();
    this->reset_pin_->digital_write(true);
    delay(1);
    // Trigger Reset
    this->reset_pin_->digital_write(false);
    delay(10);
    // Wake up
    this->reset_pin_->digital_write(true);
  }
}

void ST7789::backlight_(bool onoff) {
  if (this->backlight_pin_ != nullptr) {
    this->backlight_pin_->setup();
    this->backlight_pin_->digital_write(onoff);
  }
}

void ST7789::write_command_(uint8_t value) {
  this->enable();
  this->dc_pin_->digital_write(false);
  this->write_byte(value);
  this->dc_pin_->digital_write(true);
  this->disable();
}

void ST7789::write_data_(uint8_t value) {
  this->dc_pin_->digital_write(true);
  this->enable();
  this->write_byte(value);
  this->disable();
}

void ST7789::write_addr_(uint16_t addr1, uint16_t addr2) {
  /* static */ uint8_t BYTE[4];
  BYTE[0] = (addr1 >> 8) & 0xFF;
  BYTE[1] = addr1 & 0xFF;
  BYTE[2] = (addr2 >> 8) & 0xFF;
  BYTE[3] = addr2 & 0xFF;

  this->dc_pin_->digital_write(true);
  this->write_array(BYTE, 4);
}

void ST7789::write_color_(uint16_t color, uint16_t size) {
  /* static */ uint8_t BYTE[1024];
  int index = 0;
  for (int i = 0; i < size; i++) {
    BYTE[index++] = (color >> 8) & 0xFF;
    BYTE[index++] = color & 0xFF;
  }

  this->dc_pin_->digital_write(true);
  return write_array(BYTE, size * 2);
}

int ST7789::get_height_internal() {
  return 240;  // 320;
}

int ST7789::get_width_internal() {
  return 240;  // 240;
}

size_t ST7789::get_buffer_length_() {
  return size_t(this->get_width_internal()) *
         size_t(this->get_height_internal()) * 2;
}

// Draw a filled rectangle
// x1: Start X coordinate
// y1: Start Y coordinate
// x2: End X coordinate
// y2: End Y coordinate
// color: color
void ST7789::draw_filled_rect_(uint16_t x1, uint16_t y1, uint16_t x2,
                               uint16_t y2, uint16_t color) {
  // ESP_LOGD(TAG,"offset(x)=%d offset(y)=%d",dev->_offsetx,dev->_offsety);
  this->enable();
  this->dc_pin_->digital_write(false);
  this->write_byte(ST7789_CASET);  // set column(x) address
  this->dc_pin_->digital_write(true);
  this->write_addr_(x1, x2);

  this->dc_pin_->digital_write(false);
  this->write_byte(ST7789_RASET);  // set Page(y) address
  this->dc_pin_->digital_write(true);
  this->write_addr_(y1, y2);
  this->dc_pin_->digital_write(false);
  this->write_byte(ST7789_RAMWR);  // begin a write to memory
  this->dc_pin_->digital_write(true);
  for (int i = x1; i <= x2; i++) {
    uint16_t size = y2 - y1 + 1;
    this->write_color_(color, size);
  }
  this->disable();
}

void HOT ST7789::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (x >= this->get_width_internal() || x < 0 ||
      y >= this->get_height_internal() || y < 0)
    return;

  auto color565 = display::ColorUtil::color_to_565(color);

  uint32_t pos = (x + y * this->get_width_internal()) * 2;
  this->buffer_[pos++] = (color565 >> 8) & 0xff;
  this->buffer_[pos] = color565 & 0xff;
}

}  // namespace st7789
}  // namespace esphome
