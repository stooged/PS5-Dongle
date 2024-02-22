# PS5-Dongle
 
This is a project designed for the <a href="https://www.lilygo.cc/products/t-dongle-s3?variant=42455191519413">LilyGo T-Dongle-S3</a> to provide a wifi http(s) server, dns server.

<br>

it is for the <a href=https://github.com/Cryptogenic/PS5-IPV6-Kernel-Exploit>PS5 3.xx / 4.xx Kernel Exploit</a>.

using this fork of the exploit <a href=https://github.com/idlesauce/PS5-Exploit-Host>PS5 3.xx / 4.xx Kernel Exploit</a> by idlesauce.


<br>


you must use the LilyGo T-Dongle-S3 with this project and have a fat32 formatted sd card inserted into the dongle.

<a href="https://www.lilygo.cc/products/t-dongle-s3?variant=42455191486645">LilyGo T-Dongle-S3 With LCD</a><br>
<a href="https://www.lilygo.cc/products/t-dongle-s3?variant=42455191519413">LilyGo T-Dongle-S3 Without LCD</a><br>
<br>
<img src=https://github.com/stooged/PS5-Dongle/blob/main/images/dongle1.jpg><br>

## Libraries

the project is built using <b><a href=https://github.com/stooged/esp32_https_server>ESP32 HTTPS Server</a>, <a href=https://github.com/me-no-dev/ESPAsyncWebServer>ESPAsyncWebServer</a> and <a href=https://github.com/me-no-dev/AsyncTCP>AsyncTCP</a></b> so you need to add this library to arduino.

<a href=https://github.com/stooged/esp32_https_server>ESP32 HTTPS Server</a><br>
<a href=https://github.com/me-no-dev/ESPAsyncWebServer>ESPAsyncWebServer</a><br>
<a href=https://github.com/me-no-dev/AsyncTCP>AsyncTCP</a><br>

<br>

install or update the ESP32 core by adding this url to the <a href=https://docs.arduino.cc/learn/starting-guide/cores>Additional Boards Manager URLs</a> section in the arduino "<b>Preferences</b>".

` https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json `

then goto the "<b>Boards Manager</b> and install or update the "<b>esp32</b>" core.

<br>

if you have problems with the board being identified/found in windows then you might need to install the <a href=https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers>USB to UART Bridge</a> drivers.


<br>


## Uploading to board

installation is simple you just use the arduino ide to flash the sketch/firmware to the esp32 board.<br>

use the following board settings<br>

<img src=https://github.com/stooged/PS5-Dongle/blob/main/images/board.jpg>


<br>

## Udating HTML and payloads

to update the files on the dongle you just plug the dongle into your computer and treat it as a usb flash drive.

the exploit files are in the sdcard folder of the repo, copy them into the root of the dongles drive.


<br>


## Internal pages

* <b>admin.html</b> - the main landing page for administration.

* <b>info.html</b> - provides information about the esp board.

* <b>update.html</b> - used to update the firmware on the esp board (<b>fwupdate.bin</b>).

* <b>config.html</b> - used to configure wifi ap and ip settings.

* <b>reboot.html</b> - used to reboot the esp board


<br><br>



