var fs = require('fs')
var path = require('path')

exports.gif = function() {
  return fs.createReadStream(path.join(__dirname, 'test.gif'))
}

exports.jpeg = function() {
  return fs.createReadStream(path.join(__dirname, 'test.jpeg'))
}

exports.png = function() {
  return fs.createReadStream(path.join(__dirname, 'test.png'))
}

exports.bmp = function() {
  return fs.createReadStream(path.join(__dirname, 'test.bmp'))
}

exports.pdf = function() {
  return fs.createReadStream(path.join(__dirname, 'test.pdf'))
}
