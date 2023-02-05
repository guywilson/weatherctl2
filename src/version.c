#include "version.h"

#define __BDATE__      "2023-02-05 14:21:08"
#define __BVERSION__   "0.1.003"

const char * getVersion()
{
    return __BVERSION__;
}

const char * getBuildDate()
{
    return __BDATE__;
}
