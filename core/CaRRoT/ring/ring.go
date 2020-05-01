/*
Copyright 2017-2020 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including without
limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

Author(s): Roman Bendt, Thomas Riedmaier
*/

package ring

type Bytes struct {
	windowSize     uint64
	bufferSize     uint64
	windowPosition uint64
	buffer         []byte
}

func New(windowsize uint64) *Bytes {
	var b Bytes
	b.windowSize = windowsize
	b.bufferSize = windowsize * 100
	b.windowPosition = (b.bufferSize - b.windowSize)
	b.buffer = make([]byte, b.bufferSize)
	return &b
}

func (b *Bytes) WindowSize() (r uint64) {
	return b.windowSize
}

func (b *Bytes) Window() (r []byte) {
	if b.windowPosition+b.windowSize < b.bufferSize {
		return b.buffer[b.windowPosition : b.windowPosition+b.windowSize]
	} else {
		nb := make([]byte, b.windowSize)
		for i := uint64(0); i < b.windowSize; i++ {
			nb[i] = b.buffer[(b.windowPosition+i)%b.bufferSize]
		}
		return nb
	}
}

func (b *Bytes) Push(n byte) {
	b.buffer[(b.windowPosition+b.windowSize)%b.bufferSize] = n
	b.windowPosition = (b.windowPosition + 1) % b.bufferSize
}
