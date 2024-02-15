#include "version.h"

#define __BDATE__      "2024-02-15 22:11:02"
#define __BVERSION__   "1.4.004"

const char * getVersion(void)
{
    return __BVERSION__;
}

const char * getBuildDate(void)
{
    return __BDATE__;
}
