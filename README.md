# graphicsmagick-stream

Fast convertion/scaling of images using a pool of long lived graphicsmagick processes

```shell
npm install graphicsmagick-stream
```

[![Build Status](https://travis-ci.org/e-conomic/graphicsmagick-stream.png)](https://travis-ci.org/e-conomic/graphicsmagick-stream)

It works by spawning and reusing a custom graphicsmagick processes (see src/) that
accepts images over stdin and pipes out the converted result over stdout

## Usage

```js
var gm = require('graphicsmagick-stream')
var fs = require('fs')

var convert = gm({
  pool: 5,             // how many graphicsmagick processes to use
  format: 'png',       // format to convert to
  scale: {
    width: 200,        // scale input to this width
    height: 200,       // scale input this height
    type: 'contain'    // scale type (either contain/cover/fixed)
  },
  crop: {
    width: 200,        // crop input to this width
    height: 200,       // crop input this height
    x: 0,              // crop using this x offset
    y: 0               // crop using this y offset
  },
  page: [1,5],         // only render page 1 to 5 (for pdfs)
                       // set to a single number if you only want to render one page
                       // or omit if you want all pages
  rotate: 'auto',      // auto rotate image based on exif data
                       // or use rotate:degrees
  density: 300,        // set the image density. useful when converting pdf to images
  split: false,        // when converting pdfs into images it is possible to split
                       // into multiple pages. If set to true the resulting file will
                       // be a tar containing all the images.
  tar: false           // stream a tar containing the image. This is forced to `true`
                       // if split is set to `true`
})


fs.createReadStream('input.jpg')
  .pipe(convert({
    // override any of the above options here
  }))
  .pipe(fs.createWriteStream('output.jpg'))
```

You do not need to set all the options. If you only want to scale an image do

```js
var stream = convert({
  scale: {
    width: 400,
    height: 300
  }
})
```

You can also use it to get metadata info about an image using `convert.info`

``` js
var info = convert.info(function(err, info) {
  console.log(info) // prints something like {width:500, height:400, format:'png'}
})

fs.createReadStream('input.jpg').pipe(info)
```

For more examples and usage see the test folder


## Scale types

* `contain` sets the scaled image to maximum have a width/height of the scale box. Always respects ratio.
* `cover` sets the scaled image to at least have one of the width/height within the scale box. Always respects ratio.
* `fixed` sets the scaled image to precisely the given width/height of the scale box. If both width/height is given it does not respect the ratio.


## PDF Conversion

If you install ghostscript as well you will be able to convert pdfs to images by simply piping in a pdf and setting output format to `jpeg` (or another image format).

If you are rendering a multipage pdf `scale.height` will set the height of each page. To force `scale.height` to donate the height of the entire image set `scale.multipage = true`.

Use `split = true` to output each page as an image file. This will result in a tar file containing all the images, so you will need to untar them on the other end. Use a project like [tar-stream](https://www.npmjs.com/package/tar-stream) to achieve this.


## Dependencies

You need to install libgraphicsmagicks in order to compile this.


### Using OS X and Homebrew

```shell
brew install graphicsmagick --build-from-source
brew install libarchive
```

You will have to build the binary using the following command

```shell
gcc src/*.c -o bin/convert -L/usr/local/opt/libarchive/lib -I/usr/local/opt/libarchive/include -larchive -O `GraphicsMagickWand-config --cflags --cppflags --ldflags --libs`
```


### Using Ubuntu

```shell
sudo apt-get install build-essential libgraphicsmagick++1-dev libarchive-dev
```

Then `npm install` should work.


## License

MIT
