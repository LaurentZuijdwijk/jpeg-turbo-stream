# graphicsmagick-stream

Fast convertion/scaling of images using a pool of long lived graphicsmagick processes

```
npm install graphicsmagick-stream
```

It works by spawning and reusing a custom graphicsmagick processes (see src/) that
accepts images over stdin and pipes out the converted result over stdout

## Usage

``` js
var gm = require('graphicsmagick-stream')
var fs = require('fs')

var convert = gm({
  pool: 5,        // how many graphicsmagick processes to use
  format: 'png',  // format to convert to
  scale: {
    width: 200,   // scale input to this width
    height: 200,  // scale input this height
    ratio: true   // keep the aspect ratio (defaults to true)
  },
  crop: {
    width: 200,   // crop input to this width
    height: 200,  // crop input this height
    x: 0,         // crop using this x offset
    y: 0          // crop using this y offset
  },
  rotate: 'auto'  // auto rotate image based on exif data
                  // or use rotate:degrees
})


fs.createReadStream('input.jpg')
  .pipe(convert({
    // override any of the above options here
  }))
  .pipe(fs.createWriteStream('output.jpg'))
```

You do not need to set all the options. If you only want to scale an image do

``` js
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

## Dependencies

You need to install libgraphicsmagicks in order to compile this
On OSX using homebrew you can do

```
brew install graphicsmagick --build-from-source
```

## License

MIT
