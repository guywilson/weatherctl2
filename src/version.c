#include "version.h"

#define __BDATE__      "2023-12-04 10:23:43"
#define __BVERSION__   "1.4.003"

const char * getVersion(void)
{
    return __BVERSION__;
}

const char * getBuildDate(void)
{
    return __BDATE__;
}
