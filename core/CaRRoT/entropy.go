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

package main

import (
	"math"
	"sync"
)

func AnalyzeEntropy(hash string) {
	if db.Files[hash].IsPureAscii || db.Files[hash].IsPureUtf8 || db.Files[hash].IsPureUnicode {
		L.Println("skipping entropy analysis because file is text only")
	} else {
		var edAll, ed10, ed100, ed1000 []float64
		var wg sync.WaitGroup
		go func() {
			wg.Add(1)
			edAll = shannonDigester(db.Files[hash].Content, db.Files[hash].Size)
			wg.Done()
		}()
		go func() {
			wg.Add(1)
			ed10 = shannonDigester(db.Files[hash].Content, db.Files[hash].Size/10)
			wg.Done()
		}()
		go func() {
			wg.Add(1)
			ed100 = shannonDigester(db.Files[hash].Content, db.Files[hash].Size/100)
			wg.Done()
		}()
		go func() {
			wg.Add(1)
			ed1000 = shannonDigester(db.Files[hash].Content, db.Files[hash].Size/1000)
			wg.Done()
		}()
		wg.Wait()
		L.Println("Shannon:", edAll, ed10, ed100, ed1000)
	}
}

func shannonDigester(buf []byte, blocksize uint64) (edist []float64) {
	if blocksize == 0 {
		return
	}
	i := uint64(0)
	entropy := 0.0
	histogram := make(map[byte]int, 256)
	edist = make([]float64, uint64(len(buf))/blocksize)

	for i < uint64(len(buf)) {
		histogram[buf[i]]++
		i++
		if (i % blocksize) == 0 {
			for x := byte(0x00); x <= 255; x++ {
				p_x := float64(histogram[x]) / float64(blocksize)
				if p_x > 0.0 {
					entropy -= p_x * math.Log2(p_x)
				}
			}
			entropy /= 8
			edist = append(edist, entropy)
			L.Println("HIT", entropy)
			entropy = 0.0
			histogram = make(map[byte]int, 256)
		}

	}

	return
}
