#include "version.h"

#define __BDATE__      "2025-02-03 11:07:30"
#define __BVERSION__   "1.4.006"

const char * getVersion(void)
{
    return __BVERSION__;
}

const char * getBuildDate(void)
{
    return __BDATE__;
}
