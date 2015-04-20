var tape = require('tape')
var fixtures = require('./fixtures')
var gm = require('../')

var convert = gm({pool:5})

tape('poll', function(t) {
  // to test this currently just run a bunch in parallel
  var iterations = 100
  var runs = 0

  function cb(err, data) {
    if (err) t.fail('failed after ' + runs + ' runs.')
    runs += 1
    if (runs === iterations) {
      t.pass('success, ran ' + runs + ' runs without failures')
      t.end()
    }
  }

  for (var i = 0; i < iterations; i += 1) {
    fixtures.gif()
      .pipe(convert({scale: 300}))
      .pipe(convert.info(cb))

    fixtures.png()
      .pipe(convert({scale:{width: 300, height: 300, type: 'fixed'}}))
      .pipe(convert.info(cb))
  }
})
