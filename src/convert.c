

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/stat.h>
#include <math.h>

#include <jpeglib.h>
#include "io.h"
#include <archive.h>
#include <archive_entry.h>
#include "debug.h"
#include <vips/vips.h>


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

    uint32_t quality;
} convert_t;

unsigned long out_size = 0;
  //free(data);



double modulus(double a, double b)
{
    int result = (int)( a / b );
    return a - (double)result * b;
}

//our main convert function
int convert (size_t size, unsigned char* data, convert_t* options, unsigned char** pCompressed) {
    int rc, i, j;

    // Init syslog for degugging
    char *syslog_prefix = (char*) malloc(1024);
    openlog(syslog_prefix, LOG_PERROR | LOG_PID, LOG_USER);

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    // Variables for the output buffer, and how long each row is
    unsigned long bmp_size;
    unsigned char *bmp_buffer;
    int row_stride, width, height, pixel_size;

    cinfo.err = jpeg_std_error(&jerr);  
    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, data, size);



    // syslog(LOG_INFO, "Proc: Read the JPEG header");
    rc = jpeg_read_header(&cinfo, TRUE);

    if (rc != 1) {
        syslog(LOG_ERR, "File does not seem to be a normal JPEG");
        exit(EXIT_FAILURE);
    }

    // only scaling supported is 1/16 or bigger. 
    // We should scale the max possible and then scale usig grapgicsmagic api.

    double applied_scale_factor;
    double MAX_SCALE = 0.125;
    double scale_factor = cinfo.image_width / options->scale_width;
    double _modulus = modulus((1/scale_factor), (1/8));
    double scale = 1/scale_factor;


    if(modulus((1/scale_factor), MAX_SCALE) == 0){
        applied_scale_factor = 1/scale_factor;
    }
    else if(scale < MAX_SCALE){
        applied_scale_factor = MAX_SCALE;
    }
    else{
        applied_scale_factor = (int)(scale/MAX_SCALE + 0.5) * MAX_SCALE;
    }
     syslog(LOG_INFO, "Proc: resizing  scale factor %f, applied_scale_factor:  %f ,Modulo %f, scale %f", 
         scale, applied_scale_factor, _modulus, scale);
    cinfo.scale_num = 1;
    cinfo.scale_denom = (int)(1/applied_scale_factor);

    jpeg_start_decompress(&cinfo);

    width = cinfo.image_width * applied_scale_factor;
    height = cinfo.image_height * applied_scale_factor;

    pixel_size = cinfo.output_components;


    bmp_size = width * height * pixel_size;
    bmp_buffer = (unsigned char*) malloc(bmp_size);

    // syslog(LOG_INFO, "Proc: Image is %d by %d with %d components %d", 
    //     width, height, pixel_size, bmp_size);
    // The row_stride is the total number of bytes it takes to store an
    // entire scanline (row). 
    row_stride = width * pixel_size;


    //syslog(LOG_INFO, "Proc: Start reading scanlines  %d", cinfo.output_height);
    while (cinfo.output_scanline < cinfo.output_height) {
        // syslog(LOG_INFO, "Proc:  scanlines  %d", cinfo.output_height);
        unsigned char *buffer_array[1];
        buffer_array[0] = bmp_buffer + \
                   (cinfo.output_scanline) * row_stride;

        jpeg_read_scanlines(&cinfo, buffer_array, 1);

    }

    jpeg_finish_decompress(&cinfo);

    jpeg_destroy_decompress(&cinfo);



/*******************************8
*
* Do we need to scale or crop using VIPS?
*
*
***************************************/
    VipsImage *vipsImage;
    VipsImage **t;
    vipsImage = vips_image_new_memory();
    unsigned char *resized_buffer;
    size_t _size;

if(options->crop_x || scale != applied_scale_factor ){


    t = (VipsImage **) vips_object_local_array( VIPS_OBJECT( vipsImage ), 5 );

    if( !(t[0] = vips_image_new_from_memory(bmp_buffer,
        width,
        height,
        3,
        VIPS_FORMAT_UCHAR))){
        syslog(LOG_INFO, "unable to read img" );
    }
    if(options->crop_x){
        if(vips_crop( t[0], &t[1], (int)(options->crop_x*applied_scale_factor), (int)(options->crop_y*applied_scale_factor), (int)(options->crop_width*applied_scale_factor), (int)(options->crop_height*applied_scale_factor), NULL ))
        vips_error_exit( "unable to process crop %G %G %G %G", (int)(options->crop_x*applied_scale_factor), (int)(options->crop_y*applied_scale_factor), (int)(options->crop_width*applied_scale_factor), (int)(options->crop_height*applied_scale_factor) );
    }
    else{
        t[1] = t[0];
    }
    syslog(LOG_INFO," crop %d %d %d %d", (int)(options->crop_x*applied_scale_factor), (int)(options->crop_y*applied_scale_factor), (int)(options->crop_width*applied_scale_factor), (int)(options->crop_height*applied_scale_factor));

    if(         vips_similarity( t[1], &t[4], "scale",0.99, NULL )  ){
        vips_error_exit( "unable to process ");
    }
    // syslog(LOG_INFO, "processed img %d, %d" ,  t[4]->Xsize, t[4]->Ysize );

    if (t[4]->BandFmt != VIPS_FORMAT_UCHAR) {
        syslog(LOG_INFO, "wrong format" );
    }
    _size = t[4]->Xsize * t[4]->Ysize * 3;

    resized_buffer = (unsigned char*) malloc(_size);

    vips_image_write_to_memory(t[4], &resized_buffer, &_size);

    options->scale_width = t[4]->Xsize;
    options->scale_height = t[4]->Ysize;
    // syslog(LOG_INFO,"Here is the message(%d): %x\n\n", sizeof(resized_buffer), resized_buffer);

    width = options->scale_width;
    height = options->scale_height;

    syslog(LOG_INFO, "VIPS width: %d, height: %d" ,  width, height);

    g_object_unref( vipsImage ); 
    g_object_unref( t ); 
    free(bmp_buffer);

}
else{
  resized_buffer = bmp_buffer;
  _size = width*height*3;

options->scale_width = width;
options->scale_height = height;
}

/***************************************
*
* End of VIPS
*
*
***************************************/



// DDDD       OOO    N     N  EEEEEEE
// D  DDD    O   O   NN    N  E      
// D    DD  O     O  N N   N  E      
// D     D  O     O  N N   N  E      
// D     D  O     O  N  N  N  EEEE   
// D     D  O     O  N   N N  E      
// D    DD  O     O  N   N N  E      
// D  DDD    O   O   N    NN  E      
// DDDD       OOO    N     N  EEEEEEE

    struct jpeg_compress_struct compress_info;

    JSAMPROW row_pointer[1];  /* pointer to JSAMPLE row[s] */

    compress_info.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&compress_info);


    jpeg_mem_dest(&compress_info, pCompressed, &out_size);


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

    // syslog(LOG_INFO, "Proc: quality  %d", options->quality);

    if(!options->quality ){
        options->quality = 75;
    }

    jpeg_set_quality(&compress_info, options->quality, TRUE);

    jpeg_start_compress(&compress_info, TRUE);

    row_stride = width * 3; /* JSAMPLEs per row in image_buffer */

    while (compress_info.next_scanline < compress_info.image_height) {
        /* jpeg_write_scanlines expects an array of pointers to scanlines.
         * Here the array is only one element long, but you could pass
         * more than one scanline at a time if that's more convenient.
         */
        row_pointer[0] = & resized_buffer[compress_info.next_scanline * row_stride];
        (void) jpeg_write_scanlines(&compress_info, row_pointer, 1);
    }

    /* Step 6: Finish compression */

    /* After finish_compress, we can close the output file. */

    /* Step 7: release JPEG compression object */

    /* This is an important step since it will release a good deal of memory. */
    jpeg_finish_compress(&compress_info);

    jpeg_destroy_compress(&compress_info);

    free(resized_buffer);
    syslog(LOG_INFO, "VIPS width: %d, height: %d" ,  width, height);

    // syslog(LOG_INFO, "End of decompression");
    return 11;
}


int parse (size_t size, unsigned char *data) {
    

    convert_t *opts = (convert_t*) data;
    unsigned char* pCompressed = 0;
    
    unsigned char* _data = data;

    size -= sizeof(convert_t);
    data += sizeof(convert_t);

    int status = convert(size, data, opts, &pCompressed);
    
    io_write(out_size, pCompressed);
    // _data = realloc(data, size);
     // free(_data);

    free(pCompressed);

    return 1;
}

int main (int argc, char *argv[]) {
    VIPS_INIT(argv[0]);

    return io_read(parse);
}
    