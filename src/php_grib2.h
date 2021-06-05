#ifdef __cplusplus
extern "C" {
#endif

// we define Module constants
#define PHP_GRIB2_EXTNAME "grib2"
#define PHP_GRIB2_VERSION "0.0.1"

// then we declare the function to be exported
PHP_FUNCTION(read_grib2_file);

#ifdef __cplusplus
}
#endif
