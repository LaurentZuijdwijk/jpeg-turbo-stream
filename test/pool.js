var tape = require('tape')
var fixtures = require('./fixtures')
var gm = require('../')

var convert = gm({pool:5})

tape('scale', function(t) { // to test this currently just run a bunch in parallel
  t.plan(80)

  for (var i = 0; i < 10; i++) {
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
  }

})
