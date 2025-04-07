#include "version.h"

#define __BDATE__      "2025-04-07 11:22:27"
#define __BVERSION__   "1.5.002"

const char * getVersion(void)
{
    return __BVERSION__;
}

const char * getBuildDate(void)
{
    return __BDATE__;
}
