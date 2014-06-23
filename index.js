var proc = require('child_process')
var through = require('through2')
var duplexer = require('duplexer2')
var path = require('path')

var headerStruct = function(type, opts, buffer) {
  var header = new Buffer(16)
  var length = buffer.length + opts.length + 12

  header.writeUInt32LE(length, 0)
  header.writeUInt32LE(type, 4)
  header.writeUInt32LE(12, 8)
  header.writeUInt32LE(opts.length+12, 12)

  return header
}

var scaleStruct = function(wid, hei) {
  var buf = new Buffer(8)
  buf.writeUInt32LE(wid, 0)
  buf.writeUInt32LE(hei, 4)
  return buf
}

var spawn = function() {
  var input = through.obj(function(data, enc, cb) {
    var buf = data.buffer
    var opts
    var type

    switch (data.type) {
      case 'scale':
      opts = scaleStruct(data.width, data.height)
      type = 1
      break

      default:
      opts = new Buffer(0)
      type = 0
      break
    }

    this.push(headerStruct(type, opts, buf))
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