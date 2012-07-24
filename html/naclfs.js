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
  // Do nothing.
};

naclfs.prototype.appendStdErr = function (message) {
  // Do nothing.
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
  if (cmd == 'S')
    this._stat(data.slice(3));
};

naclfs.prototype._stat = function (path) {
  console.info('naclfs rpc: directory stat request; path=' + path);
  if (!naclfs.fs) {
    this._element.postMessage('X5S/');  // return -1
    return;
  }
  naclfs.fs.root.getDirectory(path, {}, function () {
    console.info(' => 0');
    this._element.postMessage('X5S0');  // return 0
  }.bind(this), function () {
    console.info(' => -1');
    this._element.postMessage('X5S/');  // return -1
  }.bind(this));
};

naclfs.fs = null;

window.webkitRequestFileSystem(window.TEMPORARY, 1024 * 1024, function (fs) {
  naclfs.fs = fs;
}, function () {
  console.error("RequestFileSystem failed");
});

function naclfs (element) {
  this._element = element;
}

