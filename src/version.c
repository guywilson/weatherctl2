#include "version.h"

#define __BDATE__      "2023-08-29 22:07:40"
#define __BVERSION__   "1.3.005"

const char * getVersion(void)
{
    return __BVERSION__;
}

const char * getBuildDate(void)
{
    return __BDATE__;
}
