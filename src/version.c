#include "version.h"

#define __BDATE__      "2023-08-29 13:32:57"
#define __BVERSION__   "1.3.002"

const char * getVersion(void)
{
    return __BVERSION__;
}

const char * getBuildDate(void)
{
    return __BDATE__;
}
