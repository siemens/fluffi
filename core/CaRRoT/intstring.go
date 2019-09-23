/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Roman Bendt, Thomas Riedmaier
*/

package main

import (
	"math"
	"runtime"
	//"strings"
)

func CorrelateGroupStrings(hash string) {
	L.Println("correlating strings with groups")
	strs := db.Files[hash].Stringz.S
	ints := db.Files[hash].Intz.I
	intbreaks := db.Files[hash].Intz.Breaks
	strbreaks := db.Files[hash].Stringz.Breaks

	sams := 1
	cpus := int(math.Min(float64(len(strs)), float64(runtime.NumCPU()/2-sams)))
	c := make(chan IntStringGroupCorrelation, 10)

	blockchan := make(chan Block, 1000)
	done := make(chan bool, sams+cpus)

	D.Println("forking", cpus, "workers")
	for i := 0; i < cpus; i++ {
		go groupWorker(c, done, blockchan, hash)
	}

	for i := 0; i < sams; i++ {
		go collector(blockchan, done, hash)
	}

	for _, br := range intbreaks {
		if br[1]-br[0] < 3 {
			continue
		}
		for _, st := range strbreaks {
			// first, sort out a little.
			// if highest integer comes after lowest string, no match.
			if ints[br[1]-1].Pos > strs[st[0]].Pos {
				continue
			}

			// here
			b := IntStringGroupCorrelation{IntBreaks: br, StrBreaks: st}
			c <- b
		}
	}
	close(c)
	D.Println("sending blocks is done. collecting workers")
	for i := 0; i < cpus; i++ {
		<-done
	}
	for {
		if len(blockchan) == 0 {
			break
		}
	}
	close(blockchan)

	for i := 0; i < sams; i++ {
		<-done
	}
	D.Println("CorrelateGroupStrings is done")
}

func groupWorker(ch chan IntStringGroupCorrelation, done chan bool, doneblockchan chan Block, hash string) {
	defer func() { done <- true }()
	strs := db.Files[hash].Stringz.S
	ints := db.Files[hash].Intz.I
	for {
		select {
		case isc, more := <-ch:
			if !more {
				return
			}
			st := isc.StrBreaks
			br := isc.IntBreaks
			max := 0
			mapofhist := make(map[uint64]int)
			for _, s := range strs[st[0]:st[1]] {
				for _, z := range ints[br[0]:br[1]] {
					// intbreak x stringbreak, collect sample histogram. if any peak is higher than half (?), we might have a find
					// int is larger than its own byte width and smaller than the string position
					if z.ValU >= z.ByteW && z.ValU <= s.Pos {
						mapofhist[s.Pos-z.ValU]++
						if mapofhist[s.Pos-z.ValU] > max {
							max = mapofhist[s.Pos-z.ValU]
						}
					}
				}
			}
			// if the bases do not accumulate, we have nothing here
			if max <= 1 || float64(max)-0.1 < math.Ceil(float64(len(mapofhist))/2.0) {
				continue
			}
			// if we are here, chances are good we found an offset-string group pair.
			// create new block and send it on the channel
			var tmp []Text
			copy(tmp, strs[st[0]:st[1]])

			var ntmp []int
			it := br[0]
			for it < br[1] {
				ntmp = append(ntmp, it)
				it++
			}

			b := Block{Offsets: isc}
			doneblockchan <- b
		}
	}
}

func CorrelateIntStrings(hash string) {
	L.Println("correlating strings with ints")

	strs := db.Files[hash].Stringz.S

	sams := 2
	cpus := int(math.Min(float64(len(strs)), float64(runtime.NumCPU()/2-sams)))

	blockchan := make(chan Block, 1000)
	c := make(chan Block, 10)
	done := make(chan bool, cpus+sams)
	D.Println("forking", cpus, "workers")
	for i := 0; i < cpus; i++ {
		go intWorker(c, done, blockchan, hash)
	}

	for i := 0; i < sams; i++ {
		go collector(blockchan, done, hash)
	}

	for _, s := range strs {
		news := s
		b := Block{T: []Text{news}, P: make([]int, db.Files[hash].Size)}
		c <- b
		// if i%1000 == 0 {
		// 	D.Println("sending block", i, "/", len(strs), s.Pos)
		// }
	}
	close(c)
	D.Println("sending blocks is done. collecting workers")

	for i := 0; i < cpus; i++ {
		<-done
	}
	for {
		if len(blockchan) == 0 {
			break
		}
	}
	close(blockchan)

	for i := 0; i < sams; i++ {
		<-done
		D.Println("joining collector", i)
	}
	D.Println("CorrelateIntStrings is done")
}

func intWorker(ch chan Block, done chan bool, doneblockchan chan Block, hash string) {
	defer func() { done <- true }()
	ints := db.Files[hash].Intz.I
	for {
		select {
		case b, more := <-ch:
			if !more {
				return
			}
			// this is safe to assume here, because we only get blocks from the int2string function above, which only puts in arrays containing one lemenet
			s := b.T[0]

			for _, i := range ints {
				//if i.ValS > 0 && strings.HasPrefix(i.Enc, "int") {
				// if signed was parsed as positive, unsigned also was, so this would be a duplicate
				// TODO Maybe we do not need this anymore?
				//continue
				//}

				if i.Pos < s.Pos {
					if i.ValU == s.Len {
						//D.Println("possible length:", i, s)
						newl := i
						//b.LenNew = append(b.LenNew, num)
						b.Length = append(b.Length, newl)
					}
				}
			}

			// add stuff to map
			//if len(b.Offs) != 0 {
			b.Pcomp = make(map[int]int)
			for iter, val := range b.P {
				if val != 0 {
					b.Pcomp[iter] = val
				}
			}
			//}
			b.P = nil
			if len(b.Length) != 0 || len(b.Pcomp) != 0 {
				//D.Println("adding to list of blocks")
				doneblockchan <- b
			}

		} //select
	} //for
} //func

func collector(bc chan Block, done chan bool, hash string) {
	defer func() { done <- true }()
	for {
		select {
		case b, more := <-bc:
			if !more {
				return
			}
			db.Files[hash].Blockz.Add(b)
		}
	}
}
