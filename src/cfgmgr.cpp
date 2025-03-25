#include <iostream>
#include <fstream>
#include <string>

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <strutils.h>

#include "cfgmgr.h"

using namespace std;

static bool inline isStringHexadecimal(string & value) {
    if (value[0] == '0' && (value[1] == 'x' || value[1] == 'X')) {
        return true;
    }

    return false;
}

static const char * getHexadecimalValue(string & value) {
    static string hexValue = value.substr(2);
    return hexValue.c_str();
}

static const char * getDecimalValue(string & value) {
    return value.c_str();
}

void cfgmgr::initialise(const string & configFileName) {
    ifstream ifs;

    try {
        ifs.open(configFileName.c_str());
    }
    catch (exception & e) {
        throw cfg_error(cfg_error::buildMsg("Failed to open config file '%s'", configFileName.c_str()));
    }

    string line;

    while (getline(ifs, line)) {
        if (line.front() == '#' || line.length() == 0) {
            continue;
        }

        uint32_t equalPos = line.find_first_of('=');

        string key = line.substr(0, equalPos);
        string value = line.substr(equalPos + 1);

        this->values[key] = value;
    }

    isConfigured = true;

    ifs.close();
}

string cfgmgr::getValue(const string & key) {
    if (values.count(key) == 0) {
        return "";
    }

    return values[key];
}

bool cfgmgr::getValueAsBoolean(const string & key) {
    string value = getValue(key);

    return ((value.compare("yes") == 0  || value.compare("true") == 0 || value.compare("on") == 0) ? true : false);
}

int cfgmgr::getValueAsInteger(const string & key) {
    string value = getValue(key);

    return atoi(getDecimalValue(value));
}

int32_t cfgmgr::getValueAsLongInteger(const string & key) {
    string value = getValue(key);

    if (isStringHexadecimal(value)) {
        return strtol(getHexadecimalValue(value), NULL, 16);
    }
    else {
        return strtol(getDecimalValue(value), NULL, 10);
    }
}

uint32_t cfgmgr::getValueAsLongUnsignedInteger(const string & key) {
    string value = getValue(key);

    if (isStringHexadecimal(value)) {
        return strtoul(getHexadecimalValue(value), NULL, 16);
    }
    else {
        return strtoul(getDecimalValue(value), NULL, 10);
    }
}

double cfgmgr::getValueAsDouble(const string & key) {
    string value = getValue(key);

    return strtod(getDecimalValue(value), NULL);
}

void cfgmgr::dumpConfig() {
    if (isConfigured) {
        for (auto& i : values) {
            string key = i.first;
            string value = i.second;

            cout << "'" << key << "' = '" << value << "'" << endl;
        }
    }
}
