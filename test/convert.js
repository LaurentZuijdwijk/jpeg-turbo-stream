var tape = require('tape')
var fixtures = require('./fixtures')
var gm = require('../')
var tar = require('tar-stream')
var convert = gm()

tape('convert', function(t) {
  t.test('scale', function(t) {
    t.test('gifs', function(t) {
      fixtures.gif()
        .pipe(convert({scale:300}))
        .pipe(convert.info(function(err, data) {
          t.error(err, 'should not return error')
          t.equals(data.width, 300, 'with should be correct')
          t.equals(data.height, 225, 'height should respect the apsect ratio')
          t.equals(data.format, 'gif', 'output has the expected format')
          t.end()
        }))
    })

    t.test('pngs', function(t) {
      fixtures.png()
        .pipe(convert({scale:{width:300, height:300, type:'fixed'}}))
        .pipe(convert.info(function(err, data) {
          t.error(err, 'should not return error')
          t.equals(data.width, 300, 'with should be correct')
          t.equals(data.height, 300, 'height should be correct')
          t.equals(data.format, 'png', 'output has the expected format')
          t.end()
        }))
    })
  })

  t.test('crop', function(t) {
    t.test('gifs', function(t) {
      fixtures.gif()
        .pipe(convert({crop:300}))
        .pipe(convert.info(function(err, data) {
          t.error(err, 'should not return error')
          t.equals(data.width, 300, 'width should be correct')
          t.equals(data.height, 300, 'height should be correct')
          t.equals(data.format, 'gif', 'should keep its format')
          t.end()
        }))
    })
  })

  t.test('change format', function(t) {
    t.test('gif to png', function(t) {
      fixtures.gif()
        .pipe(convert({format:'png'}))
        .pipe(convert.info(function(err, data) {
          t.error(err, 'should not return error')
          t.equals(data.format, 'png', 'output should be an png')
          t.end()
        }))
    })

    t.test('png to png', function(t) {
      fixtures.png()
        .pipe(convert({format:'png'}))
        .pipe(convert.info(function(err, data) {
          t.error(err, 'should not return error')
          t.equals(data.format, 'png', 'output should be a png')
          t.end()
        }))
    })

    t.test('pdf to pdf', function(t) {
      fixtures.pdf()
        .pipe(convert({format:'pdf', split:false}))
        .pipe(convert.info(function(err, data) {
          t.error(err, 'should not return error')
          t.equals(data.format, 'pdf', 'output should be a pdf')
          t.end()
        }))
    })

    t.test('pdf to split pdf', function(t) {
      fixtures.pdf()
        .pipe(convert({format:'pdf', split:true}))
        // untar it before we send it to convert.info
        .pipe(tar.extract().on('entry', function (header, stream, callback) {
          stream.pipe(convert.info(function(err, data) {
            t.error(err, 'should not return error')
            t.equals(data.format, 'pdf', 'content of tar file should be a pdf file')
            t.end()
          }))
        }))
    })
  })

  t.test('rotate', function(t) {
    t.test('jpeg', function(t) {
      fixtures.jpeg()
        .pipe(convert({rotate: 90}))
        .pipe(convert.info(function(err, data) {
          t.error(err, 'should not return error')
          t.equals(data.width, 480, 'should have the correct width')
          t.equals(data.height, 640, 'should have the correct height')
          t.equals(data.format, 'jpeg', 'should still be a jpeg file')
          t.end()
        }))
    })
  })

  t.test('scale and rotate', function(t) {
    t.test('jpeg', function(t) {
      fixtures.jpeg()
        .pipe(convert({scale:300, rotate:90}))
        .pipe(convert.info(function(err, data) {
          t.error(err, 'should not return error')
          t.equals(data.height, 300, 'should have the correct height')
          t.equals(data.width, 225, 'should keep the aspect in the width')
          t.same(data.format, 'jpeg', 'should still be a jpeg file')
          t.end()
        }))
    })
  })
})
