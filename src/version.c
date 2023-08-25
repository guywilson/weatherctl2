#include "version.h"

#define __BDATE__      "2023-08-25 14:27:08"
#define __BVERSION__   "1.2.005"

const char * getVersion(void)
{
    return __BVERSION__;
}

const char * getBuildDate(void)
{
    return __BDATE__;
}
