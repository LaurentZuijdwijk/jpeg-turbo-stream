#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "io.h"
#include <archive.h>
#include <archive_entry.h>
#include <jpeglib.h>
#include "debug.h"
#include <setjmp.h>
#include <syslog.h>
#define MIN(x,y) (x < y ? x : y)


static char* formats[] = {"NOOP", "INFO", "JPEG", "GIF", "PNG", "BMP", "PDF"};
enum {NOOP=0, INFO=1, JPEG=2, GIF=3, PNG=4, BMP=5, PDF=6};
enum {CONTAIN=0x01, FIXED=0x02, COVER=0x04, MULTIPAGE=0x08};

typedef struct {
  uint32_t scale_options;
  uint32_t scale_width;
  uint32_t scale_height;
  uint32_t crop_x;
  uint32_t crop_y;
  uint32_t crop_width;
  uint32_t crop_height;
  uint32_t rotate_degrees;
  uint32_t density;
  uint32_t page_start;
  uint32_t page_end;
  uint32_t format;
  uint32_t split;
} convert_t;

typedef struct {
  uint32_t width;
  uint32_t height;
  uint32_t format;
  uint32_t pages;
} convert_info_t;


// int convert_rotate (MagickWand *input, convert_t *opts) {
//   if (!opts->rotate_degrees) return MagickPass;
//   if (opts->rotate_degrees == 360) opts->rotate_degrees = 0;

//   char* exif_orientation = MagickGetImageAttribute(input,  "EXIF:Orientation");

//   if (exif_orientation != NULL) {
//     switch (atoi(exif_orientation)) {
//       case 3:
//       opts->rotate_degrees += 180;
//       break;

//       case 6:
//       opts->rotate_degrees += 90;
//       break;

//       case 8:
//       opts->rotate_degrees += 270;
//       break;
//     }

//     // just clear all exif data
//     MagickProfileImage(input, "exif", NULL, 0);
//   }

//   return opts->rotate_degrees ? MagickRotateImage(input, default_background, opts->rotate_degrees) : MagickPass;
// }

// int convert_crop (MagickWand *input, convert_t *opts) {
//   unsigned long crop_width = opts->crop_width;
//   unsigned long crop_height = opts->crop_height;

//   if (!crop_width && !opts->crop_x && !crop_height && !opts->crop_y) return MagickPass;
//   if (!crop_width) crop_width = MagickGetImageWidth(input);
//   if (!crop_height) crop_height = MagickGetImageHeight(input);

//   return MagickCropImage(input, crop_width, crop_height, opts->crop_x, opts->crop_y);
// }

// int convert_scale (MagickWand *input, convert_t *opts) {
//   unsigned long scale_width = opts->scale_width;
//   unsigned long scale_height = opts->scale_height;

//   if (!scale_height && !scale_width) return MagickPass;

//   unsigned long wid = MagickGetImageWidth(input);
//   unsigned long hei = MagickGetImageHeight(input);
//   double ratio = (double)wid / (double)hei;

//   unsigned long new_wid = 0;
//   unsigned long new_hei = 0;

//   if (opts->scale_options & FIXED) {
//     new_wid = scale_width;
//     new_hei = scale_height;
//   } else {
//     new_wid = MIN(scale_width, wid);
//     new_hei = MIN(scale_height, hei);

//     if (new_hei && new_wid) {
//       double new_ratio = (double)new_wid / (double)new_hei;

//       if (opts->scale_options & COVER) {
//         if (new_ratio > ratio) new_hei = 0;
//         else new_wid = 0;
//       }
//       if (opts->scale_options & CONTAIN) {
//         if (new_ratio > ratio) new_wid = 0;
//         else new_hei = 0;
//       }
//     }
//   }
//   if (!new_wid) new_wid = ratio * (double)new_hei;
//   if (!new_hei) new_hei = (double)new_wid / ratio;
//   return MagickScaleImage(input, new_wid, new_hei);
// }



int decompress(struct jpeg_decompress_struct input_info, JSAMPARRAY scanlines, int row_stride, unsigned char* data, size_t size){ 
  struct jpeg_error_mgr jerr;

  input_info.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&input_info);
  jpeg_mem_src(&input_info, data, size);
  jpeg_read_header(&input_info, TRUE);
  jpeg_start_decompress(&input_info);
  
  row_stride = input_info.output_width * input_info.output_components;
  /* Make a one-row-high sample array that will go away when done with image */
  scanlines = (*input_info.mem->alloc_sarray)
                ((j_common_ptr) &input_info, JPOOL_IMAGE, row_stride, 1);
  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  /* Here we use the library's state variable input_info.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
  while (input_info.output_scanline < input_info.output_height) {
    /* jpeg_read_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could ask for
     * more than one scanline at a time if that's more convenient.
     */
    (void) jpeg_read_scanlines(&input_info, scanlines, 1);
    /* Assume put_scanline_someplace wants a pointer and sample count. */
  }
  (void) jpeg_finish_decompress(&input_info);
  jpeg_destroy_decompress(&input_info);
return 1;

}
int compress(struct jpeg_decompress_struct input_info,  JSAMPARRAY scanlines, int row_stride){
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  
  FILE * outfile;
  if ((outfile = fopen("out.jpg", "wb")) == NULL) {
      fprintf(stderr, "can't open %s\n", "out.jpg");
       exit(1);
  }

  jpeg_stdio_dest(&cinfo, outfile);
  cinfo.image_width = input_info.output_width;      /* image width and height, in pixels */
  cinfo.image_height = input_info.output_height;
  cinfo.input_components = 3;     /* # of color components per pixel */
  cinfo.in_color_space = JCS_RGB; /* colorspace of input image */

  jpeg_set_defaults(&cinfo);

  jpeg_start_compress(&cinfo, TRUE);
  // for (int i = 0; i < scanlines.length; ++i)
  // {
    jpeg_write_scanlines(&cinfo, scanlines, input_info.output_width);
    /* code */
  // }
    jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);
  return 1;

}




int convert ( convert_t *opts, unsigned char* data, size_t size) {
  JSAMPARRAY scanlines;            /* Output row buffer */
  struct jpeg_decompress_struct input;

  int stride = 0;
  unsigned char* output;
  
  decompress(input, scanlines, stride, data, size);
  compress(input, scanlines, stride);

}

// void destroy (MagickWand *input, MagickWand *output) {
//   if (input != NULL) DestroyMagickWand(input);
//   if (output != NULL && input != output) DestroyMagickWand(output);
// }

// int write_archive_from_mem(char *outname, MagickWand *wand)
// {
//   int archiveSize = 0;
//   int pageNumber = 1;
//   struct archive *a;
//   struct archive_entry *entry;
//   a = archive_write_new();
//   archive_write_set_format_ustar(a);
//   archive_write_open_filename(a, outname);
//   char filename[13];
//   MagickResetIterator(wand);
//   MagickNextImage(wand); // Has to be called after MagickResetIterator to set the first picture as the current
//   do { 
//     unsigned char *data;
//     size_t size;
//     data = MagickWriteImageBlob(wand, &size);
//     entry = archive_entry_new();
//     snprintf(filename, 13, "page_%d", pageNumber++);
//     archive_entry_set_pathname(entry, filename);
//     archive_entry_set_size(entry, size);
//     archive_entry_set_filetype(entry, AE_IFREG);
//     archive_entry_set_perm(entry, 0644);
//     archive_write_header(a, entry);
//     archiveSize += archive_write_data(a, data, size);
//     archive_entry_free(entry);
//   } while (MagickNextImage(wand));
//   archive_write_close(a);
//   archive_write_free(a);
//   return archiveSize;
// }

int parse (size_t size, unsigned char *data) {
  int writtendata = 0;
  
  convert_t *opts = (convert_t*) data;
  
  size -= sizeof(convert_t);
  data += sizeof(convert_t);

  // here we convert the file
  int status = convert(opts, data, size);
  //writtendata = io_write(size, data);
  //}
  //destroy(input, output);
  //free(data);
  //return writtendata;
}

int main (int argc, char *argv[]) {
  return io_read(parse);
}
