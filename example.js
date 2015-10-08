var gm = require('./')
var fs = require('fs')

var gmm = require('graphicsmagick-stream');

var convert = gm({pool: 1})
var convert2 = gmm({pool: 1})
// image size 3456x2592
var server = new require('http').Server();
server.listen('8080');
server.on('request', function (req, res) {
    console.log('request')
    fs.createReadStream('./in.jpg')
    .pipe(convert2({crop: {
        x: 50, y: 50, width: 1000, height: 1000

    }, scale: {width: 500, height: 500}, quality: 90}))
    .pipe(res)
})

// fs.createReadStream('./test/fixtures/test.jpeg')
//   .pipe(convert({scale:300, rotate:180, format:'png'}))
//   .pipe(fs.createWriteStream('output1.png'))
