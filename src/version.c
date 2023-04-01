#include "version.h"

#define __BDATE__      "2023-04-01 15:27:12"
#define __BVERSION__   "0.1.003"

const char * getVersion(void)
{
    return __BVERSION__;
}

const char * getBuildDate(void)
{
    return __BDATE__;
}
