var gm = require('./')
var fs = require('fs')

var convert = gm()

fs.createReadStream('./in.jpg')
  .pipe(convert({scale: {width: 1000, height: 1000}}))
  .pipe(fs.createWriteStream('output2.jpg'))

setTimeout(function () {}, 5000)

// fs.createReadStream('./test/fixtures/test.jpeg')
//   .pipe(convert({scale:300, rotate:180, format:'png'}))
//   .pipe(fs.createWriteStream('output1.png'))
