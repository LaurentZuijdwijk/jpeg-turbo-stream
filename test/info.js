var tape = require('tape')
var fixtures = require('./fixtures')
var gm = require('../')

var convert = gm()

tape('info', function(t) {
  t.test('gif', function(t) {
    fixtures.gif().pipe(convert.info(function(err, data) {
      t.error(err, 'should not return an error')
      t.equals(data.width, 640, 'should return the correct width')
      t.equals(data.height, 480, 'should return the correct height')
      t.equals(data.format, 'gif', 'should get expected format')
      t.end()
    }))
  })

  t.test('png', function(t) {
    fixtures.png().pipe(convert.info(function(err, data) {
      t.error(err, 'should not return an error')
      t.equals(data.width, 640, 'should return the correct width')
      t.equals(data.height, 480, 'should return the correct height')
      t.equals(data.format, 'png', 'should get expected format')
      t.end()
    }))
  })

  t.test('jpeg', function(t) {
    fixtures.jpeg().pipe(convert.info(function(err, data) {
      t.error(err, 'should not return an error')
      t.equals(data.width, 640, 'should return the correct width')
      t.equals(data.height, 480, 'should return the correct height')
      t.equals(data.format, 'jpeg', 'should get expected format')
      t.end()
    }))
  })
})
