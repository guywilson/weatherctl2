#include "version.h"

#define __BDATE__      "2023-08-27 17:11:19"
#define __BVERSION__   "1.2.008"

const char * getVersion(void)
{
    return __BVERSION__;
}

const char * getBuildDate(void)
{
    return __BDATE__;
}
