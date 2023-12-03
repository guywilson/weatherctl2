#include "version.h"

#define __BDATE__      "2023-12-03 12:37:21"
#define __BVERSION__   "1.4.001"

const char * getVersion(void)
{
    return __BVERSION__;
}

const char * getBuildDate(void)
{
    return __BDATE__;
}
