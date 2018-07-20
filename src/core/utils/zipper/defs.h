#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <unistd.h>
#include <utime.h>

#include "minizip/zip.h"
#include "minizip/unzip.h"
#include "minizip/ioapi_mem.h"

#define CASESENSITIVITY (0)
#define WRITEBUFFERSIZE (8192)
#define MAXFILENAME (256)
#define EXCEPTION_CLASS runtime_error
#define MKDIR(d) mkdir(d, 0775)
#define CHDIR(d) chdir(d)

