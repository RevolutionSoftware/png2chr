#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

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
	Uint8 original_depth;
	Uint8 **rows;
} PNG_Doc;

PNG_Doc inf; /* INputFile */

enum image_type {
	IMG_PALETTE,
	IMG_GRAYSCALE_4C,
	IMG_GRAYSCALE_256C,
	IMG_UNKNOWN,
};

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

	png_read_info(iput->imgptr, iput->infoptr);
	iput->original_depth = png_get_bit_depth(iput->imgptr, iput->infoptr);

	if (iput->original_depth < 8) {
		png_set_packing(iput->imgptr); //make sure each pixel is a byte
	}
	if (iput->original_depth == 16) {
		png_set_strip_16(iput->imgptr); //strip 16-bit images to 8-bit
	}
	
	png_set_strip_alpha(iput->imgptr); //remove alpha channel from the image
	png_set_rgb_to_gray(iput->imgptr, 1, -1.0, -1.0); //convert RGB to grayscale
	png_read_update_info(iput->imgptr, iput->infoptr);

	//png_read_png(iput->imgptr, iput->infoptr, PNG_TRANSFORM_PACKING | PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_STRIP_ALPHA, NULL);
	
	iput->imgw = png_get_image_width(iput->imgptr, iput->infoptr);
	iput->imgh = png_get_image_height(iput->imgptr, iput->infoptr);
	iput->type = png_get_color_type(iput->imgptr, iput->infoptr);
	iput->depth = png_get_bit_depth(iput->imgptr, iput->infoptr);

	iput->rows = png_malloc(iput->imgptr, iput->imgh * sizeof(png_bytep));
	int i;
	for (i=0; i < iput->imgh; i++) {
		iput->rows[i] = NULL;
		iput->rows[i] = png_malloc(iput->imgptr, iput->imgw * (iput->depth / 8));
	}

	png_read_image(iput->imgptr, iput->rows);
	png_read_end(iput->imgptr, NULL);
}

void
exportsprite_256c(FILE *chrfile, Uint8 **rows, Uint8 col, Uint8 row)
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
			/* Grab the pixel from the rows array.
			 * The pixel value is 0 to 255, so we divide it by 64
			 * and get a value from 0 to 3. */
			pixel = rows[row + y][col + x] / 64;

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
putdata(PNG_Doc *iput, FILE *chrfile, enum image_type type)
{
	Uint32 y, x;
	Uint8 spriteno = 0;

	for(y = 0; y < iput->imgh / 8; y++) {
		for(x = 0; x < iput->imgw / 8; x++) {
			printf("Writing sprite #%d\n", ++spriteno);
			switch(type) {
				case IMG_PALETTE:
				case IMG_GRAYSCALE_4C:
					exportsprite(chrfile, iput->rows, x * 8, y * 8); break;
				case IMG_GRAYSCALE_256C:
					exportsprite_256c(chrfile, iput->rows, x * 8, y * 8); break;
			}
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
usage(char *argv[])
{
	printf("Usage: %s input.png [output.chr]\n", argv[0]);
	printf("If missing optional [output.chr] param is entered ");
	printf("input name will be used as it's base.\n");
}

int
main(int argc, char *argv[])
{
	PNG_Doc *input = &inf;

	FILE *fp;
	FILE *output;

	char *outputptr;
	Uint8 namelength;

	int p_png; /* Predicate PNG - Is it a PNG file? */
	Uint32 rowbytes;

	if(argc < 2) {
		usage(argv);
		return 0;
	}

	/* Open .png file for reading */
	fp = fopen(argv[1], "rb");
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

	enum image_type type = IMG_UNKNOWN;

	if(input->imgw % 8 != 0 || input->imgh % 8 != 0)
		return error("PNG Image", "Not divisible by 8");

	if ((input->type == PNG_COLOR_TYPE_PALETTE) && (input->depth == 8)) {
		type = IMG_PALETTE;
	}

	if ((input->type == PNG_COLOR_TYPE_GRAY) && (input->depth == 8)) {

		/* due to PNG_TRANSFORM_PACKING the bit depth would never be
		 * less than 8, but it's worth checking if it's a 2bpp image
		 * re-packed into 8bpp or a native 8bpp image, because
		 * the significant bits are still the same */

		png_byte channels = png_get_channels(input->imgptr,input->infoptr);
		assert (channels == 1);
		/*would be weird if there's more than 1 channel in a grayscale image without alpha, but we're making sure */

		switch (input->original_depth) {
			case 2: type = IMG_GRAYSCALE_4C; break;
			case 8: 
			case 16: type = IMG_GRAYSCALE_256C; break;
			default: type = IMG_UNKNOWN; break;
		}
	}
	

	if (type == IMG_UNKNOWN) {
		fprintf(stderr,
			"The PNG image provided is not supported.\n"
			"This program supports the following PNG image types:\n"
			" * Indexed color (8 bits per pixel)\n"
			" * Grayscale (2 bits per pixel / 4 colors)\n"
			" * Grayscale (8 bits per pixel / 256 colors, will be downsampled)\n"
			" * RGB (8 or 16 bits per pixel, will be downsampled)\n");
		return 1;
	}

	rowbytes = png_get_rowbytes(input->imgptr, input->infoptr);

	if ((type == IMG_PALETTE) && (rowbytes != input->imgw))
		return error("Packing", "Failure. imgw != rowbytes");

	namelength = strlen(argv[1]);
	
	size_t outnamelength = namelength + 4 + 1; //enough to add a .png at the end of the name
	char outputname[outnamelength];

	/* Open .chr file for writing */
	strcpy(outputname, argv[1]);

	char* last_dot = strrchr(outputname,'.'); //find the file's extension by looking 
	if (!last_dot) last_dot = outputname + strlen(outputname); //if there's none, we'll add one
	/* WARNING: If namelength < 3, this will segfault the program. */
	last_dot[0] = '.';
	last_dot[1] = 'c';
	last_dot[2] = 'h';
	last_dot[3] = 'r';
	last_dot[4] = 0;

	/* If a second parameter is entered: use that as output filename. */
	outputptr = argc > 2 ? argv[2] : outputname;

	output = fopen(outputptr, "wb");

	putdata(input, output, type);
	close(fp, output, input);

	return 0;
}
