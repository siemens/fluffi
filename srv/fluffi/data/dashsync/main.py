# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Michael Kraus, Thomas Riedmaier, Junes Najah

from flask import Flask, render_template
§§from flask_socketio import SocketIO, emit

# initialize Flask
app = Flask(__name__)
socketio = SocketIO(app)

@app.route('/')
def index():
    """Serve the index HTML"""
    return render_template('index.html')
	

@socketio.on('syncProject')
def on_syncProject(data):
§§    send=data['send']
§§    project=data['project']
§§    time=data['time']
§§    time2=data['time2']
§§    
§§    emit('syncevent', {'send': send, 'project': project, 'time': time, 'time2': time2}, broadcast=True)
§§
if __name__ == '__main__':
    socketio.run(app, debug=True, host='0.0.0.0', port=4000)
