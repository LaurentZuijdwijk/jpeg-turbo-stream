var proc = require('child_process')
var through = require('through2')
var duplexer = require('duplexer2')
var path = require('path')

var EMPTY = new Buffer(0)

var toFormatType = function(format) {
  if (!format) return 0

  switch (format.toLowerCase()) {
    case 'jpeg':
    case 'jpg': return 1
    case 'gif': return 2
    case 'png': return 3
    case 'bmp': return 4
  }
  return 0
}

var toUInt32LE = function(len) {
  var buf = new Buffer(4)
  buf.writeUInt32LE(len, 0)
  return buf
}

var toStruct = function(opts) {
  var buf = new Buffer(36)
  var offset = -4

  // scale
  if (typeof opts.scale === 'number') opts.scale = {width:opts.scale, height:opts.scale}
  var scale = opts.scale || {}
  buf.writeUInt32LE(scale.ratio !== false ? 1 : 0, offset += 4)
  buf.writeUInt32LE(scale.width || 0, offset += 4)
  buf.writeUInt32LE(scale.height || 0, offset += 4)

  // crop
  if (typeof opts.crop === 'number') opts.crop = {width:opts.crop, height:opts.crop}
  var crop = opts.crop || {}
  buf.writeUInt32LE(crop.x || 0, offset += 4)
  buf.writeUInt32LE(crop.y || 0, offset += 4)
  buf.writeUInt32LE(crop.width || 0, offset += 4)
  buf.writeUInt32LE(crop.height || 0, offset += 4)

  // rotate
  var degrees = opts.rotate === 'auto' || opts.rotate === true ? 360 : (opts.rotate || 0)
  buf.writeUInt32LE(degrees < 0 ? 360 + degrees : degrees, offset += 4)

  // output format
  buf.writeUInt32LE(toFormatType(opts.format), offset += 4)

  return buf
}

var spawn = function() {
  var input = through.obj(function(data, enc, cb) {
    var buf = data.buffer
    var opts = toStruct(data)

    this.push(toUInt32LE(buf.length+opts.length))
    this.push(opts)
    this.push(buf)

    cb()
  })

  var buffer = through()
  var size = 0
  var output = through.obj(function(data, enc, cb) {
    buffer.write(data)

    while (true) {
      var want = size || 4
      var block = buffer.read(want)
      if (!block) return cb()

      if (size) {
        size = 0
        this.push(block)
      } else {
        size = block.readUInt32LE(0)
      }
    }
  })

  var child = proc.spawn(path.join(__dirname, './bin/convert'))

  child.stderr.pipe(process.stdout)

  input.pipe(child.stdin)
  child.stdout.pipe(output)

  return duplexer(input, output)
}

module.exports = function() {
  return spawn()
}