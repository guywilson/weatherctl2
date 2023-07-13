#include "version.h"

#define __BDATE__      "2023-07-13 14:33:15"
#define __BVERSION__   "1.0.004"

const char * getVersion(void)
{
    return __BVERSION__;
}

const char * getBuildDate(void)
{
    return __BDATE__;
}
