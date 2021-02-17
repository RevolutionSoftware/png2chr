#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <png.h>

#define HLEN 8

typedef unsigned char Uint8;
typedef unsigned short Uint16;
typedef unsigned int Uint32;
typedef unsigned long Uint64;

typedef struct {
	char name[256];
	png_structp imgptr;
	png_infop infoptr;

	Uint8 header[HLEN];
	Uint32 imgw;
	Uint32 imgh;
	Uint8 type;
	Uint8 depth;
	Uint8 **rows;
} PNG_Doc;

PNG_Doc inf; /* INputFile */

int
error(char *msg, char *err)
{
	printf("ERROR %s: %s\n", msg, err);
	return 1;
}

void
setuppng(FILE *fp, PNG_Doc *iput)
{
	png_init_io(iput->imgptr, fp);
	png_set_sig_bytes(iput->imgptr, HLEN);
	png_read_png(iput->imgptr, iput->infoptr, PNG_TRANSFORM_PACKING | PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_STRIP_ALPHA, NULL);

	iput->rows = png_get_rows(iput->imgptr, iput->infoptr);
	iput->imgw = png_get_image_width(iput->imgptr, iput->infoptr);
	iput->imgh = png_get_image_height(iput->imgptr, iput->infoptr);
	iput->type = png_get_color_type(iput->imgptr, iput->infoptr);
	iput->depth = png_get_bit_depth(iput->imgptr, iput->infoptr);
}

void
exportsprite(FILE *chrfile, Uint8 **rows, Uint8 col, Uint8 row)
{
	Uint32 y, x;
	Uint8 buf1[8];
	Uint8 buf2[8];
	Uint8 pixel, bit0, bit1;
	for(y = 0; y < 8; y++) {
		/* Clear the current byte in the buffer */
		buf1[y] = 0;
		buf2[y] = 0;
		for(x = 0; x < 8; x++) {
			/* Grab the pixel from the rows array. */
			pixel = rows[row + y][col + x];

			/* We only care about bits 0 and 1 of 'pixel': %XXXX XX11
			 * bit0 goes into buffer one, bit1 into buffer 2. bit1 will be %10
             * or %00, so we want to shift it right so that it's %1 or %0.
			 */
			bit0 = pixel & 1;
			bit1 = (pixel & 2) >> 1;

			/* We want to put the bits starting from the left of the byte. */
			buf1[y] |= bit0 << (7 - x);
			buf2[y] |= bit1 << (7 - x);
		}
	}
	/* Write the buffers to the .chr file. The CHR is one sprite at a time, each
	 * sprite = 16 bytes. The first 8 bytes hold bit 0 and the second 8 bytes
     * hold bit 1. Combined, that gives you the palette index. For more info:
	 * http://www.dustmop.io/blog/2015/04/28/nes-graphics-part-1/#chr-encoding
	 */
	fwrite(buf1, 8, 1, chrfile);
	fwrite(buf2, 8, 1, chrfile);
}

void
putdata(PNG_Doc *iput, FILE *chrfile)
{
	Uint32 y, x;
	Uint8 spriteno = 0;

	for(y = 0; y < iput->imgh / 8; y++) {
		for(x = 0; x < iput->imgw / 8; x++) {
			printf("Writing sprite #%d\n", ++spriteno);
			exportsprite(chrfile, iput->rows, x * 8, y * 8);
		}
	}
}

void
close(FILE *fp, FILE *output, PNG_Doc *iput)
{
	fclose(fp);
	fclose(output);
	png_destroy_info_struct(iput->imgptr, &iput->infoptr);
	png_destroy_read_struct(&iput->imgptr, NULL, NULL);
}

void
usage()
{
}

int
main(int argc, char *argv[])
{
	PNG_Doc *input = &inf;

	FILE *fp;
	FILE *output;

	char filename[256];
	Uint8 namelength;

	int p_png; /* Predicate PNG - Is it a PNG file? */
	Uint32 rowbytes;

	if(argc < 2) {
		printf("Usage: %s input.png [output.chr]\n", argv[0]);
		printf("Currently having a [output.chr] argument isn't supported.\n");

		return 0;
	}

	namelength = strlen(argv[1]);

	/* Open .png file for reading */
	fp = fopen(argv[1], "r");
	if(!fp)
		return error("bad input", "checkpng - !fp");

	if(fread(input->header, 1, HLEN, fp) != HLEN)
		return error("wrong number of bytes read", "fread output not 8");
	p_png = !png_sig_cmp(input->header, 0, HLEN);
	if(!p_png)
		return error("Not a png", "png_sig_cmp");

	input->imgptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!input->imgptr)
		return error("IMGPTR", "PNG Failure");

	input->infoptr = png_create_info_struct(input->imgptr);
	if(!input->infoptr)
		return error("INFOPTR", "PNG Failure");

	if(setjmp(png_jmpbuf(input->imgptr))) {
		png_destroy_read_struct(&input->imgptr, &input->infoptr, (png_infopp)NULL);
		fclose(fp);

		return error("PNG", "NULL");
	}

	/* Use png library to extract data from png file
     * extracted data will be stored in input
	 */
	setuppng(fp, input);

	if(input->imgw % 8 != 0 || input->imgh % 8 != 0)
		return error("PNG Image", "Not divisible by 8");
	if(input->type != PNG_COLOR_TYPE_PALETTE)
		return error("PNG Image", "Not paletted type");
	if(input->depth != 8)
		return error("PNG Image", "Depth not 8");

	rowbytes = png_get_rowbytes(input->imgptr, input->infoptr);

	if(rowbytes != input->imgw)
		return error("Packing", "Failure. imgw != rowbytes");

	/* Open .chr file for writing */
	strcpy(filename, argv[1]);
	filename[namelength - 1] = 'r';
	filename[namelength - 2] = 'h';
	filename[namelength - 3] = 'c';
	output = fopen(filename, "w");

	putdata(input, output);
	close(fp, output, input);

	return 0;
}
