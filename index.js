var proc = require('child_process')
var once = require('once')
var xtend = require('xtend')
var Duplex = require('stream').Duplex
var path = require('path')

var noop = function() {}
var EMPTY = new Buffer(0)
var FORMATS = ['noop', 'info', 'jpeg', 'gif', 'png', 'bmp']

var toFormatType = function(format) {
  if (!format) return 0

  switch (format.toLowerCase()) {
    case 'info': return 1
    case 'jpeg':
    case 'jpg':  return 2
    case 'gif':  return 3
    case 'png':  return 4
    case 'bmp':  return 5
  }
  return 0
}

var fromInfoStruct = function(data) {
  var result = {}
  result.width = data.readUInt32LE(0)
  result.height = data.readUInt32LE(4)
  result.format = FORMATS[data.readUInt32LE(8)]
  return result
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

var destroyer = function(stream) {
  var destroyed = false
  return function(err) {
    if (destroyed) return
    if (err) stream.emit('error', err)
    destroyed = true
    stream.emit('close')
  }
}

var pool = function(opts) {
  if (!opts) opts = {}

  var size = opts.size || 1
  var workers = []
  for (var i = 0; i < size; i++) workers[i] = {queue:[], process:null}

  var update = function(worker) {
    if (!worker.process) return

    if (worker.queue.length) {
      worker.process.ref()
      worker.process.stdout.ref()
      worker.process.stderr.ref()
      worker.process.stdin.ref()
    } else {
      worker.process.unref()
      worker.process.stdout.unref()
      worker.process.stderr.unref()
      worker.process.stdin.unref()
    }
  }

  var select = function() {
    var worker = workers.reduce(function(a, b) {
      return a.queue.length <= b.queue.length ? a : b
    })

    if (worker.process) return worker

    var child = worker.process = proc.spawn(path.join(__dirname, 'bin/convert'))

    var onerror = once(function(err) {
      child.kill()
    })

    child.on('exit', function(code) {
      var err = new Error('graphicsmagick crashed with code: '+code)
      if (stream) stream.destroy(err)
      while (worker.queue.length) worker.queue.shift()(err)
      worker.process = null
    })

    child.stdout.on('error', onerror)
    child.stderr.on('error', onerror)
    child.stdin.on('error', onerror)

    var missing = 0
    var stream

    var draining = false
    var drain = function() {
      if (draining) return
      draining = true

      while (true) {
        if (!missing) {
          var buf = child.stdout.read(4)
          if (!buf) break
          missing = buf.readUInt32LE(0)
          stream = worker.queue[0]
          stream._read = drain
        }

        var block = child.stdout.read(Math.min(missing, child.stdout._readableState.length))
        if (!block) break

        missing -= block.length
        var drained = stream.push(block)

        if (!missing) {
          worker.queue.shift()
          stream.push(null)
          stream._read = noop
          if (worker.queue[0]) worker.queue[0].kick()
          update(worker)
        }

        if (!drained) break
      }

      draining = false
    }

    child.stdout.on('readable', drain)

    return worker
  }

  return function(opts) {
    var worker = select()
    var dup = new Duplex()
    var buffer = [toStruct(opts)]
    var destroyed = false
    var wait

    dup.on('finish', function() {
      buffer = Buffer.concat(buffer)
      worker.process.stdin.write(toUInt32LE(buffer.length))
      worker.process.stdin.write(buffer)
    })

    dup._read = noop
    dup._write = function(data, enc, cb) {
      if (worker.queue[0] !== dup) {
        wait = [data, cb]
        return
      }

      buffer.push(data)
      cb()
    }

    dup.kick = function() {
      var w = wait
      wait = null
      if (w) dup._write(w[0], null, w[1])
    }

    dup.destroy = function(err) {
      if (destroyed) return
      destroyed = true
      if (err) dup.emit('error', err)
      dup.emit('close')
    }

    worker.queue.push(dup)
    update(worker)

    return dup
  }
}

module.exports = function(defaults) {
  if (!defaults) defaults = {}

  var size = defaults.pool || 1
  var exec = pool({size:size})

  var convert = function(opts) {
    return exec(xtend(defaults, opts))
  }

  convert.info = function(opts, cb) {
    if (typeof opts === 'function') return convert.info(null, opts)
    if (!opts) opts = {}

    opts.format = 'info'
    cb = once(cb)
    var buf = []

    return convert(opts)
      .on('error', cb)
      .on('data', function(data) {
        buf.push(data)
      })
      .on('end', function() {
        cb(null, fromInfoStruct(Buffer.concat(buf)))
      })
  }

  return convert
}