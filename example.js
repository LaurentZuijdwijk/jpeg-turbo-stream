var gm = require('./')
var fs = require('fs')

var convert = gm()

fs.createReadStream('./output.jpg')
  .pipe(convert({scale: {width: 200, height: 200}}))
  .pipe(fs.createWriteStream('output2.jpg'))

// fs.createReadStream('./test/fixtures/test.jpeg')
//   .pipe(convert({scale:300, rotate:180, format:'png'}))
//   .pipe(fs.createWriteStream('output1.png'))
