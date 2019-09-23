/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Roman Bendt, Thomas Riedmaier
*/

§§package main
§§
§§import (
§§	"io/ioutil"
§§	"log"
§§	"testing"
§§)
§§
§§func BenchmarkInt2String(b *testing.B) {
§§	db = NewDB()
§§	f := NewFile()
§§	L = log.New(ioutil.Discard, "", log.Ldate|log.Ltime)
§§	D = L
§§	for i := uint64(0); i < uint64(b.N); i++ {
§§		f.Stringz.Add(Text{Enc: "ascii", Len: 20, Pos: 2 * (i + 1)})
§§		f.Intz.Add(Int{Enc: "uint32be", ByteW: 4, Pos: i, ValS: int64(i % 50), ValU: i % 100})
§§	}
§§	f.Size = uint64(2*(b.N+1) + 1)
§§	db.Files["test"] = f
§§	b.ResetTimer()
§§	CorrelateIntStrings("test")
§§}
