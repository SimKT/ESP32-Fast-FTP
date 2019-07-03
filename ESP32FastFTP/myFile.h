
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

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include <dirent.h>
class myFile
{

		protected:


		    DIR *               _d;
		    char *              _path;
		    bool                _isDirectory;
		    mutable struct stat _stat;
		    mutable bool        _written;



		public:

				FILE *      _f;
				myFile()
				{
				    strcpy(_mountpoint,"/sdcard");_f=0;_d=0;_path=0;_isDirectory=0;_written=0;

				};
				FILE*  			open( const char* path);
		    FILE* 			open(const char* path, const char* mode);
		    bool        exists(const char* path) ;
		    bool        rename(const char* pathFrom, const char* pathTo) ;
		    bool        remove(const char* path) ;
		    bool        mkdir(const char *path) ;
		    bool        rmdir(const char *path) ;
		    char _mountpoint[64];

		    ~myFile() {
				    close();
				};
		    size_t      wwrite(const uint8_t *buf, size_t size) ;
		    size_t      rread(uint8_t* buf, size_t size) ;
		    void        flush() ;
		    bool        seek(uint32_t pos) ;
		    size_t      position() const ;
		    size_t      size() const ;
		    void        close() ;
		    const char* name() const ;
		    time_t 			getLastWrite()  ;
		    bool     		isDirectory(void) ;
		    struct dirent *				openNextFile(const char* mode) ;
				struct dirent *				openNextFile() ;
		    void        rewindDirectory(void) ;
		    operator    bool();

				void 				_getStat() const;
};
