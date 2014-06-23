var gm = require('./')
var fs = require('fs')

var render = gm()
var test = fs.readFileSync('test.png')

render.write({type:'echo', buffer:test})
render.write({type:'scale', width:200, height:200, buffer:test})
render.write({type:'scale', width:500, height:400, buffer:test, ratio:false})

var i = 0;
render.on('data', function(data) {
  var filename= 'test-output-'+(i++)+'.png'
  console.log('rendered', filename)
  fs.writeFileSync(filename, data)
})