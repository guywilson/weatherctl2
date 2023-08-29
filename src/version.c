#include "version.h"

#define __BDATE__      "2023-08-29 10:38:15"
#define __BVERSION__   "1.3.001"

const char * getVersion(void)
{
    return __BVERSION__;
}

const char * getBuildDate(void)
{
    return __BDATE__;
}
