var gm = require('./')
var fs = require('fs')

var convert = gm()

fs.createReadStream('photo.jpg')
  .pipe(convert({scale:{width:200,height:200}}))
  .pipe(fs.createWriteStream('output.jpg'))

fs.createReadStream('photo.jpg')
  .pipe(convert({scale:300, rotate:180, format:'png'}))
  .pipe(fs.createWriteStream('output1.png'))
