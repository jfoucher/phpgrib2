## PHP GRIB2 Extension

PHP extension to decode weather grib2 files

- `phpize`
- `./configure`
- `make`
- `php -dextension=modules/grib2.so -r "var_dump(php_grib2('gfs.t12z.pgrb2.0p25.f000'));"`

Handles only Latitude/longitude coordinates (https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_table3-1.shtml)

