// we define Module constants
#define PHP_GRIB2_EXTNAME "grib2"
#define PHP_GRIB2_VERSION "0.0.2"

struct property {
    char* name;
    float value;
};



// then we declare the function to be exported
PHP_FUNCTION(read_grib2_file);
PHP_FUNCTION(grib2_file_to_geojson);
