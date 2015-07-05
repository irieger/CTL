/*
 * Dummy input file + CUBE LUT output file.
 *
 * This file type offers a pseudo input format which is just a pattern generator
 * that generates an pixel pattern which represents cube input values to pass
 * through a bunch of ctl files and generate a cube lut from it.
 *
 * Only tested with luts converting different formats through IDT, RRT and ODT or
 * A combination of IDT + aces2acesLog16i and another lut with aceslog2aces + RRT
 * + ODT. Not intended for use to make luts working with linear in or out.
 *
 * Attention: Needs modified acesLog16i_to_aces.ctl. See:
 *   https://groups.google.com/forum/#!topic/academyaces/VD7Yd0Yh7Sg
 *
 * Usage sample:
 *   ctlrender -ctl idt-alexav3-logC-EI800.ctl -ctl aces_to_acesLog16i.ctl 129 alexaLogC-EI800_2_acesLog.129.cube
 * This will create a 129*129*129 LUT to convert logC to acesLog. Input file
 * parameter is just the dimension of the generated lut.
 *
 * by Ingmar Rieger (git@irieger.net)
 */


#include <cube_file.hh>
#include <dpx.hh>
#include <alloca.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>


// Quick and dirty transfer variables for cube size, min and max from read to write function
uint cube_file_lut_size;
float cube_min;
float cube_max;
uint cube_float_length;

/*
 * Pseudo file reader. This method doesn't read a file but generates a pattern
 * simulating a LUT input with the number of steps defined by the "filename".
 *
 * Will die on invalid inputs as a simple error handling method!
 */
bool cube_read(const char *name, float scale,
               ctl::dpx::fb<float> *pixels,
               format_t *format) {

    uint r_count;
    uint g_count;
    uint b_count;
    float r_val;
    float g_val;
    float b_val;
    float multiplier;
    float *pixel_ptr;

    cube_min = 0.0;
    cube_max = 1.0;
    cube_float_length = 6;

    //read cube size from input file name (dummy name)
    cube_file_lut_size = (int) strtoimax(name, NULL, 0);
    if (cube_file_lut_size < 3 || cube_file_lut_size > 300) {
        fprintf(stderr, "ERROR: Can't read cube size. Just type the cube size you want as the input file name!\n");
        exit(1);
    } else {
        fprintf(stderr, "\nInfo: The LUT size you entered is %d\n", cube_file_lut_size);
    }

    if (getenv("CUBE_MIN") != NULL && getenv("CUBE_MAX") != NULL) {
        const char* cmin_str = getenv("CUBE_MIN");
        const char* cmax_str = getenv("CUBE_MAX");

        float cmin;
        float cmax;
        int scan_min_exit = sscanf(cmin_str, "%f", &cmin);
        int scan_max_exit = sscanf(cmax_str, "%f", &cmax);

        if (scan_min_exit == 0 || scan_max_exit == 0) {
            fprintf(stderr, "ERROR: Invalid CUBE_MIN or CUBE_MAX environment variable!\n");
            exit(1);
        } else {
            cube_min = cmin;
            cube_max = cmax;
        }

        printf("Cube Domain: Min: %f, Max: %f\n", cmin, cmax);
    }
    if (getenv("CUBE_FLOAT_LENGTH") != NULL) {
        const char* float_length_str = getenv("CUBE_FLOAT_LENGTH");
        int float_length;
        int scan_flt_exit = sscanf(float_length_str, "%d", &float_length);

        if (scan_flt_exit == 0) {
            fprintf(stderr, "ERROR: Invalid CUBE_FLOAT_LENGTH environment variable!\n");
            exit(1);
        } else {
            cube_float_length = float_length;
        }
    }


    multiplier = (cube_max - cube_min) / (cube_file_lut_size-1);

    pixels->init( cube_file_lut_size*cube_file_lut_size*cube_file_lut_size , 1, 4);
    pixel_ptr = pixels->ptr();


    for (b_count=0; b_count < cube_file_lut_size; b_count++) {
        for (g_count=0; g_count < cube_file_lut_size; g_count++) {
            for (r_count=0; r_count < cube_file_lut_size; r_count++) {
                //red
                *(pixel_ptr++) = cube_min + r_count*multiplier;
                //green
                *(pixel_ptr++) = cube_min + g_count*multiplier;
                //blue
                *(pixel_ptr++) = cube_min + b_count*multiplier;
                //set alpha
                *(pixel_ptr++) = 1.0;
            }
        }
    }

    return true;
}


/*
 * Cube file writer. Will just generate a cube lut with the dimension defined
 * in the input function by writing the result of the transform (pixel values)
 * to a cube file line by line.
 *
 * Dies on invalid output (non-existing or non-writeable)!
 */
void cube_write(const char *name, float scale,
                const ctl::dpx::fb<float> &pixels,
                format_t *format) {//, const CTLOperations &ctl_operations) {

    uint64_t pixel_max;
    uint64_t pixel;
    float r;
    float g;
    float b;

    char str_tmp[120];

    const float *pixel_ptr;
    FILE *fp_out;


    // Fail if file exists or couldn't be created
    if (access(name, F_OK) == 0) {
        fprintf(stderr, "ERROR: File already exists\n");
        exit(1);
    }
    fp_out = fopen( name, "w" );
    if (fp_out == NULL) {
        fprintf(stderr, "ERROR: File could not be created!\n");
        exit(1);
    }

    // init cube header
    //fprintf(fp_out, "## %s\n\n", "Fill later");
    //add list of used ctl files later

    if (getenv("CUBE_COMMENT") != NULL) {
        const char* cube_comment = getenv("CUBE_COMMENT");
        fprintf(fp_out, "## %s\n\n", cube_comment);
    }

    fprintf(fp_out, "TITLE \"Generated by modified ctlrender from Color Transformation Language files\"\n");
    fprintf(fp_out, "LUT_3D_SIZE %d\n", cube_file_lut_size);

    if (cube_min != 0.0 && cube_max != 1.0) {
        sprintf(str_tmp, "LUT_3D_INPUT_RANGE %%.%df %%.%df\n", cube_float_length, cube_float_length);
        fprintf(fp_out, str_tmp, cube_min, cube_max);
        //fprintf(fp_out, "LUT_3D_INPUT_RANGE %.5f %.5f\n", cube_min, cube_max);
    }

    fprintf(fp_out, "\n");

    sprintf(str_tmp, "%%.%df %%.%df %%.%df\n", cube_float_length, cube_float_length, cube_float_length);

    pixel_max = pixels.pixels();
    pixel_ptr = pixels.ptr();

    // write each result pixel line by line
    for(pixel=0; pixel < pixel_max; pixel++) {
        r = *(pixel_ptr++);
        g = *(pixel_ptr++);
        b = *(pixel_ptr++);
        pixel_ptr++; //ignore alpha

        fprintf(fp_out, str_tmp, r, g, b);
    }

    fclose(fp_out);

    fprintf(stdout, "Info: Cube file written\n");
}
