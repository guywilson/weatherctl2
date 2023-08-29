#include "version.h"

#define __BDATE__      "2023-08-29 21:39:19"
#define __BVERSION__   "1.3.004"

const char * getVersion(void)
{
    return __BVERSION__;
}

const char * getBuildDate(void)
{
    return __BDATE__;
}
