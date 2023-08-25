#include "version.h"

#define __BDATE__      "2023-08-25 13:44:04"
#define __BVERSION__   "1.2.004"

const char * getVersion(void)
{
    return __BVERSION__;
}

const char * getBuildDate(void)
{
    return __BDATE__;
}
