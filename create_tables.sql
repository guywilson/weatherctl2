CREATE TABLE weather_data (
    id SERIAL PRIMARY KEY,
    created TIMESTAMP NOT NULL,
    packet_num INTEGER,
    temperature NUMERIC(5,2),
    dew_point NUMERIC(5,2),
    actual_pressure NUMERIC(6,2),
    pressure NUMERIC(6,2),
    humidity NUMERIC(5,2),
    lux NUMERIC(8,2),
    uv_index NUMERIC(5,2),
    rainfall NUMERIC(7,2),
    wind_speed NUMERIC(5,2),
    wind_gust NUMERIC(5,2)
);

CREATE TABLE telemetry_data (
    id SERIAL PRIMARY KEY,
    created TIMESTAMP NOT NULL,
    packet_num INTEGER,
    battery_voltage NUMERIC(3,2),
    battery_percentage NUMERIC(5,2),
    battery_crate NUMERIC(5,2),
    status_bits INTEGER
);

CREATE TABLE daily_summary (
    id SERIAL PRIMARY KEY,
    created DATE NOT NULL,
    min_temperature NUMERIC(5,2),
    max_temperature NUMERIC(5,2),
    min_pressure NUMERIC(6,2),
    max_pressure NUMERIC(6,2),
    min_humidity NUMERIC(5,2),
    max_humidity NUMERIC(5,2),
    max_lux NUMERIC(8,2),
    total_rainfall NUMERIC(7,2),
    max_wind_speed NUMERIC(5,2),
    max_wind_gust NUMERIC(5,2)
);
