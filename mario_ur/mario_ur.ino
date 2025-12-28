/*
 * Mario Clock for GC9A01 240x240 Circular Display (Arduino_GFX)
 * ESP32-C3 + GC9A01 (Arduino_GFX library)
 * Requires: https://github.com/moononournation/Arduino_GFX
 */
#include <Arduino_GFX_Library.h>
#include <WiFi.h>
#include <time.h>
#include "assets.h"
#include "Gamtex26pt7b.h"
#include "Gamtex8pt7b.h"

#define TFT_BL 3   // Backlight pin
#define TFT_RST 1  // Reset pin
#define TFT_CS 10  // Chip select
#define TFT_DC 2   // Data/command
#define TFT_SCLK 6 // Clock
#define TFT_MOSI 7 // Data

#define MARIOSPRITESIZE 32 // 32 for 32pixel and 16 for 16pixel sprites

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240
#define CENTER_X (SCREEN_WIDTH/2)
#define CENTER_Y (SCREEN_HEIGHT/2)

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, -1 /*MISO*/);
Arduino_GC9A01 *gfx = new Arduino_GC9A01(bus, TFT_RST, 0 /*rotation*/, true /*IPS*/);
Arduino_Canvas *canvas = nullptr;

// ========== Mario Animation Variables ==========
float mario_x = 20;
int mario_base_y = 170;
float mario_jump_y = 0;
bool mario_facing_right = true;
int mario_walk_frame = 0;
unsigned long last_mario_update = 0;
const int MARIO_ANIM_SPEED = (50*16)/MARIOSPRITESIZE;

enum MarioState {
  MARIO_IDLE,
  MARIO_WALKING,
  MARIO_JUMPING,
  MARIO_WALKING_OFF
};
MarioState mario_state = MARIO_IDLE;

// Digit targeting
int target_x_positions[4];
int target_digit_index[4];
int target_digit_values[4];
int num_targets = 0;
int current_target_index = 0;
int last_minute = -1;
bool animation_triggered = false;

// Jump physics
float jump_velocity = 0;
const float GRAVITY = 0.6;
const float JUMP_POWER = -7.5;
bool digit_bounce_triggered = false;

const int MARIO_HEAD_OFFSET = 18;
const int DIGIT_BOTTOM = 120;

// Displayed time
int displayed_hour = -1;
int displayed_min = -1;
bool time_overridden = false;
// For per-digit animation
int displayed_digits[4] = {-1, -1, -1, -1};
bool first_boot = true;

// Digit bounce animation
float digit_offset_y[5] = {0, 0, 0, 0, 0};
float digit_velocity[5] = {0, 0, 0, 0, 0};
const float DIGIT_BOUNCE_POWER = -6.5;
const float DIGIT_GRAVITY = 0.6;

const int DIGIT_X[5] = {50, 80, 108, 135, 165};
const int TIME_Y = 80;

// ========== Time Settings ==========
#define USE_24H 1 // Set to 0 for 12h mode
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600; // Set your timezone offset
const int daylightOffset_sec = 0;

// WiFi credentials (replace with your own)
const char* ssid = "My Wi-Fi Network";
const char* password = "PASSWORD";

void tryBacklight(bool high) {
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, high ? HIGH : LOW);
}

void setup() {
  tryBacklight(HIGH);
  delay(100);
  gfx->begin();
  gfx->setRotation(3); // 90 degrees, matches ESPHome
  gfx->fillScreen(SKYBLUE);
  canvas = new Arduino_Canvas(SCREEN_WIDTH, SCREEN_HEIGHT, gfx);
  canvas->begin();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  // NTP time sync
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    canvas->setTextColor(WHITE);
    canvas->setCursor(CENTER_X - 50, CENTER_Y - 8);
    canvas->setFont(&Gamtex26pt7b);
    //canvas->setTextSize(2);
    canvas->print("Time Error");
    delay(1000);
    return;
  }

  // Only update once per frame
  static unsigned long lastDraw = 0;
  if (millis() - lastDraw < 30) return;
  lastDraw = millis();

  canvas->fillScreen(0x5C7E);
  drawStaticSpriteScaled(BUSH, scenery_palette, 21, 9, 160, 173,3);
  drawStaticSpriteScaled(HILL, scenery_palette, 20, 22, 5, 134,3);
  drawStaticSpriteScaled(CLOUD2, scenery_palette, 41, 18, -10, 50,3);
  drawStaticSpriteScaled(CLOUD1, scenery_palette, 26, 19, 150, 30,3);
  drawStaticSpriteScaled(GROUND, scenery_palette, 8, 8, 28, 200,3);
  drawStaticSpriteScaled(GROUND, scenery_palette, 8, 8, 52, 200,3);
  drawStaticSpriteScaled(GROUND, scenery_palette, 8, 8, 76, 200,3);
  drawStaticSpriteScaled(GROUND, scenery_palette, 8, 8, 100, 200,3);
  drawStaticSpriteScaled(GROUND, scenery_palette, 8, 8, 124, 200,3);
  drawStaticSpriteScaled(GROUND, scenery_palette, 8, 8, 148, 200,3);
  drawStaticSpriteScaled(GROUND, scenery_palette, 8, 8, 172, 200,3);
  drawStaticSpriteScaled(GROUND, scenery_palette, 8, 8, 196, 200,3);
  drawStaticSpriteScaled(GROUND, scenery_palette, 8, 8, 52, 224,3);
  drawStaticSpriteScaled(GROUND, scenery_palette, 8, 8, 76, 224,3);
  drawStaticSpriteScaled(GROUND, scenery_palette, 8, 8, 100, 224,3);
  drawStaticSpriteScaled(GROUND, scenery_palette, 8, 8, 124, 224,3);
  drawStaticSpriteScaled(GROUND, scenery_palette, 8, 8, 148, 224,3);
  drawStaticSpriteScaled(GROUND, scenery_palette, 8, 8, 172, 224,3); 
  displayClockWithMario(&timeinfo);
  canvas->flush();
}

void drawStaticSprite(const uint8_t *sprite,
                      const uint16_t *palette,
                      int width, int height,
                      int x0, int y0)
{
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            uint8_t pix = sprite[row * width + col];
            if (pix != 0) { // 0 = transparent
                canvas->drawPixel(x0 + col, y0 + row, palette[pix]);
            }
        }
    }
}

// Generalized static sprite draw with arbitrary scaling
void drawStaticSpriteScaled(const uint8_t *sprite,
                            const uint16_t *palette,
                            int width, int height,
                            int x0, int y0,
                            int scale)   // 
{
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            uint8_t pix = sprite[row * width + col];
            if (pix != 0) { // 0 = transparent
                uint16_t color = palette[pix];
                // Draw a scale×scale block for each pixel
                for (int dy = 0; dy < scale; dy++) {
                    for (int dx = 0; dx < scale; dx++) {
                        canvas->drawPixel(x0 + col * scale + dx,
                                          y0 + row * scale + dy,
                                          color);
                    }
                }
            }
        }
    }
}

void drawMario(int x, int y, bool facingRight, int frame, bool jumping) {
  // Select sprite based on state and frame
  const uint8_t (*sprite)[MARIOSPRITESIZE];
  if (jumping) {
    sprite = facingRight ? mario_jump : mario_jump_left;
  } else if (mario_state == MARIO_WALKING || mario_state == MARIO_WALKING_OFF) {
    if (frame == 0)
      sprite = facingRight ? mario_walk_frame_0 : mario_walk_frame_0_left;
    else if (frame == 1)
      sprite = facingRight ? mario_walk_frame_1 : mario_walk_frame_1_left;
    else
      sprite = facingRight ? mario_walk_frame_2 : mario_walk_frame_2_left;
  } else {
    sprite = facingRight ? mario_stand : mario_stand_left;
  }
  for (int row = 0; row < MARIOSPRITESIZE; row++) {
    for (int col = 0; col < MARIOSPRITESIZE; col++) {
      int draw_col = facingRight ? (MARIOSPRITESIZE - 1 - col) : col;
      uint8_t pix = sprite[row][draw_col];
      if (pix != 0) {
        canvas->drawPixel(x + col, y + row, mario_palette[pix]);
      }
    }
  }
}

// Center clock and date lower on the screen
void displayClockWithMario(struct tm* timeinfo) {
  // Date at lower third, centered
  char dateStr[12];
  sprintf(dateStr, "%02d/%02d/%04d", timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900);
  canvas->setTextColor(WHITE);
  canvas->setFont(&Gamtex8pt7b);
  //canvas->setTextSize(2);
  int16_t x1, y1; uint16_t w, h;
  canvas->getTextBounds(dateStr, 0, 0, &x1, &y1, &w, &h);
  int date_y = 20;
  canvas->setCursor((SCREEN_WIDTH - w) / 2, date_y);
  canvas->print(dateStr);

  // Time (centered vertically)
  updateDigitBounce();
  drawTimeWithBounce(timeinfo);

  updateMarioAnimation(timeinfo);
  int mario_draw_y = mario_base_y + (int)mario_jump_y;
  bool isJumping = (mario_state == MARIO_JUMPING);
  drawMario((int)mario_x, mario_draw_y, mario_facing_right, mario_walk_frame, isJumping);
}

void drawTimeWithBounce(struct tm* timeinfo) {
  int hour = timeinfo->tm_hour;
  bool isPM = false;
#if !USE_24H
  isPM = hour >= 12;
  hour = hour % 12;
  if (hour == 0) hour = 12;
#endif
  int min = timeinfo->tm_min;
  displayed_hour = hour;
  displayed_min = min;

  // Use displayed_digits for animation
  char digits[6];
  digits[0] = (displayed_digits[0] >= 0 && displayed_digits[0] <= 9) ? ('0' + displayed_digits[0]) : '/';
  digits[1] = (displayed_digits[1] >= 0 && displayed_digits[1] <= 9) ? ('0' + displayed_digits[1]) : '/';
  digits[2] = ':';
  digits[3] = (displayed_digits[2] >= 0 && displayed_digits[2] <= 9) ? ('0' + displayed_digits[2]) : '/';
  digits[4] = (displayed_digits[3] >= 0 && displayed_digits[3] <= 9) ? ('0' + displayed_digits[3]) : '/';
  digits[5] = '\0';

  //canvas->setTextSize(5);
  canvas->setFont(&Gamtex26pt7b);
  canvas->setTextColor(WHITE);
  int16_t x1, y1; uint16_t w, h;
  canvas->getTextBounds(digits, 0, 0, &x1, &y1, &w, &h);
  int time_x = (SCREEN_WIDTH - w) / 2;
  int time_y = 155 - h / 2; // Centered lower
  canvas->setCursor(time_x, time_y);
  canvas->print(digits);
#if !USE_24H
  //canvas->setTextSize(2);
  canvas->setFont(&Gamtex26pt7b);
  canvas->setTextColor(WHITE);
  canvas->setCursor(time_x + w + 10, time_y + 30);
  canvas->print(isPM ? "PM" : "AM");
#endif
}

void updateDigitBounce() {
  for (int i = 0; i < 5; i++) {
    if (digit_offset_y[i] != 0 || digit_velocity[i] != 0) {
      digit_velocity[i] += DIGIT_GRAVITY;
      digit_offset_y[i] += digit_velocity[i];
      if (digit_offset_y[i] >= 0) {
        digit_offset_y[i] = 0;
        digit_velocity[i] = 0;
      }
    }
  }
}

void triggerDigitBounce(int digitIndex) {
  if (digitIndex >= 0 && digitIndex < 5) {
    digit_velocity[digitIndex] = DIGIT_BOUNCE_POWER;
  }
}

void updateMarioAnimation(struct tm* timeinfo) {
  unsigned long currentMillis = millis();
  if (currentMillis - last_mario_update < MARIO_ANIM_SPEED) return;
  last_mario_update = currentMillis;

  int seconds = timeinfo->tm_sec;
  int current_minute = timeinfo->tm_min;

  if (current_minute != last_minute) {
    last_minute = current_minute;
    animation_triggered = false;
  }

  // On first boot, animate Mario to set each digit in sequence
  if (first_boot && mario_state == MARIO_IDLE) {
    int hour = timeinfo->tm_hour;
    int min = timeinfo->tm_min;
    int correct_digits[4] = {hour / 10, hour % 10, min / 10, min % 10};
    // Build a list of all digits that need to be set
    num_targets = 0;
    for (int i = 0; i < 4; i++) {
      if (displayed_digits[i] != correct_digits[i]) {
        int digit_idx = (i < 2) ? i : i + 1;
        target_x_positions[num_targets] = DIGIT_X[digit_idx] - 8;
        target_digit_index[num_targets] = digit_idx;
        target_digit_values[num_targets] = correct_digits[i];
        num_targets++;
      }
    }
    if (num_targets > 0) {
      current_target_index = 0;
      mario_x = -20; // start off-screen left
      mario_state = MARIO_WALKING;
      mario_facing_right = true;
      digit_bounce_triggered = false;
      // Set all unset digits to '/'
      for (int i = 0; i < 4; i++) {
        if (displayed_digits[i] < 0 || displayed_digits[i] > 9) displayed_digits[i] = -1;
      }
    } else {
      first_boot = false; // All digits set
    }
  }
  else if (seconds >= 55 && !animation_triggered && mario_state == MARIO_IDLE && !first_boot) {
    animation_triggered = true;
    // Prepare per-digit animation
    int hour = timeinfo->tm_hour;
    int min = timeinfo->tm_min;
    displayed_digits[0] = hour / 10;
    displayed_digits[1] = hour % 10;
    displayed_digits[2] = min / 10;
    displayed_digits[3] = min % 10;
    calculateTargetDigits(hour, min);
    if (num_targets > 0) {
      current_target_index = 0;
      mario_x = -20; // start off-screen left
      mario_state = MARIO_WALKING;
      mario_facing_right = true;
      digit_bounce_triggered = false;
    }
  }

  switch (mario_state) {
    case MARIO_IDLE:
      mario_walk_frame = 0;
      mario_x = -20; // keep Mario out of frame when idle
      break;
    case MARIO_WALKING:
      if (current_target_index < num_targets) {
        int target = target_x_positions[current_target_index];
        if (abs((mario_x + 8) - (target + 8)) > 2) { // Mario's center to digit center
          if (mario_x < target) {
            mario_x += 3.5;
            mario_facing_right = true;
          } else {
            mario_x -= 3.5;
            mario_facing_right = false;
          }
          mario_walk_frame = (mario_walk_frame + 1) % 3;
        } else {
          mario_x = target;
          mario_state = MARIO_JUMPING;
          jump_velocity = JUMP_POWER;
          mario_jump_y = 0;
          digit_bounce_triggered = false;
          mario_walk_frame = 0; // reset to stand before jump
        }
      } else {
        mario_state = MARIO_WALKING_OFF;
        mario_facing_right = true;
      }
      break;
    case MARIO_JUMPING: {
      jump_velocity += GRAVITY;
      mario_jump_y += jump_velocity;
      int mario_head_y = mario_base_y + (int)mario_jump_y - MARIO_HEAD_OFFSET;
      int mario_center_x = (int)mario_x + 8;
      int digit_index = target_digit_index[current_target_index];
      int digit_center_x = DIGIT_X[digit_index];
      // Stand before and after jump
      if (mario_jump_y < 0 && mario_jump_y > -8) mario_walk_frame = 1; // midair
      else mario_walk_frame = 0; // stand
      if (!digit_bounce_triggered && mario_head_y <= DIGIT_BOTTOM && abs(mario_center_x - digit_center_x) <= 2) {
        digit_bounce_triggered = true;
        triggerDigitBounce(digit_index);
        if (first_boot) {
          // Set the correct digit for this position
          int hour = timeinfo->tm_hour;
          int min = timeinfo->tm_min;
          int correct_digits[4] = {hour / 10, hour % 10, min / 10, min % 10};
          for (int i = 0; i < 4; i++) {
            int idx = (i < 2) ? i : i + 1;
            if (idx == digit_index) {
              displayed_digits[i] = correct_digits[i];
            }
          }
        } else {
          // Update only the digit Mario just hit (normal animation)
          int next_min = (displayed_digits[2] * 10 + displayed_digits[3] + 1) % 60;
          int next_hour = displayed_digits[0] * 10 + displayed_digits[1];
          if (next_min == 0) next_hour = (next_hour + 1) % 24;
          int next_digits[4] = {next_hour / 10, next_hour % 10, next_min / 10, next_min % 10};
          for (int i = 0; i < 4; i++) {
            int idx = (i < 2) ? i : i + 1;
            if (idx == digit_index) {
              displayed_digits[i] = next_digits[i];
            }
          }
        }
        jump_velocity = 3.0;
      }
      if (mario_jump_y >= 0) {
        mario_jump_y = 0;
        jump_velocity = 0;
        current_target_index++;
        if (current_target_index < num_targets) {
          mario_state = MARIO_WALKING;
          mario_facing_right = (target_x_positions[current_target_index] > mario_x);
          digit_bounce_triggered = false;
        } else {
          mario_state = MARIO_WALKING_OFF;
          mario_facing_right = true;
        }
      }
    } break;
    case MARIO_WALKING_OFF:
      mario_x += 3.5;
      mario_walk_frame = (mario_walk_frame + 1) % 3;
      if (mario_x > SCREEN_WIDTH + 15) {
        mario_state = MARIO_IDLE;
        mario_x = -20; // move Mario off-screen immediately
      }
      break;
  }
}

void calculateTargetDigits(int hour, int min) {
  num_targets = 0;
  int next_min = (min + 1) % 60;
  int next_hour = hour;
  if (next_min == 0) next_hour = (hour + 1) % 24;
  int curr_digits[4] = {hour / 10, hour % 10, min / 10, min % 10};
  int next_digits[4] = {next_hour / 10, next_hour % 10, next_min / 10, next_min % 10};
  // Collect targets in left-to-right order
  for (int i = 0; i < 4; i++) {
    if (curr_digits[i] != next_digits[i]) {
      int digit_idx = (i < 2) ? i : i + 1; // skip colon
      target_x_positions[num_targets] = DIGIT_X[digit_idx] - 8; // -8 to center Mario (16px wide)
      target_digit_index[num_targets] = digit_idx;
      target_digit_values[num_targets] = next_digits[i];
      num_targets++;
    }
  }
}
