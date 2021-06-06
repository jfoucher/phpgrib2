// include the PHP API itself
#include <php.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <zend_exceptions.h>

#include "../grib2c/grib2.h"
#include "../grib2c/gridtemplates.c" 
#include "../grib2c/drstemplates.c" 
#include "../grib2c/pdstemplates.c" 
#include "../grib2c/gbits.c" 
#include "../grib2c/g2_unpack1.c" 
#include "../grib2c/g2_unpack2.c" 
#include "../grib2c/g2_unpack3.c" 
#include "../grib2c/g2_unpack4.c" 
#include "../grib2c/g2_unpack5.c" 
#include "../grib2c/g2_unpack6.c" 
#include "../grib2c/g2_unpack7.c" 
#include "../grib2c/g2_free.c" 
#include "../grib2c/g2_info.c" 
#include "../grib2c/g2_getfld.c" 
#include "../grib2c/simunpack.c" 
#include "../grib2c/comunpack.c" 
#include "../grib2c/pack_gp.c" 
#include "../grib2c/reduce.c" 
#include "../grib2c/specpack.c" 
#include "../grib2c/specunpack.c" 
#include "../grib2c/rdieee.c" 
#include "../grib2c/mkieee.c" 
#include "../grib2c/int_power.c" 
#include "../grib2c/simpack.c" 
#include "../grib2c/compack.c" 
#include "../grib2c/cmplxpack.c" 
#include "../grib2c/misspack.c"
#include "../grib2c/g2_create.c" 
#include "../grib2c/g2_addlocal.c" 
#include "../grib2c/g2_addgrid.c" 
#include "../grib2c/g2_addfield.c" 
#include "../grib2c/g2_gribend.c" 
#include "../grib2c/getdim.c" 
#include "../grib2c/g2_miss.c" 
#include "../grib2c/getpoly.c" 
#include "../grib2c/seekgb.c"

#include "tables.h"


// then include the header of your extension
#include "php_grib2.h"

// Run "make" to build extension, they test it like this : 
// php -dextension=modules/grib2.so -r "var_dump(php_grib2('gfs.t12z.pgrb2.0p25.f000'));"
zend_function_entry grib2_functions[] = {
    PHP_FE(read_grib2_file, NULL)
    PHP_FE(grib2_file_to_geojson, NULL)
    PHP_FE_END
};

// some pieces of information about our module
zend_module_entry grib2_module_entry = {
    STANDARD_MODULE_HEADER,
    PHP_GRIB2_EXTNAME,
    grib2_functions,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    PHP_GRIB2_VERSION,
    STANDARD_MODULE_PROPERTIES
};

// use a macro to output additional C code, to make ext dynamically loadable
ZEND_GET_MODULE(grib2)

/**
 * This function reads a grib file and return a php array containg all available grib message
 * Each message is an associative array formatted as follows : 
 * [
 *  "data" => all data points value for the current message
 *  "category" => the product category from https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_table4-1.shtml
 *  "product" => the product from tables 4.category, e.g. https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_table4-2-0-0.shtml
 *  "lat_points" => the number of points on each meridian
 *  "lon_points" => the number of points on each parallel
 *  "lat_step" => the number of degrees to go from one point to the next in latitude (units : 10^-6 degs)
 *  "lon_step" => the number of degrees to go from one point to the next in longitude (units : 10^-6 degs)
 *  "start_lat" => the starting latitude (units : 10^-6 degs)
 *  "start_lon" => the starting longitude (units : 10^-6 degs)
 *  "datetime" => forecast date and time
 * ]
 * 
 * 
**/
PHP_FUNCTION(read_grib2_file) {
    char *filename;
    size_t filename_len;

    // grib vars
    unsigned char *cgrib;
    g2int  listsec0[3],listsec1[13],numlocal,numfields;
    long   lskip,n,lgrib,iseek, k;
    int    unpack,ret,ierr,expand, start_lon, start_lat, lat_points, lon_points, lat_step, lon_step;
    gribfield  *gfld;
    FILE   *fptr;
    size_t  lengrib;


    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &filename, &filename_len) == FAILURE) {
        zend_throw_exception(zend_exception_get_default(TSRMLS_C), "File argument required", 0 TSRMLS_CC);
        RETURN_NULL();
    }


    if (filename_len) {
        //We have a filename, so parse it
        zval results;

        array_init(&results);

        iseek=0;
        unpack=1;
        expand=1;
        fptr=fopen(filename,"r");
        zval item;
        zval data;

        if (fptr != NULL) {
            
            php_printf("File %s loaded\n", filename);
            for (;;) {
                seekgb(fptr, iseek, 32000, &lskip, &lgrib);
                if (lgrib == 0) break;    // end loop at EOF or problem
                cgrib=(unsigned char *)malloc(lgrib);
                ret=fseek(fptr,lskip,SEEK_SET);
                lengrib=fread(cgrib,sizeof(unsigned char),lgrib,fptr);
                iseek=lskip+lgrib;
                ierr=g2_info(cgrib,listsec0,listsec1,&numfields,&numlocal);



                for (n=0;n<numfields;n++) {

                    ierr=g2_getfld(cgrib,n+1,unpack,expand,&gfld);

                    //Validity checks

                    // Wrong grid type
                    // https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_table3-0.shtml
                    if (gfld->griddef != 0) {
                        add_assoc_string_ex(&results, "error", 5, "Could not read Grib2 data: Wrong grid type");
                        break;
                    }

                    // Points not defined by lat / lng
                    // https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_table3-1.shtml
                    if (gfld->igdtnum != 0) {
                        add_assoc_string_ex(&results, "error", 5, "Could not read Grib2 data: Points not defined by lat / lng");
                        break;
                    }

                    // We only handle regular forecasts
                    // https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_table4-0.shtml
                    if (gfld->ipdtnum != 0) {
                        add_assoc_string_ex(&results, "error", 5, "Could not read Grib2 data: Not a regular forecast");
                        break;
                    }

                    // We only handle categories 0 to 2
                    // https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_table4-0.shtml
                    if (*(gfld->ipdtmpl) > 2) {
                        add_assoc_string_ex(&results, "error", 5, "Could not read Grib2 data: Product category handling not implementend");
                        break;
                    }

                    lat_points = *(gfld->igdtmpl+8);
                    lon_points = *(gfld->igdtmpl+7);
                    lat_step = *(gfld->igdtmpl+17);
                    lon_step = *(gfld->igdtmpl+16);
                    start_lat = *(gfld->igdtmpl+11);
                    start_lon = *(gfld->igdtmpl+12);

                    long cat = *(gfld->ipdtmpl);
                    long prod = *(gfld->ipdtmpl + 1);

                    const char* prod_name = "Unknown";

                    if (*(gfld->ipdtmpl) <= 2) {
                        prod_name = products[cat*21+prod];
                    }

                    //php_printf("%ld-%ld-%ld %ld:%ld:%ld\n", gfld->idsect[5], gfld->idsect[6], gfld->idsect[7], gfld->idsect[8], gfld->idsect[9], gfld->idsect[10]);

                    array_init(&item);
                    array_init(&data);

                    char buffer[50];
                    char kbuffer[50];

                    g2float *p = gfld->fld;
                    long *v = gfld->ipdtmpl;
                    long *f = gfld->igdtmpl;

                    // These are the data points

                    for (k=0; k < gfld->ndpts; k++) {
                        add_next_index_double(&data, *p);
                        p++;
                    }

                    add_assoc_zval(&item, "data", &data);

                    snprintf(buffer, 20, "%ld-%ld-%ld %ld:%ld:%ld\n", gfld->idsect[5], gfld->idsect[6], gfld->idsect[7], gfld->idsect[8], gfld->idsect[9], gfld->idsect[10]);
                    add_assoc_string(&item, "datetime", buffer);


                    // This is the grid definition
                    // https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_temp3-0.shtml
                    // for (k=0; k < gfld->igdtlen; k++) {
                    //     snprintf(buffer, 50, "%ld", *f);
                    //     snprintf(kbuffer, 50, "igdt %ld", k);
                    //     int length = snprintf( NULL, 0, "%ld", *f );
                    //     add_assoc_string_ex(&item, kbuffer, length+6, buffer);
                    //     f++;
                    // }


                    // snprintf(buffer, 10, "%lu", gfld->ipdtnum);
                    // add_assoc_string_ex(&item, "ipdtnum", 7, buffer);

                    // snprintf(buffer, 10, "%lu", gfld->ipdtlen);
                    // add_assoc_string_ex(&item, "ipdtlen", 7, buffer);

                    // https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_table4-1.shtml
                    // https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_table4-2-0-2.shtml
                    // for (k=0; k < gfld->ipdtlen; k++) {
                    //     snprintf(buffer, 50, "%ld", *v);
                    //     snprintf(kbuffer, 50, "ipdt %ld", k);
                    //     int length = snprintf( NULL, 0, "%ld", *v );
                    //     add_assoc_string_ex(&item, kbuffer, length+6, buffer);
                    //     v++;
                    // }

                    // // if ipdt 1 == 3 => V-Component of Wind
                    // // if ipdt 1 == 2 => U-Component of Wind


                    add_assoc_long_ex(&item, "category", 8, cat);

                    add_assoc_long_ex(&item, "product", 7, prod);

                    add_assoc_string_ex(&item, "name", 4, prod_name);

                    add_assoc_long_ex(&item, "lat_points", 4, lat_points);
                    add_assoc_long_ex(&item, "lon_points", 4, lon_points);
                    add_assoc_long_ex(&item, "lat_step", 4, lat_step);
                    add_assoc_long_ex(&item, "lon_step", 4, lon_step);
                    add_assoc_long_ex(&item, "start_lat", 4, start_lat);
                    add_assoc_long_ex(&item, "start_lon", 4, start_lon);
                    add_assoc_zval(&results, prod_name, &item);
                    g2_free(gfld);
                }
                
                free(cgrib);
            }

            zend_array *ret = Z_ARR(results);

            RETVAL_ARR(ret);

        } else {
            php_printf("File %s does not exist\n", filename);
            zend_throw_exception(zend_exception_get_default(TSRMLS_C), "File does not exist", 0 TSRMLS_CC);
        }
    }
    return;
}




PHP_FUNCTION(grib2_file_to_geojson) {
    char *filename;
    size_t filename_len;

    // grib vars
    unsigned char *cgrib;
    g2int  listsec0[3],listsec1[13],numlocal,numfields;
    long   lskip,n,lgrib,iseek, k;
    int    unpack,ret,ierr,expand, start_lon, start_lat, lat_points, lon_points, lat_step, lon_step;
    gribfield  *gfld;
    FILE   *fptr;
    size_t  lengrib;


    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &filename, &filename_len) == FAILURE) {
        zend_throw_exception(zend_exception_get_default(TSRMLS_C), "File argument required", 0 TSRMLS_CC);
        RETURN_NULL();
    }


    if (filename_len) {
        //We have a filename, so parse it
        zval results;

        array_init(&results);

        long highest_index = 0;

        iseek=0;
        unpack=1;
        expand=1;
        fptr=fopen(filename,"r");
        
        zval data;

        long field = 0;
        long totalfields = 0;


        if (fptr != NULL) {
            php_printf("File %s loaded\n", filename);

            long numpoints;
            long point_index;

            for (;;) {
                seekgb(fptr, iseek, 32000, &lskip, &lgrib);
                if (lgrib == 0) break;    // end loop at EOF or problem
                cgrib=(unsigned char *)malloc(lgrib);
                ret=fseek(fptr,lskip,SEEK_SET);
                lengrib=fread(cgrib,sizeof(unsigned char),lgrib,fptr);
                iseek=lskip+lgrib;
                ierr=g2_info(cgrib,listsec0,listsec1,&numfields,&numlocal);
                for (n=0;n<numfields;n++) {
                    long point_index = 0;
                    ierr=g2_getfld(cgrib,n+1,unpack,expand,&gfld);

                    numpoints = gfld->ndpts;
                    totalfields++;
                }
            }
            php_printf("num points %ld\n", numpoints);
            zval points[numpoints];
            float points_values[numfields * numpoints];
            char* field_names[numfields];
            zval points_geometries[numpoints];
            iseek = 0;

            for (;;) {
                
                seekgb(fptr, iseek, 32000, &lskip, &lgrib);
                if (lgrib == 0) break;    // end loop at EOF or problem
                cgrib=(unsigned char *)malloc(lgrib);
                ret=fseek(fptr,lskip,SEEK_SET);
                lengrib=fread(cgrib,sizeof(unsigned char),lgrib,fptr);
                iseek=lskip+lgrib;
                ierr=g2_info(cgrib,listsec0,listsec1,&numfields,&numlocal);
                //Date
                // snprintf(buffer, 20, "%ld-%ld-%ld %ld:%ld:%ld\n", gfld->idsect[5], gfld->idsect[6], gfld->idsect[7], gfld->idsect[8], gfld->idsect[9], gfld->idsect[10]);
                // add_assoc_string(&item, "datetime", buffer);

                for (n=0;n<numfields;n++) {
                    point_index = 0;
                    ierr=g2_getfld(cgrib,n+1,unpack,expand,&gfld);

                    //Validity checks

                    // Wrong grid type
                    // https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_table3-0.shtml
                    if (gfld->griddef != 0) {
                        add_assoc_string_ex(&results, "error", 5, "Could not read Grib2 data: Wrong grid type");
                        break;
                    }

                    // Points not defined by lat / lng
                    // https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_table3-1.shtml
                    if (gfld->igdtnum != 0) {
                        add_assoc_string_ex(&results, "error", 5, "Could not read Grib2 data: Points not defined by lat / lng");
                        break;
                    }

                    // We only handle regular forecasts
                    // https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_table4-0.shtml
                    if (gfld->ipdtnum != 0) {
                        add_assoc_string_ex(&results, "error", 5, "Could not read Grib2 data: Not a regular forecast");
                        break;
                    }

                    // We only handle categories 0 to 2
                    // https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_table4-0.shtml
                    if (*(gfld->ipdtmpl) > 2) {
                        add_assoc_string_ex(&results, "error", 5, "Could not read Grib2 data: Product category handling not implementend");
                        break;
                    }

                    lat_points = *(gfld->igdtmpl+8);
                    lon_points = *(gfld->igdtmpl+7);
                    lat_step = *(gfld->igdtmpl+17);
                    lon_step = *(gfld->igdtmpl+16);
                    start_lat = *(gfld->igdtmpl+11);
                    start_lon = *(gfld->igdtmpl+12);

                    php_printf("start lon: %d start lat: %d\n", start_lon, start_lat);
                    php_printf("lon_step: %d lat_step: %d\n", lon_step, lat_step);
                    php_printf("lon_points: %d lon_points: %d\n", lon_points, lat_points);

                    long cat = *(gfld->ipdtmpl);
                    long prod = *(gfld->ipdtmpl + 1);
                    //zval points[gfld->ndpts];

                    const char* prod_name = "Unknown";

                    if (*(gfld->ipdtmpl) <= 2) {
                        prod_name = products[cat*21+prod];
                    }
                    field_names[field] = prod_name;

                    // php_printf("product %s\n", prod_name);

                    //php_printf("%ld-%ld-%ld %ld:%ld:%ld\n", gfld->idsect[5], gfld->idsect[6], gfld->idsect[7], gfld->idsect[8], gfld->idsect[9], gfld->idsect[10]);

                    // {
                    //   "type": "Feature",
                    //   "geometry": {
                    //     "type": "Point",
                    //     "coordinates": [125.6, 10.1]
                    //   },
                    //   "properties": {
                    //     "name": "Dinagat Islands"
                    //   }
                    // }
                    

                    char buffer[50];
                    char kbuffer[50];

                    
                    

                    g2float *p = gfld->fld;
                    long *v = gfld->ipdtmpl;
                    long *f = gfld->igdtmpl;

                    // These are the data points
                    for (k=0; k < gfld->ndpts; k++) {
                        if (k < 1000) continue;
                        if (k > 1600) break;
                        zval point;
                        // Longitude and latitude
                        zval geometry;
                        if (k > highest_index || (k == 0 && highest_index == 0)) {
                            highest_index = k;

                            array_init(&geometry);

                            int lap = k/lon_points;
                            float latitude = roundf(((float)start_lat - (float)lap * (float)lat_step)/10000.0)/100;

                            int lop = k%lon_points;
                            float longitude = roundf(((float)start_lon + (float)lop * (float)lon_step)/10000.0)/100;

                            add_index_double(&geometry, 0, longitude);
                            add_index_double(&geometry, 1, latitude);
                            
                            array_init(&point);
                            add_assoc_string_ex(&point, "type", 4, "Feature");
                            add_assoc_zval(&point, "geometry", &geometry);
                            
                            points_values[field*totalfields + k] = *p;
                            add_index_zval(&results, k, &point);
                        } else {
                            // php_printf("numfield next: %ld\n", field);

                            points_values[field*totalfields + k] = *p;
                        }

                        p++;
                    }

                    
                    
                    g2_free(gfld);
                    field++;

                }
                
                free(cgrib);
            }

            // add all points

            // for (long j=0; j < 20; j++) {
                // zval properties = points_properties[j];
                // add_assoc_zval_ex(&point, "properties", 10, &properties);
                //add_assoc_zval(&results, "data", &points[1]);
            // }

            // if (Z_TYPE_P(points) == IS_NULL) {
            //     php_printf("is null");
            // }

            //add_assoc_zval(&results, "data", &bla);

            zend_array *ret = Z_ARR(results);

            zval result;
            array_init(&result);
            zend_ulong idx;
            zend_string *key;
            zval *val; 

            ZEND_HASH_FOREACH_KEY_VAL(ret, idx, key, val) {
                zval props;
                array_init(&props);
                for (long l=0; l < totalfields; l++) {
                    // php_printf("prop %s val: %f\n", field_names[l], points_values[l*numfields+j]);
                    add_assoc_double(&props, field_names[l], points_values[l*numfields+idx]);
                }

                add_assoc_zval_ex(val, "properties", 10, &props);

                add_next_index_zval(&result, val);
                
            } ZEND_HASH_FOREACH_END(); 

            // for (long j=0; j <= 100; j++) {
            //     zval props;
            //     array_init(&props);

            //     for (long l=0; l < totalfields; l++) {
            //         // php_printf("prop %s val: %f\n", field_names[l], points_values[l*numfields+j]);
            //         add_assoc_double(&props, field_names[l], points_values[l*numfields+j]);
            //     }

            //     zval poin = *zend_hash_index_find(ret, j);

            //     add_assoc_zval_ex(&poin, "properties", 10, &props);

            //     zend_hash_index_update(ret, j, &poin);
            // }

            zend_array *ret2 = Z_ARR(result);

            RETVAL_ARR(ret2);

        } else {
            zend_throw_exception(zend_exception_get_default(TSRMLS_C), "File does not exist", 0 TSRMLS_CC);
        }
    }
    return;
}