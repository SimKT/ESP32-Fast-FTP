# ESP32-Fast-FTP
<br>
Fast FTP server for ESP32 based boards using SD cards as data storage. <br>
Uses specialised code to achieve the fastest transfer rates using SPI or MMC interfaces (700 KB/s +). <br><br>
Useful as a fast and cheap file, live audio and video server. Supports only one connection, but idle connections stay connected if no other clients are accesing.<br>
Supported prob by all major browsers like Chrome, Firefox and Edge, also Windows programs like TotalCommander and FileZilla, probably many more.<br>
KODI and VLC live video and audio support.<br>
AndFtp client for Android tested and works.<br><br>
Full Directory and long filenames support, supports creation editing and deletion of directories and files, does not support time functions yet<br>
File seek and append functions supported.<br>
Only single connection and passive server.<br>
Does not support encryption (prob not necessary), only a single user with password or no password.<br>
See the ESP32 online manuals and schemas for connecting the SD card to your ESP32.<br><br>
<h2>Installation</h2>
Copy the ESP32FastFTP directory from the archive contents to the %HOMEPATH%/Documents/Arduino/libraries, start Arduino IDE and select ESP32_FTP_server from Examples.
<h2>How To Use</h2>
Install the library, connect the SD card to your ESP32 board, start Arduino IDE and select your ESP32 board then select ESP32_FTP_server from Examples menu, edit the WiFi connection information and FTP user and pass, and flash the board. After the board restarts you should have a FTP server with the contents of your SD card.
