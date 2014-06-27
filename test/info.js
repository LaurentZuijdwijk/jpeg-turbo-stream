var tape = require('tape')
var fixtures = require('./fixtures')
var gm = require('../')

var convert = gm()

tape('gif', function(t) {
  fixtures.gif().pipe(convert.info(function(err, data) {
    t.ok(!err)
    t.same(data.width, 640)
    t.same(data.height, 480)
    t.same(data.format, 'gif')
    t.end()
  }))
})

tape('png', function(t) {
  fixtures.png().pipe(convert.info(function(err, data) {
    t.ok(!err)
    t.same(data.width, 640)
    t.same(data.height, 480)
    t.same(data.format, 'png')
    t.end()
  }))
})

tape('bmp', function(t) {
  fixtures.bmp().pipe(convert.info(function(err, data) {
    t.ok(!err)
    t.same(data.width, 640)
    t.same(data.height, 480)
    t.same(data.format, 'bmp')
    t.end()
  }))
})

tape('jpeg', function(t) {
  fixtures.jpeg().pipe(convert.info(function(err, data) {
    t.ok(!err)
    t.same(data.width, 640)
    t.same(data.height, 480)
    t.same(data.format, 'jpeg')
    t.end()
  }))
})