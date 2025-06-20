# Type D XL: Original Xbox TFT Display Project

<div align=center>
  <img src="https://github.com/Darkone83/Type-D/blob/main/images/logo.jpg">

  
</div>

**Type D XL** is a three-part open-source system for the original Xbox, bringing a modern, customizable, and fully themed display experience to your console. It replaces the stock jewel with a 240x240 TFT touch display, controlled by an ESP32-S3, and optionally pairs with the Type D Expansion module for live Xbox status telemetry.

---

## Features

### Type D XL Display Firmware

- Animated boot screen from `/boot/boot.jpg` or `/boot/boot.gif`  
- Image & GIF slideshow: Will select a random image on boot and process through them
- Touchscreen Xbox-style menu:    
  - Adjust brightness
  - View current WiFi information    
  - Initiate WiFi setup or reset WiFi credentials  
- Web interface: Upload and manage gallery images over WiFi  

## Required Hardware

- **Type D XL Display**: ESP32 S3 module (Waveshare ESP32-S3 2.1inch Round Touch Screen)

**AliExpress** <a href="https://www.aliexpress.us/item/3256807466301718.html?spm=a2g0o.productlist.main.1.74a94a0c4a0cIA&algo_pvid=78b4ff98-0353-4e5c-8b72-0343aeb06406&algo_exp_id=78b4ff98-0353-4e5c-8b72-0343aeb06406-0&pdp_ext_f=%7B%22order%22%3A%2210%22%2C%22eval%22%3A%221%22%7D&pdp_npi=4%40dis%21USD%2147.66%2147.66%21%21%21340.65%21340.65%21%402101c72a17502662863576144eaa57%2112000046392978428%21sea%21US%21196794698%21X&curPageLogUid=w5gvAlTpL6v5&utparam-url=scene%3Asearch%7Cquery_from%3A">Waveshare ESP32-S3 2.1inch Round Touch Screen</a>

**Amazon** <a href="https://www.amazon.com/dp/B0DDPQSKJD?ref=ppx_yo2ov_dt_b_fed_asin_title">Waveshare ESP32-S3 2.1inch Round Touch Screen</a>


## Purchasing

- **Full kit** <a href="https://www.darkonecustoms.com/store/p/type-d-xl">Darkone Customs</a>

- **Expansion module** <a href="https://www.darkonecustoms.com/store/p/type-d-telemetry-expansion">Darkone Customs</a>



---

## Required Libraries

_Install these via Arduino Library Manager or from their GitHub releases:_

- [`LovyanGFX`](https://github.com/lovyan03/LovyanGFX)
- [`AnimatedGIF`](https://github.com/bitbank2/AnimatedGIF)
- [`ESPAsyncWebserver`](https://github.com/me-no-dev/ESPAsyncWebServer)
- [`ESPAsyncTCP`](https://github.com/me-no-dev/ESPAsyncTCP)

---

## Build Instructions

### Type D Display 

#### Web Flasher: [![Type D XL Web Flasher](https://img.shields.io/badge/Web%20Flasher-Type%20D%20XL-green?logo=esp32&logoColor=white)](https://darkone83.github.io/type-d-xl.github.io/)

#### Build Instructions
1. **Install Arduino IDE** (recommended 2.x or later).
2. **Install the ESP32 board package**
3. **Install all required libraries** (see above).
4. **Setup board and options** Board** ESP32S3 Dev module, Flash size: 16MB, Partition Scheme: 16MB Flash (3MB APP/9.9MB FATFS), PSRAM: QPI PSRam
5. **Open the `Type_D_XL.ino` project** and verify it compiles.
6. **Flash the firmware** to your module using USB.
7. **Connect to WiFi** Connect the device via wifi and select your network with the custom portal.
8. **Upload files** Log in to the File Manager via the web interface Http://"device-ip":8080 and upload your media.
9. **Upload Resource files** Resource files can be uploaded via the resource manager Http://"device-ip":/8080/resources.


   **Notes:**
   - If no gallery images are present, a “No images found” screen will be shown.
   - File types supported are determined by firmware: common formats are `.jpg`,`.gif` (for UI assets).

## GIF Conversion

Use the script in the scripts folder to properly format your GIFS

Usage: gif_convert.py mygif.gif cool.gif

## Hardware installation

1. **Remove screws** Flip over your XBOX and removed all the bottom sace screws.

![alt text](https://github.com/Darkone83/Type-D-XL/blob/main/images/install/screws.jpg)

2. **Remove top shell and jewel** Remove the top shell and with a spudger or soft pritool, remove the XBOX jewel.

Before:

![alt text](https://github.com/Darkone83/Type-D-XL/blob/main/images/install/jewelb.jpg)

After:

![alt text](https://github.com/Darkone83/Type-D-XL/blob/main/images/install/jewela.jpg)

3. **Remove RF shield** Remove the RF shield by prying up the pinch points shown in red.

Before:

![alt text](https://github.com/Darkone83/Type-D-XL/blob/main/images/install/rfremove.jpg)

After:

![alt text](https://github.com/Darkone83/Type-D-XL/blob/main/images/install/norf.jpg)

4. **Cut out hole** Proceed to make your hole. It's recommended to start with a 66mm hole saw; the example image is a rough cut, but you want to ensure a snug fit while making sure you have plenty of clearance.

![alt_text](https://github.com/Darkone83/Type-D-XL/blob/main/images/install/rough_hole.jpg)

5. **Fit display** Test fit and trim as needed. A small bead of hot melt glue is plenty for holding in the display. Make sure you ribbon and USB-C ports are facing the front of the XBOX. See the reference image for the bottom.

Top:

![alt text](https://github.com/Darkone83/Type-D-XL/blob/main/images/install/testfit.jpg)

Bottom:

![alt text](https://github.com/Darkone83/Type-D-XL/blob/main/images/install/testfitb.jpg)

5. **Install harness** The display needs 3.3v (you can use 5v if you use the JST 1.0 12P connector), except when using USB C. We will be using the UART connector for this install. You can make your own witch uses a JST 1.0 4P connector or use the supplied one with the display.

![alt text](https://github.com/Darkone83/Type-D-XL/blob/main/images/install/harness.jpg)

6. **Re-install RF shield** Reinstall your RF shield and route wires as shown or where is best for your installation.

![alt text](https://github.com/Darkone83/Type-D-XL/blob/main/images/install/rfroute.jpg)

7. **Connect power** Connect your 3.3V and GND points to any suitable point for your installation.

8. **Reassemble** Put your XBOX back together and enjoy the awesome display.

Powered Off:

![alt text](https://github.com/Darkone83/Type-D-XL/blob/main/images/install/install.jpg)

Booting UP:

![alt text](https://github.com/Darkone83/Type-D-XL/blob/main/images/install/boot.jpg)

## WiFi Connection:

Set up your WiFi by joining the Type D XL Setup wifi network that Type D XL broadcasts. Join your preferred network and access the file manager to upload your content with the built-in file manager Http://"device ip":8080


## 🗺️ Navigation & Menu Tree

**Main Menu Structure:**
```
[ Main Menu ]
 ├─ Settings
 │    ├─ Brightness
 │    └─ WiFi info
 │    └─ Forget WiFi
 │    └─ Back
 ├─ About
 └─ Exit
```

### Navigation

- **Touch**
  - *Single tap*: Select menu item
  - *Swipe up/down*: Scroll through menus or items
  - *Long Press*: Enter Type D XL Menu
- **Settings Menu**
  - *Brightness*: Adjust display backlight (0-100%)
  - *WiFi Info*: Displays current WiFi network connection and IP address
  - *Forget WiFi*: Forgets WiFi settings and restarts the connection portal
  - *Back*: Returns to main menu
- **About**: Shows build/version info and credits
- **Exit**: Exits menu
---

## How It Works

- **Type D XL Display** runs as a stand-alone smart dashboard and animated photo frame, displaying images and menus when no Xbox is online.
- **Type D Display** Works along the XL display broadcasting on separate ID's
- When a **Type D Expansion** module is active, the display instantly receives Xbox system telemetry and shows a themed overlay with the latest status—no configuration required. (Still in development)

---

## Diagnostics

You can access the diagnostic page once you have connected to wifi by visiting HTTP://"device IP":8080/diag

## Notes

- GIF support is experimental! Keep your GIF's under 1MB. Larger GIF's may work, but cause crashing of the firmware.

___

## Special Thanks

- Andr0, Team Resurgent, Xbox Scene, and the Xbox modding community

---

**Questions, feedback, or want to contribute? Open an issue or PR on GitHub!**
