#include "version.h"

#define __BDATE__      "2023-07-18 16:14:11"
#define __BVERSION__   "1.1.005"

const char * getVersion(void)
{
    return __BVERSION__;
}

const char * getBuildDate(void)
{
    return __BDATE__;
}
