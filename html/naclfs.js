/**
 * Copyright (c) 2012, Takashi TOYOSHIMA <toyoshim@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * - Neither the name of the authors nor the names of its contributors may be
 *   used to endorse or promote products derived from this software with out
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUE
 * NTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

function naclfs (element) {
  this._element = element;
  this._out_buffer = '';
  this._err_buffer = '';
}

naclfs.prototype.handleMessage = function (message_event) {
  var data = message_event.data;
  if (!data)
    return;
  var type = data[0];
  switch (type) {
   case 'E':  // Internal error log.
    this.appendConsole(data.slice(1));
    break;
   case 'S':  // Std outputs.
    var id = data[1];
    if (1 == id)
      this.appendStdOut(data.slice(2));
    else if (2 == id)
      this.appendStdErr(data.slice(2));
    else
      this.appendConsole('port ' + id + ': ' + data.slice(2));
    break;
   case 'X':  // RPC request.
    this._dispatchRPC(data);
    break;
  }
};

naclfs.prototype.appendConsole = function (message) {
  console.log(message);
};

naclfs.prototype.appendStdOut = function (message) {
  this._out_buffer += message;
  var lines = this._out_buffer.split('\n');
  for (var i = 0; i < (lines.length - 1); ++i)
    console.warn(lines[i]);
  this._out_buffer = lines[i];
};

naclfs.prototype.appendStdErr = function (message) {
  this._err_buffer += message;
  var lines = this._err_buffer.split('\n');
  for (var i = 0; i < (lines.length - 1); ++i)
    console.error(lines[i]);
  this._err_buffer = lines[i];
};

naclfs.prototype.onkeypress = function (e) {
  this._element.postMessage('S0' + String.fromCharCode(e.which));
};

naclfs.prototype.onkeydown = function (e) {
  if (e.which == 8) {
    this._element.postMessage('S0' + String.fromCharCode(e.which));
    return false;
  }
  return true;
};

naclfs.prototype.onkeyup = function (e) {
  return false;
};

naclfs.prototype._dispatchRPC = function (data) {
  if (data[1] != '5') {
    console.error('unknown RPC: ' + data);
    return;
  }
  var cmd = data[2];
  if (cmd == 'D')
    this._dirlist(data.slice(3));
};

naclfs.prototype._dirlist = function (path) {
  console.info('naclfs rpc: directory list request; path=' + path);
  if (!naclfs.fs) {
    console.info(' => -1');
    this._element.postMessage('X5D_/');  // return -1
    return;
  }
  naclfs.fs.root.getDirectory(path, {}, function (entry) {
    var reader = entry.createReader();
    reader.readEntries(function (entries) {
      for (var i = 0; i < entries.length; ++i) {
        var entry = entries[i];
        var rpc = 'X5D';
        if (entry.isDirectory)
          rpc += 'D';
        else if (entry.isFile)
          rpc += 'F';
        else
          rpc += ' ';
        rpc += entry.name;
        this._element.postMessage(rpc);
      }
      this._element.postMessage('X5D_0');  // return 0
    }.bind(this), function () {
      console.info(' => -1');
      this._element.postMessage('X5D_/');  // return -1
    }.bind(this));
  }.bind(this), function () {
    console.info(' => -1');
    this._element.postMessage('X5D_/');  // return -1
  }.bind(this));
};

naclfs.fs = null;

window.webkitRequestFileSystem(window.TEMPORARY, 1024 * 1024, function (fs) {
  naclfs.fs = fs;
}, function () {
  console.error("RequestFileSystem failed");
});

