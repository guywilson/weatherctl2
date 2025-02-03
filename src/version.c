#include "version.h"

#define __BDATE__      "2025-02-03 10:06:34"
#define __BVERSION__   "1.4.005"

const char * getVersion(void)
{
    return __BVERSION__;
}

const char * getBuildDate(void)
{
    return __BDATE__;
}
