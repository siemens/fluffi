§§/*
§§Copyright 2017-2019 Siemens AG
§§
§§Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§
§§The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§
§§THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§
§§Author(s): Roman Bendt, Thomas Riedmaier
§§*/
§§
§§package ring
§§
§§import (
§§	"bytes"
§§	"testing"
§§)
§§
§§func TestConstruction(t *testing.T) {
§§	b := New(4)
§§	if b.bufferSize != uint64(400) {
§§		t.Error("Expected buffer size not matched")
§§	}
§§	if b.windowPosition != uint64(396) {
§§		t.Error("Expected window position not matched")
§§	}
§§	if uint64(len(b.buffer)) != b.bufferSize {
§§		t.Error("Buffer size mismatch")
§§	}
§§	if b.windowSize != uint64(4) {
§§		t.Error("Expected window size not matched")
§§	}
§§}
§§
§§func TestPush(t *testing.T) {
§§	b := New(4)
§§	b.Push(0x42)
§§	if b.buffer[0] != 0x42 {
§§		t.Error("Initial push failed")
§§	}
§§	if b.windowPosition != uint64(397) {
§§		t.Error("Expected window position not matched")
§§	}
§§
§§	b.Push(0x44)
§§	if b.buffer[1] != 0x44 {
§§		t.Error("Consecutive push failed")
§§	}
§§	if b.windowPosition != uint64(398) {
§§		t.Error("Expected window position not matched")
§§	}
§§
§§	b.Push(0x46)
§§	if b.buffer[2] != 0x46 {
§§		t.Error("Consecutive push failed")
§§	}
§§	if b.windowPosition != uint64(399) {
§§		t.Error("Expected window position not matched")
§§	}
§§
§§	b.Push(0x48)
§§	if b.buffer[3] != 0x48 {
§§		t.Error("Consecutive push failed")
§§	}
§§	if b.windowPosition != uint64(0) {
§§		t.Error("Expected window position not matched")
§§	}
§§	for i := uint64(0); i < uint64(100); i++ {
§§		b.Push(byte(i))
§§	}
§§
§§	if b.buffer[54] != 50 {
§§		t.Error("Repeated push failed")
§§	}
§§}
§§
§§func TestWindow(t *testing.T) {
§§	b := New(4)
§§	b.Push(0x48)
§§	if 0 != bytes.Compare(b.Window(), []byte{0, 0, 0, 0x48}) {
§§		t.Error("Compare on window mismatch")
§§	}
§§	b.Push(0x46)
§§	if 0 != bytes.Compare(b.Window(), []byte{0, 0, 0x48, 0x46}) {
§§		t.Error("Compare on window mismatch")
§§	}
§§	b.Push(0x44)
§§	if 0 != bytes.Compare(b.Window(), []byte{0, 0x48, 0x46, 0x44}) {
§§		t.Error("Compare on window mismatch")
§§	}
§§	b.Push(0x42)
§§	if 0 != bytes.Compare(b.Window(), []byte{0x48, 0x46, 0x44, 0x42}) {
§§		t.Error("Compare on window mismatch")
§§	}
§§	b.Push(0x40)
§§	if 0 != bytes.Compare(b.Window(), []byte{0x46, 0x44, 0x42, 0x40}) {
§§		t.Error("Compare on window mismatch")
§§	}
§§}
§§
§§func TestRollover(t *testing.T) {
§§	b := New(4)
§§	for i := uint64(0); i < uint64(1000); i++ {
§§		b.Push(byte(i % 255))
§§	}
§§
§§	b.Push(0x48)
§§	b.Push(0x46)
§§	b.Push(0x44)
§§	b.Push(0x42)
§§	if 0 != bytes.Compare(b.Window(), []byte{0x48, 0x46, 0x44, 0x42}) {
§§		t.Error("Compare on window mismatch")
§§	}
§§}
