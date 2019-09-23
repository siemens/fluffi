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
§§	"CaRRoT/jenks"
§§	"CaRRoT/ring"
§§	"bytes"
§§
§§	//"fmt"
§§	//"io/ioutil"
§§	"math"
§§	"sort"
§§	"strconv"
§§	"sync"
§§)
§§
§§var digestwait sync.WaitGroup
§§
§§func ScanStrings(b []byte, hash string) {
§§	L.Println("scanning for strings")
§§	// start routines for ascii, bom, unicode
§§	digestwait.Add(1)
§§	ac := asciidigester(hash)
§§	digestwait.Add(1)
§§	uc := unicodedigester(hash)
§§	digestwait.Add(1)
§§	bc := bomdigester(hash)
§§	digestwait.Add(1)
§§	tc := utf8digester(hash)
§§
§§	// feed them bytes // FIXME O(n)
§§	for _, c := range b {
§§		ac <- c
§§		uc <- c
§§		bc <- c
§§		tc <- c
§§	}
§§	close(ac)
§§	close(uc)
§§	close(bc)
§§	close(tc)
§§	digestwait.Wait()
§§
§§	minimizeStrings(hash)
§§
§§	// now, assess what we found
§§	db.m.Lock()
§§	// FIXME O(n)
§§	for _, n := range db.Files[hash].Magicz.M {
§§		if n.Pos == 0 {
§§			f := db.Files[hash]
§§			f.IsPureUnicode = true
§§			db.Files[hash] = f
§§			break
§§		}
§§	}
§§	if 0 != len(db.Files[hash].Stringz.S) {
§§		n := db.Files[hash].Stringz.S[0]
§§		if (n.Pos == 0) && (n.Len == db.Files[hash].Size) {
§§			f := db.Files[hash]
§§			if n.Enc == "ascii" {
§§				f.IsPureAscii = true
§§				db.Files[hash] = f
§§			} else if n.Enc == "utf8" {
§§				f.IsPureUtf8 = true
§§				db.Files[hash] = f
§§			} else {
§§				f.IsPureUnicode = true
§§				db.Files[hash] = f
§§			}
§§		}
§§	}
§§	db.m.Unlock()
§§}
§§
§§func GroupStrings(hash string, groupruns int, scaler float64) []struct {
§§	score  float64
§§	breaks [][2]int
§§	grcnt  int
§§} {
§§	L.Println("grouping strings")
§§	D.Println("starting jenks grouping for strings")
§§
§§	var m sync.Mutex
§§	var waitjenks sync.WaitGroup
§§	var groupings []struct {
§§		score  float64
§§		breaks [][2]int
§§		grcnt  int
§§	}
§§
§§	// scale group count down to order of magnitude of len(intarray) - 10^3
§§	lofa := len(db.Files[hash].Stringz.S)
§§	Olofa := len(strconv.Itoa(lofa))
§§
§§	if Olofa <= 1 {
§§		m.Lock()
§§		groupings = append(groupings, struct {
§§			score  float64
§§			breaks [][2]int
§§			grcnt  int
§§		}{0.1, [][2]int{{0, lofa - 1}}, 1})
§§		m.Unlock()
§§		D.Println("jenks group scaling will not run, because order of length is too small")
§§		return groupings
§§	}
§§	D.Println("jenks group scaling:", lofa, Olofa, int(math.Pow(10, float64(Olofa-1))))
§§
§§	// 0.90^x for x in [0..15] will get us down to 20% of the original group count
§§	for threads := 0; threads < groupruns; threads++ {
§§		waitjenks.Add(1)
§§		go func(index int) {
§§			defer waitjenks.Done()
§§
§§			//groups := int(math.Pow(10, float64(Olofa-index-1)))
§§			groups := int(math.Pow(10, float64(Olofa-1)) * math.Pow(scaler, float64(index)))
§§			if groups == 0 {
§§				return
§§			}
§§
§§			D.Println("building jenks worker with", groups)
§§
§§			// https://github.com/golang/go/wiki/InterfaceSlice
§§			var groupableSlice []jenks.Groupable = make([]jenks.Groupable, len(db.Files[hash].Stringz.S))
§§			for i, d := range db.Files[hash].Stringz.S {
§§				groupableSlice[i] = d
§§			}
§§
§§			D.Println("executing jenks worker with", groups)
§§			rv := jenks.JenksGroup(groupableSlice, groups)
§§
§§			sc := jenks.ScoreJenksGroup(groupableSlice, rv)
§§			//D.Println(groups, "groups scored as", sc, "containing", len(rv))
§§
§§			m.Lock()
§§			groupings = append(groupings, struct {
§§				score  float64
§§				breaks [][2]int
§§				grcnt  int
§§			}{sc, rv, groups})
§§			m.Unlock()
§§
§§			// posis := make([]uint64, len(rv))
§§			// for i, el := range rv {
§§			// 	posis[i] = uint64(el[1])
§§			// }
§§			// str := fmt.Sprint(posis)
§§			// ioutil.WriteFile("breaks"+strconv.Itoa(groups)+".csv", []byte(str), 0644)
§§		}(threads)
§§	}
§§
§§	waitjenks.Wait()
§§
§§	sort.Slice(groupings, func(i, j int) bool { return groupings[i].score < groupings[j].score })
§§	D.Println("finished jenks grouping")
§§
§§	return groupings
§§}
§§
§§func bomdigester(hash string) chan byte {
§§	cc := make(chan byte, 10)
§§	var pos uint64            // current position in the stream
§§	buf := []byte{0, 0, 0, 0} // byte buffer for start of file
§§	var headDone bool         // marker that magic header was searched. continue further down the file
§§	rb := ring.New(4)         // ringbuffer for finding headers inside the file
§§
§§	go func() {
§§		defer digestwait.Done()
§§		for {
§§			select {
§§			case b, more := <-cc:
§§				if more {
§§					rb.Push(b)
§§					if !(headDone) {
§§						// BUG: this stuff might not terminate for testfiles that are really small
§§						if pos < uint64(len(buf)) {
§§							buf[pos] = b
§§						} else {
§§							if compareHelper(buf) {
§§								nm := MagiVal{Len: 4, Pos: 0, Type: "BOM"}
§§								db.Files[hash].Magicz.Add(nm)
§§							}
§§							headDone = true
§§						}
§§					} else {
§§						if compareHelper(rb.Window()) {
§§							nm := MagiVal{Len: 4, Pos: pos - 4, Type: "BOM"}
§§							db.Files[hash].Magicz.Add(nm)
§§						}
§§					}
§§					pos++
§§				} else {
§§					for i := 0; i < 4; i++ {
§§						rb.Push(0x00)
§§						if compareHelper(rb.Window()) {
§§							nm := MagiVal{Len: 4, Pos: pos - 4, Type: "BOM"}
§§							db.Files[hash].Magicz.Add(nm)
§§						}
§§						pos++
§§					}
§§					return
§§				}
§§			case <-quit:
§§				return
§§			}
§§		}
§§
§§	}()
§§
§§	return cc
§§}
§§
§§func unicodedigester(hash string) chan byte {
§§	cc := make(chan byte, 10)
§§	var pos uint64
§§	var last byte
§§	var strtrace uint64
§§	strlim := uint64(*fStringMinLen) // minimal length
§§	var strslack uint64
§§	var buf bytes.Buffer
§§
§§	go func() {
§§		defer digestwait.Done()
§§		for {
§§			select {
§§			case b, more := <-cc:
§§				if more {
§§					if last == 0x00 {
§§						if isASCIIcontrol(b) || isASCIIcontrol(b) {
§§							strtrace++
§§							buf.WriteByte(b)
§§						} else {
§§							if strslack > 0 {
§§								strslack -= 1
§§								buf.WriteByte(b)
§§							} else {
§§								if strtrace >= strlim {
§§									ns := Text{Enc: "unicode", Pos: pos - uint64(buf.Len()), Len: uint64(buf.Len()), Term: isTerminator(b)}
§§									db.Files[hash].Stringz.Add(ns)
§§								}
§§								buf.Reset()
§§								strtrace = 0
§§							}
§§						}
§§					} else if isASCIIcontrol(last) || isASCIIprintable(last) {
§§						if b == 0x00 {
§§							strtrace++
§§							buf.WriteByte(b)
§§						} else {
§§							if strslack > 0 {
§§								strslack -= 1
§§								buf.WriteByte(b)
§§							} else {
§§								if strtrace >= strlim {
§§									ns := Text{Enc: "unicode", Pos: pos - uint64(buf.Len()), Len: uint64(buf.Len()), Term: isTerminator(b)}
§§									db.Files[hash].Stringz.Add(ns)
§§								}
§§								strtrace = 0
§§								buf.Reset()
§§							}
§§						}
§§					} else {
§§						if strslack > 0 {
§§							strslack -= 1
§§							buf.WriteByte(b)
§§						} else {
§§							if strtrace >= strlim {
§§								ns := Text{Enc: "unicode", Pos: pos - uint64(buf.Len()), Len: uint64(buf.Len()), Term: isTerminator(b)}
§§								db.Files[hash].Stringz.Add(ns)
§§							}
§§							strtrace = 0
§§							buf.Reset()
§§						}
§§					}
§§					last = b
§§					pos++
§§				} else {
§§					if strtrace >= strlim {
§§						ns := Text{Enc: "unicode", Pos: pos - uint64(buf.Len()), Len: uint64(buf.Len()), Term: isTerminator(b)}
§§						db.Files[hash].Stringz.Add(ns)
§§					}
§§					return
§§				}
§§			case <-quit:
§§				return
§§			}
§§		}
§§	}()
§§
§§	return cc
§§}
§§
§§func asciidigester(hash string) chan byte {
§§	cc := make(chan byte, 10)
§§
§§	var pos uint64                   // current position in the stream
§§	var possibleAscii uint64         // number of bytes seen, that match ascii criteria
§§	var strtrace uint64              // bytes that match ascii criteria since last byte that did not match ascii criteria
§§	strlim := uint64(*fStringMinLen) // minimal length
§§	var buf bytes.Buffer             // buffer to save bytes of currently read string
§§
§§	go func() {
§§		defer digestwait.Done()
§§		for {
§§			select {
§§			case b, more := <-cc:
§§				if more {
§§					if isASCIIprintable(b) || isASCIIcontrol(b) {
§§						strtrace++
§§						buf.WriteByte(b)
§§					} else {
§§						if strtrace >= strlim {
§§							ns := Text{Enc: "ascii", Pos: pos - uint64(buf.Len()), Len: uint64(buf.Len()), Term: isTerminator(b)}
§§							db.Files[hash].Stringz.Add(ns)
§§						}
§§						buf.Reset()
§§						strtrace = 0
§§					}
§§
§§					if strtrace == strlim {
§§						possibleAscii += strtrace
§§					} else if strtrace > strlim {
§§						possibleAscii++
§§					}
§§					pos++
§§				} else {
§§					if strtrace >= strlim {
§§						ns := Text{Enc: "ascii", Pos: pos - uint64(buf.Len()), Len: uint64(buf.Len()), Term: isTerminator(b)}
§§						db.Files[hash].Stringz.Add(ns)
§§					}
§§					return
§§				}
§§
§§			case <-quit:
§§				return
§§			}
§§		}
§§	}()
§§	return cc
§§}
§§
§§func utf8digester(hash string) chan byte {
§§	cc := make(chan byte, 10)
§§
§§	var pos uint64                   // current position in the stream
§§	var possibleUtf uint64           // number of bytes seen, that match ascii criteria
§§	var strtrace uint64              // bytes that match utf criteria since last byte that did not match
§§	strlim := uint64(*fStringMinLen) // minimal length
§§	var buf bytes.Buffer             // buffer to save bytes of currently read string
§§	utfSequence := false
§§	lastWasStart := false
§§
§§	go func() {
§§		defer digestwait.Done()
§§		for {
§§			select {
§§			case b, more := <-cc:
§§				if more {
§§					if isASCIIprintable(b) || isASCIIcontrol(b) {
§§						strtrace++
§§						buf.WriteByte(b)
§§						utfSequence = false
§§						lastWasStart = false
§§					} else if isUtf8Start(b) && !utfSequence {
§§						strtrace++
§§						buf.WriteByte(b)
§§						utfSequence = true
§§						lastWasStart = true
§§					} else if isUtfFollow(b) && utfSequence {
§§						strtrace++
§§						buf.WriteByte(b)
§§						lastWasStart = false
§§					} else if isUtf8Start(b) && utfSequence && !lastWasStart {
§§						strtrace++
§§						buf.WriteByte(b)
§§						lastWasStart = true
§§					} else {
§§						// TODO BUG
§§						// this has a little bug: if there is a unicode start symbol not followed by a correct follow up byte,
§§						// this terminates correctly, but the start symbol is already counted into the string. :/
§§						if strtrace >= strlim {
§§							ns := Text{Enc: "utf8", Pos: pos - uint64(buf.Len()), Len: uint64(buf.Len()), Term: isTerminator(b)}
§§							db.Files[hash].Stringz.Add(ns)
§§						}
§§						buf.Reset()
§§						strtrace = 0
§§						utfSequence = false
§§						lastWasStart = false
§§					}
§§
§§					if strtrace == strlim {
§§						possibleUtf += strtrace
§§					} else if strtrace > strlim {
§§						possibleUtf++
§§					}
§§					pos++
§§				} else {
§§					if strtrace >= strlim {
§§						ns := Text{Enc: "utf8", Pos: pos - uint64(buf.Len()), Len: uint64(buf.Len()), Term: isTerminator(b)}
§§						db.Files[hash].Stringz.Add(ns)
§§					}
§§					return
§§				}
§§
§§			case <-quit:
§§				return
§§			}
§§		}
§§	}()
§§	return cc
§§}
§§
§§func isASCIIprintable(b byte) bool {
§§	if b >= 0x20 && b <= 0x7F {
§§		return true
§§	}
§§	return false
§§}
§§
§§func isASCIIcontrol(b byte) bool {
§§	if b == 0x0A || b == 0x0D || b == 0x09 {
§§		return true
§§	}
§§	return false
§§}
§§
§§func isTerminator(b byte) bool {
§§	return b == 0x00
§§}
§§
§§func isUtf8Start(b byte) bool {
§§	if b&0xC0 == 0xC0 {
§§		return true
§§	}
§§	return false
§§}
§§
§§func isUtfFollow(b byte) bool {
§§	// zero all bits except first two, then it must read 0x80
§§	if b&0xC0 == 0x80 {
§§		return true
§§	}
§§	return false
§§}
§§
§§func compareHelper(buf []byte) (match bool) {
§§	// https://de.wikipedia.org/wiki/Byte_Order_Mark
§§	if ContainsMagic(buf, 0xef, 0xbb, 0xbf) ||
§§		ContainsMagic(buf, 0xfe, 0xff) ||
§§		ContainsMagic(buf, 0xff, 0xfe) ||
§§		ContainsMagic(buf, 0xf7, 0x64, 0x4c) ||
§§		ContainsMagic(buf, 0x0e, 0xfe, 0xff) ||
§§		ContainsMagic(buf, 0xfb, 0xee, 0x28) ||
§§		ContainsMagic(buf, 0xdd, 0x73, 0x66, 0x73) ||
§§		ContainsMagic(buf, 0x00, 0x00, 0xfe, 0xff) ||
§§		ContainsMagic(buf, 0xff, 0xfe, 0x00, 0x00) ||
§§		ContainsMagic(buf, 0x2b, 0x2f, 0x76, 0x38) ||
§§		ContainsMagic(buf, 0x2b, 0x2f, 0x76, 0x39) ||
§§		ContainsMagic(buf, 0x2b, 0x2f, 0x76, 0x2b) ||
§§		ContainsMagic(buf, 0x2b, 0x2f, 0x76, 0x2f) ||
§§		ContainsMagic(buf, 0x84, 0x31, 0x95, 0x33) {
§§		return true
§§	}
§§	return false
§§}
§§
§§func minimizeStrings(hash string) {
§§	L.Println("minimizing string dataset")
§§	db.m.Lock()
§§	defer db.m.Unlock()
§§	db.Files[hash].Stringz.m.Lock()
§§	defer db.Files[hash].Stringz.m.Unlock()
§§
§§	ar := db.Files[hash].Stringz.S
§§
§§	aa := make([]*Text, db.Files[hash].Size)
§§	au := make([]*Text, db.Files[hash].Size)
§§
§§	for i, _ := range ar {
§§		el := ar[i]
§§		if el.Enc == "ascii" {
§§			aa[el.Pos] = &el
§§		}
§§		if el.Enc == "utf8" {
§§			au[el.Pos] = &el
§§		}
§§	}
§§
§§	ar = nil
§§
§§	var inside bool
§§	var end uint64
§§	for i := uint64(0); i < db.Files[hash].Size; i++ {
§§		// for all ascii entries, look if there is a unicode match, delete unicode match
§§		if aa[i] != nil && au[i] != nil {
§§			if au[i].Pos == aa[i].Pos && au[i].Len == aa[i].Len {
§§				// D.Println("utf duplicate:", *aa[i], "=", *au[i])
§§				au[i] = nil
§§			}
§§		}
§§
§§		// for all unicode matches, look if there is one or more ascii matches inside, delete those.
§§		if au[i] != nil {
§§			inside = true
§§			end = i + au[i].Len
§§		}
§§		if inside {
§§			// if aa[i] != nil {
§§			// 	D.Println("inside another:", aa[i], "-", i+aa[i].Len-end)
§§			// }
§§			aa[i] = nil
§§		}
§§		if i == end {
§§			inside = false
§§		}
§§
§§		// write sorted stuff back to colleting slice
§§		if aa[i] != nil {
§§			ar = append(ar, *aa[i])
§§		}
§§		if au[i] != nil {
§§			ar = append(ar, *au[i])
§§		}
§§	}
§§
§§	db.Files[hash].Stringz.S = ar
§§}
