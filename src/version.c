#include "version.h"

#define __BDATE__      "2025-06-14 11:46:09"
#define __BVERSION__   "1.5.005"

const char * getVersion(void)
{
    return __BVERSION__;
}

const char * getBuildDate(void)
{
    return __BDATE__;
}
