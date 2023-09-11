#include "version.h"

#define __BDATE__      "2023-09-11 17:50:54"
#define __BVERSION__   "1.3.006"

const char * getVersion(void)
{
    return __BVERSION__;
}

const char * getBuildDate(void)
{
    return __BDATE__;
}
