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


// Quick and dirty transfer variable for cube size from read to write function
uint cube_file_lut_size;


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
	uint divisor;
	float *pixel_ptr;
	
	//read cube size from input file name (dummy name)
	//fail if value is out of range (insane value or error value)
	cube_file_lut_size = (int) strtoimax(name, NULL, 0);
	if (cube_file_lut_size < 3 || cube_file_lut_size > 3000) {
		fprintf(stderr, "ERROR: Can't read cube size. Just type the cube size you want as the input file name!\n");
		exit(1);
	} else {
		fprintf(stdout, "\nInfo: The LUT size you entered is %d\n", cube_file_lut_size);
	}
	
	divisor = cube_file_lut_size-1;
	
	// init an image buffer to fill with pattern of a standard 0 to 1 cube input
	pixels->init( cube_file_lut_size*cube_file_lut_size*cube_file_lut_size , 1, 4);
	pixel_ptr = pixels->ptr();
	
	// fill pattern image in form a cube expects the input
	// each pixel represents a line from the resulting cube file
	for (b_count=0; b_count < cube_file_lut_size; b_count++) {
		for (g_count=0; g_count < cube_file_lut_size; g_count++) {
			for (r_count=0; r_count < cube_file_lut_size; r_count++) {
				//red
				*(pixel_ptr++) = (1.0*r_count)/divisor;
				//green
				*(pixel_ptr++) = (1.0*g_count)/divisor;
				//green
				*(pixel_ptr++) = (1.0*b_count)/divisor;
				//set alpha
				//spare the need for aIn command line some ctls expect if no alpha is set
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
				format_t *format) {

	uint64_t pixel_max;
	uint64_t pixel;
	float r;
	float g;
	float b;
	
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
	fprintf(fp_out, "TITLE \"Generated by modified ctlrender from Color Transformation Language files\"\n");
	fprintf(fp_out, "LUT_3D_SIZE %d\n", cube_file_lut_size);
	
	
	pixel_max = pixels.pixels();
	pixel_ptr = pixels.ptr();
	
	// write each result pixel line by line
	for(pixel=0; pixel < pixel_max; pixel++) {
		r = *(pixel_ptr++);
		g = *(pixel_ptr++);
		b = *(pixel_ptr++);
		pixel_ptr++; //ignore alpha
		
		fprintf(fp_out, "%.6f %.6f %.6f\n", r, g, b);
	}
	
	fclose(fp_out);
	
	fprintf(stdout, "Info: Cube file written\n");
}