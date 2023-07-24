#include "version.h"

#define __BDATE__      "2023-07-24 17:08:48"
#define __BVERSION__   "1.2.002"

const char * getVersion(void)
{
    return __BVERSION__;
}

const char * getBuildDate(void)
{
    return __BDATE__;
}
