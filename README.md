# ESP32-Fast-FTP
Fast FTP server for ESP32 based boards using SD cards as data storage. <br>
Uses specialised code to achieve the fastest transfer rates using SPI or MMC interfaces (700 KB/s +). <br><br>
Useful as a fast and cheap file, live audio and video server. Supports only one connection, but idle connections stay connected if no other clients are accesing.<br>
Supported prob by all major browsers like Chrome, Firefox and Edge, also Windows programs like TotalCommander and FileZilla, probably many more.<br>
KODI and VLC live video and audio support.<br>
AndFtp client for Android tested and works.<br><br>
Full Directory and long filenames support, does not support time functions yet<br>
File seek and append functions supported.<br>
Only single connection and passive server.<br>
Does not support encryption (prob not necessary), only a single user with password or no password.<br>
See the ESP32 for connection schemas for various interfaces.<br><br>
<h2>Installation</h2>
<br>
Copy the archive contents to the %HOMEPATH%/Documents/Arduino/libraries, start Arduino IDE and select ESP32_FTP_server from Examples.