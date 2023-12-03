#include "version.h"

#define __BDATE__      "2023-12-03 13:49:42"
#define __BVERSION__   "1.4.002"

const char * getVersion(void)
{
    return __BVERSION__;
}

const char * getBuildDate(void)
{
    return __BDATE__;
}
