// memdjpeg - A super simple example of how to decode a jpeg in memory
// Kenneth Finnegan, 2012
// blog.thelifeofkenneth.com
//
// After installing jpeglib, compile with:
// cc memdjpeg.c -ljpeg -o memdjpeg
//
// Run with:
// ./memdjpeg filename.jpg
//
// Version     Date   Time      By
// -------  ----------  -----   ---------
// 0.01   2012-07-09  11:18   Kenneth Finnegan
//

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/stat.h>

#include <jpeglib.h>
#include "io.h"
#include <archive.h>
#include <archive_entry.h>
#include "debug.h"


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
unsigned long out_size = 0;
  //free(data);
unsigned char* pCompressed = 0;

int convert (size_t size, unsigned char* data, convert_t* options) {
  int rc, i, j;

  char *syslog_prefix = (char*) malloc(1024);
  openlog(syslog_prefix, LOG_PERROR | LOG_PID, LOG_USER);



//   SSS    EEEEEEE  TTTTTTT  U     U  PPPP   
// SS   SS  E           T     U     U  P   PP 
// S        E           T     U     U  P    PP
// SS       E           T     U     U  P   PP 
//   SSS    EEEE        T     U     U  PPPP   
//      SS  E           T     U     U  P      
//       S  E           T     U     U  P      
// SS   SS  E           T      U   U   P      
//   SSS    EEEEEEE     T       UUU    P      

  // Variables for the source jpg
  unsigned long jpg_size;
  unsigned char *jpg_buffer;

  // Variables for the decompressor itself
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;

  // Variables for the output buffer, and how long each row is
  unsigned long bmp_size;
  unsigned char *bmp_buffer;
  int row_stride, width, height, pixel_size;


  // Load the jpeg data from a file into a memory buffer for 
  // the purpose of this demonstration.
  // Normally, if it's a file, you'd use jpeg_stdio_src, but just
  // imagine that this was instead being downloaded from the Internet
  // or otherwise not coming from disk
  jpg_size = size;
  jpg_buffer = data;


//   SSS    TTTTTTT     A     RRRR     TTTTTTT
// SS   SS     T       A A    R   RR      T   
// S           T      A   A   R    RR     T   
// SS          T     A     A  R   RR      T   
//   SSS       T     AAAAAAA  RRRR        T   
//      SS     T     A     A  R RR        T   
//       S     T     A     A  R   R       T   
// SS   SS     T     A     A  R    R      T   
//   SSS       T     A     A  R     R     T   

  syslog(LOG_INFO, "Proc: Create Decompress struct");
  // Allocate a new decompress struct, with the default error handler.
  // The default error handler will exit() on pretty much any issue,
  // so it's likely you'll want to replace it or supplement it with
  // your own.
  cinfo.err = jpeg_std_error(&jerr);  
  jpeg_create_decompress(&cinfo);


  syslog(LOG_INFO, "Proc: Set memory buffer as source");
  // Configure this decompressor to read its data from a memory 
  // buffer starting at unsigned char *jpg_buffer, which is jpg_size
  // long, and which must contain a complete jpg already.
  //
  // If you need something fancier than this, you must write your 
  // own data source manager, which shouldn't be too hard if you know
  // what it is you need it to do. See jpeg-8d/jdatasrc.c for the 
  // implementation of the standard jpeg_mem_src and jpeg_stdio_src 
  // managers as examples to work from.
  jpeg_mem_src(&cinfo, jpg_buffer, jpg_size);


  syslog(LOG_INFO, "Proc: Read the JPEG header");
  // Have the decompressor scan the jpeg header. This won't populate
  // the cinfo struct output fields, but will indicate if the
  // jpeg is valid.
  rc = jpeg_read_header(&cinfo, TRUE);

  if (rc != 1) {
    syslog(LOG_ERR, "File does not seem to be a normal JPEG");
    exit(EXIT_FAILURE);
  }

  syslog(LOG_INFO, "Proc: Initiate JPEG decompression");
  // By calling jpeg_start_decompress, you populate cinfo
  // and can then allocate your output bitmap buffers for
  // each scanline.
  


  cinfo.scale_num = 100;
  cinfo.scale_denom = cinfo.image_width / options->scale_width * 100;

//  cinfo.image_width = options->scale_width;
//  cinfo.image_height = options->scale_height;

  jpeg_start_decompress(&cinfo);
  

  width = cinfo.image_width;
  height = cinfo.image_width;
  

  pixel_size = cinfo.output_components;

  syslog(LOG_INFO, "Proc: Image is %d by %d with %d components", 
      width, height, pixel_size);

  bmp_size = width * height * pixel_size;
  bmp_buffer = (unsigned char*) malloc(bmp_size);

  // The row_stride is the total number of bytes it takes to store an
  // entire scanline (row). 
  row_stride = width * pixel_size;


  syslog(LOG_INFO, "Proc: Start reading scanlines");
  //
  // Now that you have the decompressor entirely configured, it's time
  // to read out all of the scanlines of the jpeg.
  //
  // By default, scanlines will come out in RGBRGBRGB...  order, 
  // but this can be changed by setting cinfo.out_color_space
  //
  // jpeg_read_scanlines takes an array of buffers, one for each scanline.
  // Even if you give it a complete set of buffers for the whole image,
  // it will only ever decompress a few lines at a time. For best 
  // performance, you should pass it an array with cinfo.rec_outbuf_height
  // scanline buffers. rec_outbuf_height is typically 1, 2, or 4, and 
  // at the default high quality decompression setting is always 1.
  while (cinfo.output_scanline < cinfo.output_height) {
    unsigned char *buffer_array[1];
    buffer_array[0] = bmp_buffer + \
               (cinfo.output_scanline) * row_stride;

    jpeg_read_scanlines(&cinfo, buffer_array, 1);

  }
  syslog(LOG_INFO, "Proc: Done reading scanlines");


  // Once done reading *all* scanlines, release all internal buffers,
  // etc by calling jpeg_finish_decompress. This lets you go back and
  // reuse the same cinfo object with the same settings, if you
  // want to decompress several jpegs in a row.
  //
  // If you didn't read all the scanlines, but want to stop early,
  // you instead need to call jpeg_abort_decompress(&cinfo)
  jpeg_finish_decompress(&cinfo);

  // At this point, optionally go back and either load a new jpg into
  // the jpg_buffer, or define a new jpeg_mem_src, and then start 
  // another decompress operation.
  
  // Once you're really really done, destroy the object to free everything
  jpeg_destroy_decompress(&cinfo);
  // And free the input buffer
//  free(jpg_buffer);

// DDDD       OOO    N     N  EEEEEEE
// D  DDD    O   O   NN    N  E      
// D    DD  O     O  N N   N  E      
// D     D  O     O  N N   N  E      
// D     D  O     O  N  N  N  EEEE   
// D     D  O     O  N   N N  E      
// D    DD  O     O  N   N N  E      
// D  DDD    O   O   N    NN  E      
// DDDD       OOO    N     N  EEEEEEE
  
  // Write the decompressed bitmap out to a ppm file, just to make sure 
  // it worked. 

 // free(bmp_buffer);

  struct jpeg_compress_struct compress_info;

  JSAMPROW row_pointer[1];  /* pointer to JSAMPLE row[s] */

  compress_info.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&compress_info);



  jpeg_mem_dest(&compress_info, &pCompressed, &out_size);
  

  /* Step 3: set parameters for compression */

  /* First we supply a description of the input image.
   * Four fields of the compress_info struct must be filled in:
   */
  compress_info.image_width = options->scale_width;  /* image width and height, in pixels */
  compress_info.image_height = options->scale_height;
  compress_info.input_components = 3;   /* # of color components per pixel */
  compress_info.in_color_space = JCS_RGB;   /* colorspace of input image */


  /* Now use the library's routine to set default compression parameters.
   * (You must set at least compress_info.in_color_space before calling this,
   * since the defaults depend on the source color space.)
   */
  jpeg_set_defaults(&compress_info);
  /* Now you can set any non-default parameters you wish to.
   * Here we just illustrate the use of quality (quantization table) scaling:
   */
  // jpeg_set_quality(&compress_info, quality, TRUE /* limit to baseline-JPEG values */);


  jpeg_start_compress(&compress_info, TRUE);

  row_stride = width * 3; /* JSAMPLEs per row in image_buffer */

  while (compress_info.next_scanline < compress_info.image_height) {
    /* jpeg_write_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could pass
     * more than one scanline at a time if that's more convenient.
     */
    row_pointer[0] = & bmp_buffer[compress_info.next_scanline * row_stride];
    (void) jpeg_write_scanlines(&compress_info, row_pointer, 1);
  }

  /* Step 6: Finish compression */

  jpeg_finish_compress(&compress_info);
  /* After finish_compress, we can close the output file. */

  /* Step 7: release JPEG compression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_compress(&compress_info);

























  syslog(LOG_INFO, "End of decompression");
  return 11;
}

int parse (size_t size, unsigned char *data) {
  int writtendata = 0;
  
  convert_t *opts = (convert_t*) data;
  
  size -= sizeof(convert_t);
  data += sizeof(convert_t);

  // here we convert the file
  int status = convert(size, data, opts);

  io_write(out_size, pCompressed);
syslog(LOG_INFO, "Proc: written  bytes of data");


  //}
  //destroy(input, output);
  //free(pCompressed);
  //out_size = 0;


  return 10;
}

int main (int argc, char *argv[]) {
  while(1){
    io_read(parse);

  } 
  return 1;
}
