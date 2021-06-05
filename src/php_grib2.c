// include the PHP API itself
#include <php.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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



// Finally, we implement our "Hello World" function
// this function will be made available to PHP
// and prints to PHP stdout using printf
PHP_FUNCTION(read_grib2_file) {
    php_printf("Hello World! (from our extension)\n");

    //input vars
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
                        break;
                    }

                    // Points not defined by lat / lng
                    // https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_table3-1.shtml
                    if (gfld->igdtnum != 0) {
                        break;
                    }

                    // We only handle regular forecasts
                    // https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_table4-0.shtml
                    if (gfld->ipdtnum != 0) {
                        break;
                    }

                    // We only handle categories 0 to 2
                    // https://www.nco.ncep.noaa.gov/pmb/docs/grib2/grib2_doc/grib2_table4-0.shtml
                    if (*(gfld->ipdtmpl) > 2) {
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

                    const char* prod_name = products[cat*21+prod];

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

                    snprintf(buffer, 10, "%ld", cat);
                    add_assoc_string_ex(&item, "category", 8, buffer);

                    snprintf(buffer, 10, "%ld", prod);
                    add_assoc_string_ex(&item, "product", 7, buffer);

                    add_assoc_string_ex(&item, "name", 4, prod_name);

                    add_assoc_long_ex(&item, "lat_points", 4, lat_points);
                    add_assoc_long_ex(&item, "lon_points", 4, lon_points);
                    add_assoc_long_ex(&item, "lat_step", 4, lat_step);
                    add_assoc_long_ex(&item, "lon_step", 4, lon_step);
                    add_assoc_long_ex(&item, "start_lat", 4, start_lat);
                    add_assoc_long_ex(&item, "start_lon", 4, start_lon);

                    // snprintf(buffer, 10, "%lu", gfld->ipdtlen);
                    // add_assoc_string_ex(&item, "ipdtlen", 7, buffer);

                    // snprintf(buffer, 10, "%lu", gfld->interp_opt);
                    // add_assoc_string_ex(&item, "interp_opt", 12, buffer);

                    // snprintf(buffer, 10, "%lu", gfld->idsect[5]);

                    // add_assoc_string_ex(&item, "year", 4, buffer);
                    // snprintf(buffer, 10, "%lu", gfld->idsect[6]);
                    // add_assoc_string_ex(&item, "month", 5, buffer);
                    // snprintf(buffer, 10, "%lu", gfld->idsect[7]);
                    // add_assoc_string_ex(&item, "day", 3, buffer);
                    // snprintf(buffer, 10, "%lu", gfld->idsect[8]);
                    // add_assoc_string_ex(&item, "hour", 4, buffer);
                    // snprintf(buffer, 10, "%lu", gfld->idsect[9]);
                    // add_assoc_string_ex(&item, "min", 3, buffer);
                    // snprintf(buffer, 10, "%lu", gfld->idsect[10]);
                    // add_assoc_string_ex(&item, "sec", 3, buffer);
                    // snprintf(buffer, 10, "%lu", gfld->locallen);
                    // add_assoc_string_ex(&item, "locallen", 8, buffer);


                    // snprintf(buffer, 10, "%ld", gfld->igdtnum);
                    // add_assoc_string_ex(&item, "igdtnum", 7, buffer);

                    


                    
                    
                    // int length = snprintf( NULL, 0, "%ld", gfld->version );
                    // char* str = malloc( length + 1 );

                    // snprintf( str, length + 1, "%ld", gfld->idsect[5] );
                    // PHPWRITE(str, length);

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

    

    //RETVAL_STRINGL(out, out_len);

    return;

}