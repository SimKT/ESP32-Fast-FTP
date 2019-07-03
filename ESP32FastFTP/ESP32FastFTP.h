
/*
 * Fast FTP SERVER FOR ESP32
 * based on FTP Server for Arduino Due and Ethernet shield (W5100) or WIZ820io (W5200)
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



#ifndef FTP_SERVERESP32_H
#define FTP_SERVERESP32_H


#include "myFile.h" //helper class
#include <WiFiClient.h>


#define FTP_SERVER_VERSION "ESP32FastFTP"

#define FTP_CTRL_PORT    88       // Command port on wich server is listening
#define FTP_DATA_PORT_PASV 50009 // Data port in passive mode

#define FTP_TIME_OUT  4       // Disconnect client after secs of inactivity
#define FTP_CMD_SIZE 1255 + 8 // max size of a command
#define FTP_CWD_SIZE 1255 + 8 // max size of a directory name
#define FTP_FIL_SIZE 1255     // max size of a file name
#define FTP_BUF_SIZE 4*4096   // size of file buffer for read/write, best is default

#define FTP_DEBUG 1 //

class FtpServer
{
public:
  void    beginFTP(String Username, String Password, uint16_t port=FTP_CTRL_PORT, uint16_t dataPort=FTP_DATA_PORT_PASV);
  void    handleFTP();
  void    initSD();
  void    initSD_MMC();
  void    configPassiveIP(IPAddress cfg);

private:
  void    iniVariables();
  void    clientConnected();
  void    disconnectClient();
  boolean userIdentity();
  boolean userPassword();
  boolean processCommand();
  boolean dataConnect();
  boolean doRetrieve();
  boolean doStore();
  void    closeTransfer();
  void    abortTransfer();
  void    abortSecondClient();
  boolean makePath( char * fullname );
  boolean makePath( char * fullName, char * param );
  uint8_t getDateTime( uint16_t * pyear, uint8_t * pmonth, uint8_t * pday,
                       uint8_t * phour, uint8_t * pminute, uint8_t * second );
  char *  makeDateTimeStr( char * tstr, uint16_t date, uint16_t time );
  int16_t  readChar();

  IPAddress  dataIp;              // IP address of client for data
  WiFiClient client, tempClient;
  WiFiClient data;

  myFile     file,SD;//helper libraries

  boolean  dataPassiveConn;
  uint16_t dataPort;
  uint16_t Port;
     // data buffer for transfers
  char     cmdLine[ FTP_CMD_SIZE ];   // incomming command
  char     cwdName[ FTP_CWD_SIZE ];   // name of current directory
  char     command[ 5 ];              // command sent by client
  boolean  rnfrCmd;                   // previous command was RNFR
  char *   parameters;                // point to start of parameters
  uint16_t CMDlen;                       // pointer to cmdLine next char
  int8_t   cmdStatus=-2,                 // status of ftp command connection
           transferStatus=0;            // status of ftp data transfer
  uint32_t millisTimeOut,             // disconnect after 5 min of inactivity
           millisDelay,
           millisEndConnection,       //
           millisBeginTrans,          // store time of beginning of a transaction
           bytesTransfered;           //
  String   _FTP_USER;
  String   _FTP_PASS;
  long long unsigned int seekPoint=0;
  unsigned long bufferPointer=0;
  bool customDataIP=false;
  IPAddress dataIP,cdataIP;




};

#endif
