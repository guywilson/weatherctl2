#include "version.h"

#define __BDATE__      "2023-08-25 13:12:19"
#define __BVERSION__   "1.2.003"

const char * getVersion(void)
{
    return __BVERSION__;
}

const char * getBuildDate(void)
{
    return __BDATE__;
}
