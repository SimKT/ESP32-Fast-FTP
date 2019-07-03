
/*
 * Fast FTP SERVER FOR ESP32
 * based on FTP Server for Arduino Due and Ethernet shield (W5100) or WIZ820io
 * (W5200)
 * based on Jean-Michel Gallego's work
 * modified to work with ESP by SimKT
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ESP32FastFTP.h"
#include <WiFi.h>
#include <WiFiClient.h>
/*
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
*/

char buf[FTP_BUF_SIZE];

WiFiServer ftpServer(FTP_CTRL_PORT);
WiFiServer dataServer(FTP_DATA_PORT_PASV);
int filee; // helper low lever file pointer

void FtpServer::initSD() {
  ESP_LOGI(TAG, "Initializing SD card");

  ESP_LOGI(TAG, "Using SPI peripheral");

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
  slot_config.gpio_miso = GPIO_NUM_19;
  slot_config.gpio_mosi = GPIO_NUM_23;
  slot_config.gpio_sck = GPIO_NUM_18;
  slot_config.gpio_cs = GPIO_NUM_5;
  // This initializes the slot without card detect (CD) and write protect (WP)
  // signals.
  // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these
  // signals.

  esp_log_level_set("*", ESP_LOG_VERBOSE);
  // Options for mounting the filesystem.
  // If format_if_mount_failed is set to true, SD card will be partitioned and
  // formatted in case when mounting fails.

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 10,
      .allocation_unit_size = 16 * 1024};

  // Use settings defined above to initialize SD card and mount FAT filesystem.
  // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
  // Please check its source code and implement error recovery when developing
  // production applications.
  sdmmc_card_t *card;
  esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config,
                                          &mount_config, &card);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount filesystem. "
                    "If you want the card to be formatted, set "
                    "format_if_mount_failed = true.");
    } else {
      ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                    "Make sure SD card lines have pull-up resistors in place.",
               esp_err_to_name(ret));
    }
    return;
  }
  // Card has been initialized, print its properties
  sdmmc_card_print_info(stdout, card);
  cmdStatus = -1;
}
void FtpServer::initSD_MMC() {
  ESP_LOGI(TAG, "Initializing SD card");

  ESP_LOGI(TAG, "Using SDMMC peripheral");
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();

  // This initializes the slot without card detect (CD) and write protect (WP)
  // signals.
  // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these
  // signals.
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

  // To use 4-line SD mode, comment the following line:
  slot_config.width = 1;

  // GPIOs 15, 2, 4, 12, 13 should have external 10k pull-ups.
  // Internal pull-ups are not sufficient. However, enabling internal pull-ups
  // does make a difference some boards, so we do that here.
  gpio_set_pull_mode(GPIO_NUM_15,
                     GPIO_PULLUP_ONLY); // CMD, needed in 4- and 1- line modes
  gpio_set_pull_mode(GPIO_NUM_2,
                     GPIO_PULLUP_ONLY); // D0, needed in 4- and 1-line modes
  gpio_set_pull_mode(GPIO_NUM_4,
                     GPIO_PULLUP_ONLY); // D1, needed in 4-line mode only
  gpio_set_pull_mode(GPIO_NUM_12,
                     GPIO_PULLUP_ONLY); // D2, needed in 4-line mode only
  gpio_set_pull_mode(GPIO_NUM_13,
                     GPIO_PULLUP_ONLY); // D3, needed in 4- and 1-line modes
  host.max_freq_khz = 40000;

  esp_log_level_set("*", ESP_LOG_VERBOSE);
  // Options for mounting the filesystem.
  // If format_if_mount_failed is set to true, SD card will be partitioned and
  // formatted in case when mounting fails.

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 10,
      .allocation_unit_size = 16 * 1024};

  // Use settings defined above to initialize SD card and mount FAT filesystem.
  // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
  // Please check its source code and implement error recovery when developing
  // production applications.
  sdmmc_card_t *card;
  esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config,
                                          &mount_config, &card);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount filesystem. "
                    "If you want the card to be formatted, set "
                    "format_if_mount_failed = true.");
    } else {
      ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                    "Make sure SD card lines have pull-up resistors in place.",
               esp_err_to_name(ret));
    }
    return;
  }
  // Card has been initialized, print its properties
  sdmmc_card_print_info(stdout, card);
  cmdStatus = -1;
}
void FtpServer::configPassiveIP(IPAddress cfg) {
  customDataIP = true;
  cdataIP = cfg;
}

void FtpServer::abortSecondClient() {
  tempClient = ftpServer.available();
  if (tempClient) {
    tempClient.println("425 bye");
    tempClient.stop();
  }
}

void FtpServer::beginFTP(String Username, String Password, uint16_t port,
                         uint16_t dataPort) {
  if (cmdStatus == -2) {
#ifdef FTP_DEBUG
    Serial.println("Ypu have to initialize the SD card first either with "
                   ".initSD if you have SPI interface or .initSD_MMC for MMC "
                   "interface");
#endif
    return;
  }

  // Tells the ftp server to begin listening for incoming connection
  _FTP_USER = Username;
  _FTP_PASS = Password;

  this->dataPort = dataPort;
  Port = port;
  ftpServer.begin(port);
  delay(10);
  dataServer.begin(dataPort);
  delay(10);
  millisTimeOut = (uint32_t)FTP_TIME_OUT * 1 * 1000;
  millisDelay = 0;
  cmdStatus = 0;
  iniVariables();
}

void FtpServer::iniVariables() {
  // Default for data port
  dataPort = FTP_DATA_PORT_PASV;

  // Default Data connection is Active
  dataPassiveConn = true;

  // Set the root directory
  strcpy(cwdName, "/");

  rnfrCmd = false;
  transferStatus = 0;
}

void FtpServer::handleFTP() {

  if (millisDelay > millis()) {
    delay(10);
    return;
  }

  if (cmdStatus == 0) {
    if (client && client.connected())
      disconnectClient();
    cmdStatus = 1;
  } else if (cmdStatus == 1) // Ftp server waiting for connection
  {
    abortTransfer();
    iniVariables();
#ifdef FTP_DEBUG
    Serial.println("Ftp server waiting for connection on port " + String(Port));
#endif
    cmdStatus = 2;

  } else if (cmdStatus == 2) // Ftp server idle
  {

    if (!client || !client.connected())
      client = ftpServer.available();

    if (client.connected()) // A client connected
    {
      clientConnected();
#ifdef FTP_DEBUG
      Serial.println("connected");
#endif
      millisEndConnection = millis() + 10 * 1000; // wait client id during 10 s.
      cmdStatus = 3;
      delay(300);
    }
  } else if (readChar() > 0) // got response
  {
    if (cmdStatus == 3) // Ftp server waiting for user identity
      if (userIdentity())
        cmdStatus = 4;
      else
        cmdStatus = 0;
    else if (cmdStatus == 4) // Ftp server waiting for user registration
      if (userPassword()) {
        cmdStatus = 5;
        millisEndConnection = millis() + millisTimeOut;
      } else
        cmdStatus = 0;
    else if (cmdStatus == 5) // Ftp server waiting for user command
      if (!processCommand())
        cmdStatus = 0;
      else
        millisEndConnection = millis() + millisTimeOut;
  } else if (!client.connected() || !client) {
    cmdStatus = 1;
#ifdef FTP_DEBUG
    Serial.println("client disconnected");
#endif
  }

  if (transferStatus == 1) // Retrieve data
  {
    if (!doRetrieve())
      transferStatus = 0;
  } else if (transferStatus == 2) // Store data
  {
    if (!doStore())
      transferStatus = 0;
  } else if (cmdStatus >= 3 && (!client || !client.connected())) {
    cmdStatus = 0;
  } else if (cmdStatus > 2 &&
             !((int32_t)(millisEndConnection - millis()) > 0)) {
    if (!client || !client.connected()) {
      client.println("530 Timeout");
      millisDelay = millis() + 200; // delay of 200 ms
      cmdStatus = 0;

    } else {
      tempClient = ftpServer.available();
      if (tempClient) {

        client.println("530 Timeout");
        millisDelay = millis() + 200; // delay of 200 ms
        cmdStatus = 1;
        client = tempClient;
      } else
        millisEndConnection += millisTimeOut;
    }
  }
}

void FtpServer::clientConnected() {
#ifdef FTP_DEBUG
  Serial.println("Client connected!");
#endif
  client.println("220 Hello from FTP. Enter User");
  CMDlen = 0;
}

void FtpServer::disconnectClient() {
#ifdef FTP_DEBUG
  Serial.println(" Disconnecting client");
#endif
  abortTransfer();
  client.println("221 Goodbye");
  client.stop();
}

boolean FtpServer::userIdentity() {
  if (strcmp(command, "USER"))
    client.println("500 Syntax error");
  if (strcmp(parameters, _FTP_USER.c_str()))
    client.println("530 user not found");
  else {
    client.println("331 OK. Password required");
    strcpy(cwdName, "/");
    return true;
  }
  millisDelay = millis() + 100; // delay of 100 ms
  return false;
}

boolean FtpServer::userPassword() {
  if (strcmp(command, "PASS"))
    client.println("500 Syntax error");
  else if (strcmp(parameters, _FTP_PASS.c_str()))
    client.println("530 ");
  else {
#ifdef FTP_DEBUG
    Serial.println("OK. Waiting for commands.");
#endif
    client.println("230 OK.");
    return true;
  }
  millisDelay = millis() + 100; // delay of 100 ms
  return false;
}

boolean FtpServer::processCommand() {
  ///////////////////////////////////////
  //                                   //
  //      ACCESS CONTROL COMMANDS      //
  //                                   //
  ///////////////////////////////////////

  //
  //  CDUP - Change to Parent Directory
  //
  if (!strcmp(command, "CDUP")) {
#ifdef FTP_DEBUG
    Serial.println("CDUP ");
#endif
    int k = strlen(cwdName);
    if (k > 1) {
      cwdName[--k] = 0;
      while (cwdName[k] != '/') {
        cwdName[k--] = 0;
      }
    }
    if (k > 1)
      cwdName[k] = 0;
    client.println("250 Ok. Current directory is " + String(cwdName));
  }
  //
  //  CWD - Change Working Directory
  //
  else if (!strcmp(command, "CWD")) {
    char path[FTP_CWD_SIZE];
    Serial.println(parameters);
    if (strcmp(parameters, ".") == 0) // 'CWD .' is the same as PWD command
      client.println("257 \"" + String(cwdName) +
                     "\" is your current directory");
    else if (strncmp(parameters, "\..", 3) ==
             0) // 'CWD .' is the same as PWD command
      client.println("550 invalid directory");
    else {
      if (makePath(path))
        strcpy(cwdName, path);
      myFile root;
      root.open(cwdName);
      if (!root) {
        client.println("550 Can't open directory " + String(cwdName));
        // return;
      } else if (!root.isDirectory()) {
        client.println("550 Can't open directory " + String(cwdName));
#ifdef FTP_DEBUG
        Serial.println("Not a directory");
#endif
        // root.close()

      } else
        client.println("250 Ok. Current directory is " + String(cwdName));
    }
  }
  //
  //  PWD - Print Directory
  //
  else if (!strcmp(command, "PWD"))
    client.println("257 \"" + String(cwdName) + "\" is your current directory");
  //
  //  QUIT
  //
  else if (!strcmp(command, "QUIT") || !strcmp(command, "EXIT") ||
           !strcmp(command, "BYE")) {
    disconnectClient();
    return false;
  }

  ///////////////////////////////////////
  //                                   //
  //    TRANSFER PARAMETER COMMANDS    //
  //                                   //
  ///////////////////////////////////////

  //
  //  MODE - Transfer Mode
  //
  else if (!strcmp(command, "MODE")) {
    if (!strcmp(parameters, "S"))
      client.println("200 S Ok");
    // else if( ! strcmp( parameters, "B" ))
    //  client.println( "200 B Ok\r\n";
    else
      client.println("504 Only S(tream) is suported");
  }
  //
  //  PASV - Passive Connection management
  //
  else if (!strcmp(command, "PASV")) {
    // if (data) data.stop();
    dataIP = client.localIP();
    if (customDataIP == false)
      this->cdataIP = client.localIP();

// data = dataServer.available();
#ifdef FTP_DEBUG
    Serial.println("Connection management set to passive");
    Serial.println("Data port set to " + String(dataPort));
#endif
    client.println("227 Entering Passive Mode (" + String(cdataIP[0]) + "," +
                   String(cdataIP[1]) + "," + String(cdataIP[2]) + "," +
                   String(cdataIP[3]) + "," + String(dataPort >> 8) + "," +
                   String(dataPort & 255) + ").");
    dataPassiveConn = true;
  }
  //
  //  PORT - Data Port
  //
  else if (!strcmp(command, "PORT")) {
    if (data)
      data.stop();
    // get IP of data client
    client.println("501 Can't interpret parameters");
    return true; // implementation not supported/ breaks forwarded ports
    /*
    dataIp[ 0 ] = atoi( parameters );
    char * p = strchr( parameters, ',' );
    for( uint8_t i = 1; i < 4; i ++ )
    {
      dataIp[ i ] = atoi( ++ p );
      p = strchr( p, ',' );
    }
    // get port of data client
    dataPort = 256 * atoi( ++ p );
    p = strchr( p, ',' );
    dataPort += atoi( ++ p );
    if( p == NULL )
      client.println( "501 Can't interpret parameters");
    else
    {

                client.println("200 PORT command successful");
      dataPassiveConn = false;
    }
    */
  }
  //
  //  STRU - File Structure
  //
  else if (!strcmp(command, "STRU")) {
    if (!strcmp(parameters, "F"))
      client.println("200 F Ok");
    // else if( ! strcmp( parameters, "R" ))
    //  client.println( "200 B Ok\r\n";
    else
      client.println("504 Only F(ile) is suported");
  }
  //
  //  TYPE - Data Type
  //
  else if (!strcmp(command, "TYPE")) {
    if (!strcmp(parameters, "A"))
      client.println("200 TYPE is now ASII");
    else if (!strcmp(parameters, "I"))
      client.println("200 TYPE is now 8-bit binary");
    else
      client.println("504 Unknow TYPE");
  }

  ///////////////////////////////////////
  //                                   //
  //        FTP SERVICE COMMANDS       //
  //                                   //
  ///////////////////////////////////////

  //
  //  ABOR - Abort
  //
  else if (!strcmp(command, "ABOR")) {
    abortTransfer();
    client.println("226 Data connection closed");
#ifdef FTP_DEBUG
    Serial.println("226 Data connection closed");
#endif
  }
  //
  //  DELE - Delete a File
  //
  else if (!strcmp(command, "DELE")) {
    char path[FTP_CWD_SIZE];
    if (strlen(parameters) == 0)
      client.println("501 No file name");
    else if (makePath(path)) {
      if (!file.exists(path))
        client.println("550 File " + String(parameters) + " not found");
      else {
        if (file.remove(path))
          client.println("250 Deleted " + String(parameters));
        else
          client.println("450 Can't delete " + String(parameters));
      }
    }
  }
  //
  //  LIST - List
  //

  else if (!strcmp(command, "LIST")) {
    if (!dataConnect())
      client.println("425 No data connection");
    else {
      client.println("150 Accepted data connection");
      uint16_t nm = 0;
      myFile root;
      root.open(&cwdName[0]);
      if (!(root.isDirectory())) {
        client.println("550 Can't open directory " + String(cwdName));
      } else {
        struct dirent *de = root.openNextFile();
        while (de) {
          if (de->d_type == DT_DIR) {
            data.println("drwxr-xr-x 1 ftp ftp              0 Feb 1 11:11 " +
                         String(de->d_name));
            // Serial.print("  DIR : ");
            nm++;
            // Serial.println(file.name());
          } else {
            char path[FTP_CWD_SIZE];
            makePath(path, de->d_name);
            myFile temp;
            temp.open(path, "r");
            data.print("-r--r--r-- 1 ftp ftp\t" + String(temp.size()) +
                       " Sep 30  2017 ");
            data.println(String(de->d_name));
            temp.close();
            nm++;
          }

          de = root.openNextFile("r");
        }
        client.println("226-options: -a -l");
        client.println("226 " + String(nm) + " matches total");
      }
      data.stop();
    }
  }
  //
  //  MLSD - Listing for Machine Processing (see RFC 3659)
  //

  else if ((!strcmp(command, "MLSD"))) {
    if (!dataConnect())
      client.println("425 No data connection MLSD");
    else {
      client.println("150 Accepted data connection");
      uint16_t nm = 0;
      myFile root;
      root.open(&cwdName[0]);
      if (!(root.isDirectory())) {
        client.println("550 Can't open directory " + String(cwdName));
        // return;
      } else {
        // Serial.println(root.name());
        struct dirent *de = root.openNextFile();
        while (de) {
          if (de->d_type == DT_DIR) {

            data.println("type=dir;modify=20000101160656; " +
                         String(de->d_name));
            // Serial.print("  DIR : ");
            // Serial.println(file.name());
            nm++;
          } else {
            char path[FTP_CWD_SIZE];
            makePath(path, de->d_name);
            myFile temp;
            temp.open(path, "r");
            data.println("type=file;size=" + String(temp.size()) + ";" +
                         "modify=20000101160656; " + String(de->d_name));
            temp.close();
            nm++;
          }
          de = root.openNextFile();
        }
        client.println("226-options: -a -l");
        client.println("226 " + String(nm) + " matches total");
        // }

        data.stop();
      }
    }
  }
  //
  //  NLST - Name List
  //
  else if (!strcmp(command, "NLST")) {
    if (!dataConnect())
      client.println("425 No data connection");
    else {
      client.println("150 Accepted data connection");
      uint16_t nm = 0;

      myFile root;
      root.open(cwdName);
      if (!(root.isDirectory())) {
        client.println("550 Can't open directory " + String(cwdName));
        // return;
      } else {
        struct dirent *de = root.openNextFile();
        while (de) {
          if (de->d_type == DT_DIR) {
            data.println(String(de->d_name));
            // Serial.print("  DIR : ");
            nm++;
            // Serial.println(file.name());

          } else {
            data.println(String(de->d_name));
            nm++;
          }

          de = root.openNextFile("r");
        }
        //	client.println( "226-options: -a -l");
        client.println("226 " + String(nm) + " matches total");
      }
      data.stop();
    }
  }
  //
  //  NOOP
  //
  else if (!strcmp(command, "NOOP")) {
    // dataPort = 0;
    client.println("200 Ping...");
  }
  //
  //  RETR - Retrieve
  else if (!strcmp(command, "REST")) {
    // dataPort = 0;
    client.println("350 Rest supported. Restarting at " + String(parameters));
    seekPoint = atoll(parameters);
#ifdef FTP_DEBUG
    Serial.println("Rest supported. Restarting at " + String(parameters));
#endif
  }
  //
  else if (!strcmp(command, "RETR")) {
    char path[FTP_CWD_SIZE];
    if (strlen(parameters) == 0)
      client.println("501 No file name");
    else if (makePath(path)) {
      file.open(path, "rb");
      if (!file)
        client.println("550 File " + String(parameters) + " not found");

      else if (!dataConnect())
        client.println("425 No data connection");
      else {
#ifdef FTP_DEBUG
        Serial.println("Sending " + String(parameters));
#endif
        if (seekPoint && seekPoint < file.size()) {
          file.seek(seekPoint);
        } else
          seekPoint = 0;
        client.println("150-Connected to port " + String(dataPort));
        client.print("150 ");
        client.print(((long)(file.size()) - (long)seekPoint));
        client.println(" bytes to download");
        millisBeginTrans = millis();
        bytesTransfered = 0;
        transferStatus = 1;
        seekPoint = 0;
        filee = fileno(file._f);
        fcntl(filee, F_SETFL, fcntl(filee, F_GETFL) | O_NONBLOCK);
        // digitalWrite(LED_BUILTIN, 1);
      }
    }
  }
  //
  //  STOR - Store
  //
  else if (!strcmp(command, "STOR") || !strcmp(command, "APPE")) {
    char path[FTP_CWD_SIZE];
    if (strlen(parameters) == 0)
      client.println("501 No file name");
    else if (makePath(path)) {
      file.close();
      sprintf(buf, "%s%s", "/sdcard", path);
      if (!strcmp(command, "STOR"))
        filee = open(buf, O_WRONLY | O_CREAT | O_TRUNC | O_NONBLOCK);
      else
        filee = open(buf, O_WRONLY | O_NONBLOCK | O_APPEND);
      // file.open(path, "wb");
      if (filee == -1)
        client.println("451 Can't open/create " + String(parameters));
      else if (!dataConnect()) {
        client.println("425 No data connection");
        file.close();
      } else {
#ifdef FTP_DEBUG
        Serial.println("Receiving " + String(parameters));
#endif
        client.println("150 Connected to port " + String(dataPort));
        millisBeginTrans = millis();
        bytesTransfered = 0;
        transferStatus = 2;
        // digitalWrite(LED_BUILTIN, 1);
        // setvbuf(file._f,0,_IONBF,0);
        //  fcntl(filee, F_SETFL, fcntl(filee, F_GETFL) |_FSYNC | _FSYNC);
      }
    }
  }
  //
  //  MKD - Make Directory
  //
  else if (!strcmp(command, "MKD")) {
#ifdef FTP_DEBUG
    Serial.println("making dir " + String(parameters));
#endif
    char path[FTP_CWD_SIZE];
    if (strlen(parameters) == 0)
      client.println("550 Can't create");
    else if (makePath(path))
      if (SD.mkdir(path))
        client.println("257 " + String(path) + " created");
      else
        client.println("550 Can't create \"" +
                       String(path)); // not support on espyet
  }
  //
  //  RMD - Remove a Directory
  //
  else if (!strcmp(command, "RMD")) {
#ifdef FTP_DEBUG
    Serial.println("removing dir " + String(parameters));
#endif
    char path[FTP_CWD_SIZE];
    if (strlen(parameters) == 0)
      client.println("501 Can't delete /");
    else if (makePath(path))
      if (SD.rmdir(path))
        client.println("250 " + String(path) + " deleted");
      else
        client.println("501 Can't delete \"" + String(path));

  }
  //
  //  RNFR - Rename From
  //
  else if (!strcmp(command, "RNFR")) {
    // buf[ 0 ] = 0;
    if (strlen(parameters) == 0) {
      client.println("501 No file name");
    }

    else if (makePath(buf)) {
      if (!SD.exists(buf)) {
        client.println("550 File " + String(parameters) + " not found");
#ifdef FTP_DEBUG
        Serial.println("550 File " + String(buf) + " not found");
#endif
      }

      else {
#ifdef FTP_DEBUG
        Serial.println("Renaming " + String(buf));
#endif
        client.println(
            "350 RNFR accepted - file exists, ready for destination");
        rnfrCmd = true;
      }
    }
  }
  //
  //  RNTO - Rename To
  //
  else if (!strcmp(command, "RNTO")) {
    char path[FTP_CWD_SIZE];
    char dir[FTP_FIL_SIZE];
    if (strlen(buf) == 0 || !rnfrCmd)
      client.println("503 Need RNFR before RNTO");
    else if (strlen(parameters) == 0)
      client.println("501 No file name");
    else if (makePath(path)) {
      if (SD.exists(path))
        client.println("553 " + String(parameters) + " already exists");
      else {
#ifdef FTP_DEBUG
        Serial.println("Renaming " + String(buf) + " to " + String(path));
#endif
        if (SD.rename(buf, path))
          client.println("250 File successfully renamed or moved");
        else
          client.println("451 Rename/move failure");
      }
    }
    rnfrCmd = false;
  }

  ///////////////////////////////////////
  //                                   //
  //   EXTENSIONS COMMANDS (RFC 3659)  //
  //                                   //
  ///////////////////////////////////////

  //
  //  FEAT - New Features
  //
  else if (!strcmp(command, "FEAT")) {
    client.println("211-Extensions suported:");
    client.println(" MLSD");
    client.println(" SIZE");
    client.println(" ABOR");
    client.println(" REST");
    client.println(" UTF8");
    client.println("211 End.");
  }
  //
  //  MDTM - File Modification Time (see RFC 3659)
  //
  else if (!strcmp(command, "MDTM")) {
    client.println("502 Not Implemented");
  }

  //
  //  SIZE - Size of the file
  //
  else if (!strcmp(command, "SIZE")) {
#ifdef FTP_DEBUG
// Serial.println("Reached SIZE");
#endif
    char path[FTP_CWD_SIZE];
    if (strlen(parameters) == 0) {
      client.println("501 No file name");
#ifdef FTP_DEBUG
      Serial.println("501 No file name");
#endif
    } else if (makePath(path)) {
      if (!SD.exists(path)) {
        client.println("421 File " + String(parameters) + " does not exist");
#ifdef FTP_DEBUG
        Serial.println("421 File " + String(parameters) + " does not exist");
#endif
        client.stop();
        cmdStatus = 0;
      } else {
        SD.open(path, "r");
        if (!SD) {
          client.println("550 Can't open " + String(parameters));
#ifdef FTP_DEBUG
          client.println("550 Can't open " + String(parameters));
#endif
        } else {
          client.println("213 " + String(SD.size()));
          SD.close();
        }
      }
    }
  }
  //
  //  SITE - System command
  //
  else if (!strcmp(command, "SITE")) {
    client.println("500 Unknow SITE command " + String(parameters));
  }
  //
  //  Unrecognized commands ...
  //
  else
    client.println("500 Unknow command");

  return true;
}

boolean FtpServer::dataConnect() {
  unsigned long startTime = millis();
  // wait 5 seconds for a data connection
  if (!data || !data.connected()) {
    data.stop();
    while (!dataServer.hasClient() && millis() - startTime < 5000) {
      delay(100);
      yield();
    }
    if (dataServer.hasClient()) {

      data = dataServer.available();
#ifdef FTP_DEBUG
      Serial.println("data channel connected....");
#endif
    }
  }

  return data.connected();
}

boolean FtpServer::doRetrieve() {
#ifdef FTP_DEBUG
  u_long time1 = millis(), time2, bytesLast = 0;

#endif
  while (data.connected()) {
    int32_t nb = read(fileno(file._f), buf, (size_t)FTP_BUF_SIZE);
    if (nb > 0) {
      // delay(0);
      nb = data.write((uint8_t *)buf, (size_t)nb);
#ifdef FTP_DEBUG
      if (nb <= 0)
        Serial.println("Socket err");
#endif

      abortSecondClient();
      bytesTransfered += nb;
#ifdef FTP_DEBUG
      if (millis() >= time1 + 1000) {
        time2 = millis();
        Serial.printf("\rTransfer Speed : %.3f KB/s",
                      (bytesTransfered - bytesLast) * 1000 /
                          (float)(time2 - time1) / 1024.0);
        time1 = time2;
        bytesLast = bytesTransfered;
      }
#endif
      // return true;
    } else
      break;
  }
  millisEndConnection = millis() + millisTimeOut;
  closeTransfer();
#ifdef FTP_DEBUG
  Serial.println("\r\n Transfer Completed(or stopped)");
#endif
  // digitalWrite(LED_BUILTIN, 0);
  return false;
}

boolean FtpServer::doStore() {
  // Avoid blocking by never reading more bytes than are available
  int16_t nb;
  long timeout;
  long pos = 0;
  while (data && data.connected()) {
    int32_t nb = data.available();
    if (FTP_BUF_SIZE < nb + pos)
      nb = FTP_BUF_SIZE - pos;
    delay(1);
    if (nb > 0) {
      nb = data.read((uint8_t *)&buf[pos], nb);
      if (nb > 0) {
        pos += nb;
        if (pos >= FTP_BUF_SIZE) {
          // file.wwrite((uint8_t*)buf, pos);
          // fflush(file._f);
          write(filee, buf, (size_t)pos);

          // fsync(filee);
          // fwrite(&buf[0],(size_t)1,(size_t)pos,file._f);
          // fflush(file._f);
          // fflush(filee);
          pos = 0;
        }
        // abortSecondClient();
        bytesTransfered += nb;
      }
      // return true;
    } // else break;
  }
  delay(10);
  if (pos)
    write(filee, buf, (size_t)pos);
  // fsync(filee);
  // fwrite(&buf[0],(size_t)1,(size_t)pos,file._f);
  // fflush(file._f);
  // fflush(filee);
  delay(10);
  close(filee);
  millisEndConnection = millis() + millisTimeOut;
  closeTransfer();
  // digitalWrite(LED_BUILTIN, 0);
  return false;
}

void FtpServer::closeTransfer() {
  uint32_t deltaT = (int32_t)(millis() - millisBeginTrans);
  if (deltaT > 0 && bytesTransfered > 0) {
    client.println("226-File successfully transferred");
    client.println("226 " + String(deltaT) + " ms, " +
                   String(bytesTransfered / deltaT) + " kbytes/s");
  } else
    client.println("226 File successfully transferred");

  if (file)
    file.close();
  if (data)
    data.stop();
}

void FtpServer::abortTransfer() {
  if (transferStatus > 0) {
    if (file)
      file.close();
    if (data)
      data.stop();
    client.println("426 Transfer aborted");
#ifdef FTP_DEBUG
    Serial.println("Transfer aborted!");
#endif
  }
  transferStatus = 0;
}

// Read a char from client connected to ftp server
//
//  update cmdLine and command buffers, CMDlen and parameters pointers
//
//  return:
//    -2 if buffer cmdLine is full
//    -1 if line not completed
//     0 if empty line received
//    length of cmdLine (positive) if no empty line received

int16_t FtpServer::readChar() {
  int16_t rc = -1;

  if (client.available()) {
    char c = client.read();

#ifdef FTP_DEBUG
    Serial.print(c);
#endif
    // if( c == '\\' )
    //    c = '/';
    if (c != '\r')
      if (c != '\n') {
        if (CMDlen < FTP_CMD_SIZE)
          cmdLine[CMDlen++] = c;
        else {
          rc = -2;
#ifdef FTP_DEBUG
          Serial.print("Line too long");
#endif
        } //  Line too long
      } else {
        cmdLine[CMDlen] = 0;
        command[0] = 0;
        parameters = NULL;
        // empty line?
        if (CMDlen == 0) {
          Serial.println("empty line?");
          rc = 0;
        }

        else {
          rc = CMDlen;
          // search for space between command and parameters
          parameters = strchr(cmdLine, ' ');
          if (parameters != NULL) {
            if (parameters - cmdLine > 4) {
              rc = -2; // Syntax error
#ifdef FTP_DEBUG
              Serial.print("Syntax error");
#endif

            }

            else {
              strncpy(command, cmdLine, parameters - cmdLine);
              command[parameters - cmdLine] = 0;

              while (*(++parameters) == ' ')
                ;
#ifdef FTP_DEBUG
// Serial.println(String("Kom0 :")+String(command)+" "+String(parameters));

#endif
            }
          } else if (strlen(cmdLine) > 4) {
            strcpy(command, cmdLine + strlen(cmdLine) - 4);
          }

          else {
            strcpy(command, cmdLine);
#ifdef FTP_DEBUG
// Serial.print( String("Kom :")+String(command));
#endif
          }

          CMDlen = 0;
        }
      }
    if (rc > 0) {
      for (uint8_t i = 0; i < strlen(command); i++)
        command[i] = toupper(command[i]);
#ifdef FTP_DEBUG
      Serial.println(String("Command :") + String(command) + " " +
                     String(parameters));
#endif
    }

    if (rc == -2) {
      CMDlen = 0;
      client.println("500 Syntax error");
    }
  }
  return rc;
}

// Make complete path/name from cwdName and parameters
//
// 3 possible cases: parameters can be absolute path, relative path or only the
// name
//
// parameters:
//   fullName : where to store the path/name
//
// return:
//    true, if done

boolean FtpServer::makePath(char *fullName) {
  return makePath(fullName, parameters);
}

boolean FtpServer::makePath(char *fullName, char *param) {
  if (param == NULL)
    param = parameters;

  // Root or empty?
  if (strcmp(param, "/") == 0 || strlen(param) == 0) {
    strcpy(fullName, "/");
    return true;
  }
  // If relative path, concatenate with current dir
  if (param[0] != '/') {
    strcpy(fullName, cwdName);
    if (fullName[strlen(fullName) - 1] != '/')
      strncat(fullName, "/", FTP_CWD_SIZE);
    strncat(fullName, param, FTP_CWD_SIZE);
  } else
    strcpy(fullName, param);
  // If ends with '/', remove it
  uint16_t strl = strlen(fullName) - 1;
  if (fullName[strl] == '/' && strl > 1)
    fullName[strl] = 0;
#ifdef FTP_DEBUG
//	Serial.println( "FULLPATH : "+String(fullName));
#endif
  if (strlen(fullName) < FTP_CWD_SIZE)
    return true;
#ifdef FTP_DEBUG
  Serial.println("500 Command line too long");
#endif
  client.println("500 Command line too long");
  return false;
}
