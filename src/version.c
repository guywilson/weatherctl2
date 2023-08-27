#include "version.h"

#define __BDATE__      "2023-08-27 15:10:18"
#define __BVERSION__   "1.2.007"

const char * getVersion(void)
{
    return __BVERSION__;
}

const char * getBuildDate(void)
{
    return __BDATE__;
}
