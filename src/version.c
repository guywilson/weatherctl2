#include "version.h"

#define __BDATE__      "2023-08-27 21:36:16"
#define __BVERSION__   "1.2.009"

const char * getVersion(void)
{
    return __BVERSION__;
}

const char * getBuildDate(void)
{
    return __BDATE__;
}
