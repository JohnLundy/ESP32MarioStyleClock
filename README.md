# ESP32 Mario Style Clock

A fun and nostalgic digital clock featuring Mario walking across a circular display to interact with time digits!

![Mario Clock Demo](mario%20clock.gif)

## Overview

This project transforms a 240x240 circular GC9A01 display into an interactive Mario-themed clock. Mario walks across the screen and "hits" each digit as time updates, creating a playful animation reminiscent of classic Mario games.

## Features

- **Animated Mario Character**: Mario walks across the screen and jumps to hit time digits
- **Dynamic Digit Bounce**: Digits bounce when hit by Mario, creating satisfying feedback
- **Circular Display**: Utilizes a GC9A01 circular TFT display for an elegant presentation
- **WiFi Time Sync**: Automatically synchronizes time via NTP (Network Time Protocol)
- **Customizable**: Support for 12/24-hour format and timezone configuration
- **Custom Fonts**: Uses Gamtex fonts for clear, retro-style time display

## Hardware Requirements

- **Microcontroller**: ESP32-C3
- **Display**: GC9A01 240x240 Circular TFT
- **Connections**:
  - TFT_BL (Backlight): Pin 3
  - TFT_RST (Reset): Pin 1
  - TFT_CS (Chip Select): Pin 10
  - TFT_DC (Data/Command): Pin 2
  - TFT_SCLK (Clock): Pin 6
  - TFT_MOSI (Data): Pin 7

## Software Requirements

- Arduino IDE with ESP32 support
- [Arduino_GFX Library](https://github.com/moononournation/Arduino_GFX)

## Installation

1. Clone this repository
2. Install the Arduino_GFX library via Arduino IDE Library Manager
3. Update WiFi credentials in `mario_ur.ino`:
   ```cpp
   const char* ssid = "Your Wi-Fi Network";
   const char* password = "Your Password";
   ```
4. Configure timezone offset (GMT offset in seconds):
   ```cpp
   const long gmtOffset_sec = 3600; // Adjust for your timezone
   ```
5. Upload to your ESP32-C3

## File Structure

- `mario_ur.ino` - Main sketch with animation logic and display control
- `assets.h` - Mario sprite graphics and tile data
- `Gamtex26pt7b.h` - 26pt font for time display
- `Gamtex8pt7b.h` - 8pt font for additional text

## Configuration

### Time Format
Change `USE_24H` to switch between 24-hour and 12-hour formats:
```cpp
#define USE_24H 1  // Set to 0 for 12h mode
```

### NTP Server
Modify the NTP server if needed:
```cpp
const char* ntpServer = "pool.ntp.org";
```

### Sprite Size
Adjust Mario sprite size (32 or 16 pixels):
```cpp
#define MARIOSPRITESIZE 32
```

## How It Works

1. **Time Synchronization**: On startup, the ESP32 connects to WiFi and syncs with an NTP server
2. **Display Rendering**: The circular display shows the current time with animated digits
3. **Mario Animation**: Mario walks across the screen from left to right
4. **Digit Interaction**: When Mario reaches a time digit, he jumps and "hits" it, triggering a bounce animation
5. **Continuous Loop**: The animation repeats as time updates

## License

This project is provided as-is for educational and personal use.

## Contributing

Feel free to fork, modify, and improve! Some ideas:
- Add different animation frames or characters
- Implement additional interactive features
- Add temperature or weather display
- Create different visual themes

Enjoy your Mario Clock! 🍄⏰
