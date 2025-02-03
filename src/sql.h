#include <stdbool.h>

#ifndef __INCL_SQL
#define __INCL_SQL

typedef struct {
    char            created[20];

    float           min_temperature;
    float           max_temperature;
    
    float           min_pressure;
    float           max_pressure;
    
    float           min_humidity;
    float           max_humidity;
    
    float           total_rainfall;
    uint32_t        isHourAccountedBitmap;
    
    float           max_wind_speed;
    float           max_wind_gust;
}
daily_summary_t;


const char * pszWeatherInsertStmt = 
"INSERT INTO weather_data (\
created, \
packet_num, \
temperature, \
dew_point, \
actual_pressure, \
pressure, \
humidity, \
rainfall, \
wind_speed, \
wind_gust) \
values (\
'%s', \
%d, \
%.2f, \
%.2f, \
%.2f, \
%.2f, \
%.2f, \
%.2f, \
%.2f, \
%.2f);";

const char * pszTelemetryInsertStmt = 
"INSERT INTO telemetry_data (\
created, \
packet_num, \
battery_voltage, \
battery_percentage, \
battery_crate, \
status_bits) \
values (\
'%s', \
%d, \
%.2f, \
%.2f, \
%.2f, \
%d);";

const char * pszSummaryInsertStmt = 
"INSERT INTO daily_summary (\
created, \
min_temperature, \
max_temperature, \
min_pressure, \
max_pressure, \
min_humidity, \
max_humidity, \
total_rainfall, \
max_wind_speed, \
max_wind_gust) \
values (\
'%s', \
%.2f, \
%.2f, \
%.2f, \
%.2f, \
%.2f, \
%.2f, \
%.2f, \
%.2f, \
%.2f);";

#endif
