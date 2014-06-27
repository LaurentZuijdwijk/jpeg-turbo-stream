var tape = require('tape')
var fixtures = require('./fixtures')
var gm = require('../')

var convert = gm()

tape('scale', function(t) {
  t.plan(8)

  fixtures.gif()
    .pipe(convert({scale:300}))
    .pipe(convert.info(function(err, data) {
      t.ok(!err)
      t.same(data.width, 300)
      t.same(data.height, 225) // should respect ratio
      t.same(data.format, 'gif')
    }))

  fixtures.png()
    .pipe(convert({scale:{width:300, height:300, ratio:false}}))
    .pipe(convert.info(function(err, data) {
      t.ok(!err)
      t.same(data.width, 300)
      t.same(data.height, 300)
      t.same(data.format, 'png')
    }))
})

tape('crop', function(t) {
  t.plan(4)

  fixtures.gif()
    .pipe(convert({crop:300}))
    .pipe(convert.info(function(err, data) {
      t.ok(!err)
      t.same(data.width, 300)
      t.same(data.height, 300)
      t.same(data.format, 'gif')
    }))
})

tape('change format', function(t) {
  t.plan(4)

  fixtures.gif()
    .pipe(convert({format:'png'}))
    .pipe(convert.info(function(err, data) {
      t.ok(!err)
      t.same(data.format, 'png')
    }))

  fixtures.png()
    .pipe(convert({format:'png'}))
    .pipe(convert.info(function(err, data) {
      t.ok(!err)
      t.same(data.format, 'png')
    }))
})

tape('rotate', function(t) {
  t.plan(4)

  fixtures.jpeg()
    .pipe(convert({rotate:90}))
    .pipe(convert.info(function(err, data) {
      t.ok(!err)
      t.same(data.width, 480)
      t.same(data.height, 640)
      t.same(data.format, 'jpeg')
    }))
})

tape('scale and rotate', function(t) {
  t.plan(4)

  fixtures.jpeg()
    .pipe(convert({scale:300, rotate:90}))
    .pipe(convert.info(function(err, data) {
      t.ok(!err)
      t.same(data.width, 225)
      t.same(data.height, 300)
      t.same(data.format, 'jpeg')
    }))
})