/*
 * Dummy input file + CUBE LUT output file.
 * More information in cube_file.cc
 
 * by Ingmar Rieger (git@irieger.net)
 */


#include "main.hh"
#include <dpx.hh>

bool cube_read(const char *name, float scale,
				ctl::dpx::fb<float> *pixels,
				format_t *format);
void cube_write(const char *name, float scale,
				const ctl::dpx::fb<float> &pixels,
				format_t *format);