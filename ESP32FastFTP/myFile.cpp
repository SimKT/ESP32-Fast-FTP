
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

#include "myFile.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include <dirent.h>




bool myFile::exists(const char* path)
{
    if(!_mountpoint) {
        return false;
    }


    if(open( path, "r")) {
        close();
        return true;
    }
    return false;
}

bool myFile::rename(const char* pathFrom, const char* pathTo)
{
    if(!_mountpoint) {
        return false;
    }

    if(!pathFrom || pathFrom[0] != '/' || !pathTo || pathTo[0] != '/') {
        return false;
    }
    if(!exists(pathFrom)) {
        return false;
    }
    char * temp1 = (char *)malloc(strlen(pathFrom)+strlen(_mountpoint)+1);
    if(!temp1) {
        return false;
    }
    char * temp2 = (char *)malloc(strlen(pathTo)+strlen(_mountpoint)+1);
    if(!temp2) {
        free(temp1);
        return false;
    }
    sprintf(temp1,"%s%s", _mountpoint, pathFrom);
    sprintf(temp2,"%s%s", _mountpoint, pathTo);
    auto rc = ::rename(temp1, temp2);
    free(temp1);
    free(temp2);
    return rc == 0;
}

bool myFile::remove(const char* path)
{
    if(!_mountpoint) {
        return false;
    }

    if(!path || path[0] != '/') {
        return false;
    }
		close();
    open(path, "r");
    if(!_f || _isDirectory) {
        if(_f) {


            close();
        }
        return false;
    }
    close();

    char * temp = (char *)malloc(strlen(path)+strlen(_mountpoint)+1);
    if(!temp) {
        return false;
    }
    sprintf(temp,"%s%s", _mountpoint, path);
    auto rc = unlink(temp);
    free(temp);
    return rc == 0;
}

bool myFile::mkdir(const char *path)
{
    if(!_mountpoint) {
        return false;
    }
		close();
   	open(path, "r");
    if(_d && _isDirectory) {
        close();
        return true;
    } else if(_f) {
        close();
        return false;
    }

    char * temp = (char *)malloc(strlen(path)+strlen(_mountpoint)+1);
    if(!temp) {
        return false;
    }
    sprintf(temp,"%s%s", _mountpoint, path);
    auto rc = ::mkdir(temp, ACCESSPERMS);
    free(temp);
    return rc == 0;
}

bool myFile::rmdir(const char *path)
{
    if(!_mountpoint) {
        return false;
    }
		close();
    open(path, "r");
    if(!isDirectory()) {
        close();
        return false;
    }
    close();
    char * temp = (char *)malloc(strlen(path)+strlen(_mountpoint)+1);
    if(!temp) {
        return false;
    }
    sprintf(temp,"%s%s", _mountpoint, path);
    auto rc = unlink(temp);
    free(temp);
    return rc == 0;
}
FILE* myFile::open( const char* path)
{
    return open(path,"r");
}

FILE* myFile::open( const char* path, const char* mode)
{
		close();
		_isDirectory=false;
    _written=false;
		char * temp = (char *)malloc(strlen(path)+strlen(_mountpoint)+1);
    if(!temp) {
        return 0;
    }
    sprintf(temp,"%s%s", _mountpoint, path);

    _path = strdup(path);
    if(!_path) {
        free(temp);
        return 0;
    }

    if(!stat(temp, &_stat)) {
        //file found
        if (S_ISREG(_stat.st_mode)) {
            _isDirectory = false;
            _f = fopen(temp, mode);
            if(!_f) {
            }
        } else if(S_ISDIR(_stat.st_mode)) {
            _isDirectory = true;
            _d = opendir(temp);
            if(!_d) {
                //TODO
            }
        } else {
            //TODO
        }
    } else {
        //file not found
        if(!mode || mode[0] == 'r') {
            //try to open as directory
            _d = opendir(temp);
            if(_d) {
                _isDirectory = true;
            } else {
                _isDirectory = false;

            }
        } else {
            //lets create this new file
            _isDirectory = false;
            _f = fopen(temp, mode);
            if(!_f) {
                //TODO
            }
        }
    }
    free(temp);
    if(_isDirectory)return (FILE*)_d;
    return _f;
}






void myFile::close()
{
    if(_path) {
        free(_path);
        _path = NULL;
    }
    if(_isDirectory && _d) {
        closedir(_d);
        _d = NULL;
        _isDirectory = false;
    } else if(_f) {
        fclose(_f);
        _f = NULL;
    }
}

myFile::operator bool()
{
    return (_isDirectory && _d != NULL) || _f != NULL;
}

time_t myFile::getLastWrite() {
    _getStat() ;
    return _stat.st_mtime;
}

void myFile::_getStat() const
{
    if(!_path) {
        return;
    }
    char * temp = (char *)malloc(strlen(_path)+strlen(_mountpoint)+1);
    if(!temp) {
        return;
    }
    sprintf(temp,"%s%s", _mountpoint, _path);
    if(!stat(temp, &_stat)) {
        _written = false;
    }
    free(temp);
}

size_t myFile::wwrite(const uint8_t *buf, size_t size)
{
    //if(_isDirectory || !_f || !buf || !size) {
    //    return 0;
    //}
    _written = true;
    return write(fileno(_f),buf,size );
    //return fwrite(buf, 1, size, _f);
}

size_t myFile::rread(uint8_t* buf, size_t size)
{
   // if(_isDirectory || !_f || !buf || !size) {
  //      return 0;
   // }
		return read(fileno(_f),buf,size );
   // return fread(buf, 1, size, _f);
}

void myFile::flush()
{
    if(_isDirectory || !_f) {
        return;
    }
   // fflush(_f);
}

bool myFile::seek(uint32_t pos)
{
    if(_isDirectory || !_f) {
        return false;
    }
    auto rc = fseek(_f, pos, 0);
    return rc == 0;
}

size_t myFile::position() const
{
    if(_isDirectory || !_f) {
        return 0;
    }
    return ftell(_f);
}

size_t myFile::size() const
{
    if(_isDirectory || !_f) {
        return 0;
    }
    if (_written) {
        _getStat();
    }
    return _stat.st_size;
}

const char* myFile::name() const
{
    return (const char*) _path;
}

//to implement
bool myFile::isDirectory(void)
{
    return _isDirectory;
}
struct dirent * myFile::openNextFile()
{
  return openNextFile("r");
}
struct dirent * myFile::openNextFile(const char* mode)
{
    if(!_isDirectory || !_d) {
        return 0;
    }
    struct dirent *file = readdir(_d);
    if(file == NULL) {
    		//closedir(_d);
    		//close();
        return 0;
    }
    if(file->d_type != DT_REG && file->d_type != DT_DIR) {
        return openNextFile(mode);
    }
    String fname = String(file->d_name);
    String name = String(_path);
    if(!fname.startsWith("/") && !name.endsWith("/")) {
        name += "/";
    }
    //if(file->d_type == DT_DIR)_isDirectory=1;

    return file;
}

void myFile::rewindDirectory(void)
{
    if(!_isDirectory || !_d) {
        return;
    }
    rewinddir(_d);
}
