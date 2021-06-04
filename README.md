## PHP GRIB2

PHP extension to decode weather grib files

- `phpize`
- `./configure`
- `make`
- `php -dextension=modules/grib2.so -r "var_dump(php_grib2('gfs.t12z.pgrb2.0p25.f000'));"`