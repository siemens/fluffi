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
	"CaRRoT/jenks"
	"encoding/binary"
	"strings"

	//"fmt"
	//"io/ioutil"
	"math"
	//"runtime"
	"sort"
	"strconv"
	"sync"
)

var waitFor sync.WaitGroup

func ScanInts(b []byte, hash string) {
	L.Println("scanning for ints")
	// create string/pos buffer
	instring := make([]bool, db.Files[hash].Size)
	db.Files[hash].Stringz.m.Lock()
	for _, el := range db.Files[hash].Stringz.S {
		for i := el.Pos; i < el.Pos+el.Len; i++ {
			instring[i] = true
		}
	}
	db.Files[hash].Stringz.m.Unlock()

	// try to find and interpret values
	for i := uint64(0); i < db.Files[hash].Size; i++ {
		if i < db.Files[hash].Size-2 {
			if !instring[i] && !instring[i+1] {
				waitFor.Add(1)
				go checkInt(b[i:], i, 2, hash)
			}
		}
		if i < db.Files[hash].Size-4 {
			if !instring[i] && !instring[i+1] && !instring[i+2] && !instring[i+3] {
				waitFor.Add(1)
				go checkInt(b[i:], i, 4, hash)
			}
		}
		if i < db.Files[hash].Size-8 {
			if !instring[i] && !instring[i+1] && !instring[i+2] && !instring[i+3] &&
				!instring[i+4] && !instring[i+5] && !instring[i+6] && !instring[i+7] {
				waitFor.Add(1)
				go checkInt(b[i:], i, 8, hash)
			}
		}
	}
	waitFor.Wait()

	// posis := make([]uint64, len(db.Files[hash].Intz.I))
	// for i, el := range db.Files[hash].Intz.I {
	// 	posis[i] = el.Pos
	// }
	// str := fmt.Sprint(posis)
	// ioutil.WriteFile("positions.csv", []byte(str), 0644)

	// TODO deduplicate

	sls := NewSliceSorter(db.Files[hash].Intz.I)

	if *fIntDedup {
		D.Println("starting int map dedup")
		belm := make([][]uint64, db.Files[hash].Size)
		newints := make([]Int, 0)
		lofints := len(db.Files[hash].Intz.I)
		for _, j := range sls.W64().All() {
			for i := j.Pos; i < j.Pos+j.ByteW; i++ {
				belm[i] = append(belm[i], j.ValU)
			}
			nj := j
			newints = append(newints, nj)
		}
		for _, j := range sls.W32().All() {
			blocked := false
			for _, k := range belm[j.Pos] {
				if k == j.ValU {
					blocked = true
				}
			}
			if !blocked {
				for i := j.Pos; i < j.Pos+j.ByteW; i++ {
					belm[i] = append(belm[i], j.ValU)
				}
				nj := j
				newints = append(newints, nj)
			}
		}
		for _, j := range sls.W16().All() {
			blocked := false
			for _, k := range belm[j.Pos] {
				if k == j.ValU {
					blocked = true
				}
			}
			if !blocked {
				for i := j.Pos; i < j.Pos+j.ByteW; i++ {
					belm[i] = append(belm[i], j.ValU)
				}
				nj := j
				newints = append(newints, nj)
			}
		}
		db.Files[hash].Intz.I = newints
		D.Println("finished int map dedup. reduced ints - before", (lofints), "after", (len(newints)))
	}

	D.Println("starting int sort")
	sort.Slice(db.Files[hash].Intz.I, func(i, j int) bool { return db.Files[hash].Intz.I[i].Pos < db.Files[hash].Intz.I[j].Pos })
	D.Println("finished int sort")
}

type SliceSorter struct {
	ar []Int
}

func NewSliceSorter(i []Int) SliceSorter {
	return SliceSorter{ar: i}
}

func (s SliceSorter) All() (r []Int) {
	return s.ar
}

func (s SliceSorter) Big() (ns SliceSorter) {
	for _, i := range s.ar {
		if strings.HasSuffix(i.Enc, "be") {
			newi := i
			ns.ar = append(ns.ar, newi)
		}
	}
	return
}

func (s SliceSorter) Little() (ns SliceSorter) {
	for _, i := range s.ar {
		if strings.HasSuffix(i.Enc, "le") {
			newi := i
			ns.ar = append(ns.ar, newi)
		}
	}
	return
}

func (s SliceSorter) W16() (ns SliceSorter) {
	for _, i := range s.ar {
		if i.ByteW == 2 {
			newi := i
			ns.ar = append(ns.ar, newi)
		}
	}
	return
}
func (s SliceSorter) W32() (ns SliceSorter) {
	for _, i := range s.ar {
		if i.ByteW == 4 {
			newi := i
			ns.ar = append(ns.ar, newi)
		}
	}
	return
}
func (s SliceSorter) W64() (ns SliceSorter) {
	for _, i := range s.ar {
		if i.ByteW == 8 {
			newi := i
			ns.ar = append(ns.ar, newi)
		}
	}
	return
}

func checkInt(bf []byte, pos uint64, w int, hash string) {
	defer waitFor.Done()
	plausibleSigned := func(i int64) bool {
		if i == 0 {
			return false
		}
		if i < -10 {
			return false
		}
		if i < 0 {
			return true
		}
		if i == 256 {
			return false
		}
		if uint64(i) > db.Files[hash].Size {
			return false
		}
		return true
	}
	plausibleUnsigned := func(i uint64) bool {
		if i == 0 {
			return false
		}
		if i == 256 {
			return false
		}
		if i > db.Files[hash].Size {
			return false
		}
		return true
	}

	var sdata int64
	var udata uint64

	if w == 2 {
		udata = uint64(binary.BigEndian.Uint16(bf))
		sdata = int64(int16(binary.BigEndian.Uint16(bf)))

		if plausibleUnsigned(udata) {
			db.Files[hash].Intz.Add(Int{Enc: "uint16be", ByteW: 2, Pos: pos, ValS: sdata, ValU: udata})
		}
		if sdata < 0 || udata != uint64(sdata) {
			if plausibleSigned(sdata) {
				db.Files[hash].Intz.Add(Int{Enc: "int16be", ByteW: 2, Pos: pos, ValS: sdata, ValU: udata})
			}
		}

		udata = uint64(binary.LittleEndian.Uint16(bf))
		sdata = int64(int16(binary.LittleEndian.Uint16(bf)))
		if plausibleUnsigned(udata) {
			db.Files[hash].Intz.Add(Int{Enc: "uint16le", ByteW: 2, Pos: pos, ValS: sdata, ValU: udata})
		}
		if sdata < 0 || udata != uint64(sdata) {
			if plausibleSigned(sdata) {
				db.Files[hash].Intz.Add(Int{Enc: "int16le", ByteW: 2, Pos: pos, ValS: sdata, ValU: udata})
			}
		}
	} else if w == 4 {
		udata = uint64(binary.BigEndian.Uint32(bf))
		sdata = int64(int32(binary.BigEndian.Uint32(bf)))
		if plausibleUnsigned(udata) {
			db.Files[hash].Intz.Add(Int{Enc: "uint32be", ByteW: 4, Pos: pos, ValS: sdata, ValU: udata})
		}
		if sdata < 0 || udata != uint64(sdata) {
			if plausibleSigned(sdata) {
				db.Files[hash].Intz.Add(Int{Enc: "int32be", ByteW: 4, Pos: pos, ValS: sdata, ValU: udata})
			}
		}

		udata = uint64(binary.LittleEndian.Uint32(bf))
		sdata = int64(int32(binary.LittleEndian.Uint32(bf)))
		if plausibleUnsigned(udata) {
			db.Files[hash].Intz.Add(Int{Enc: "uint32le", ByteW: 4, Pos: pos, ValS: sdata, ValU: udata})
		}
		if sdata < 0 || udata != uint64(sdata) {
			if plausibleSigned(sdata) {
				db.Files[hash].Intz.Add(Int{Enc: "int32le", ByteW: 4, Pos: pos, ValS: sdata, ValU: udata})
			}
		}
	} else if w == 8 {
		udata = binary.BigEndian.Uint64(bf)
		sdata = int64(binary.BigEndian.Uint64(bf))
		if plausibleUnsigned(udata) {
			db.Files[hash].Intz.Add(Int{Enc: "uint64be", ByteW: 8, Pos: pos, ValS: sdata, ValU: udata})
		}
		if sdata < 0 || udata != uint64(sdata) {
			if plausibleSigned(sdata) {
				db.Files[hash].Intz.Add(Int{Enc: "int64be", ByteW: 8, Pos: pos, ValS: sdata, ValU: udata})
			}
		}

		udata = binary.LittleEndian.Uint64(bf)
		sdata = int64(binary.LittleEndian.Uint64(bf))
		if plausibleUnsigned(udata) {
			db.Files[hash].Intz.Add(Int{Enc: "uint64le", ByteW: 8, Pos: pos, ValS: sdata, ValU: udata})
		}
		if sdata < 0 || udata != uint64(sdata) {
			if plausibleSigned(sdata) {
				db.Files[hash].Intz.Add(Int{Enc: "int64le", ByteW: 8, Pos: pos, ValS: sdata, ValU: udata})
			}
		}
	}
}

func GroupInts(hash string, groupruns int, scaler float64) []struct {
	score  float64
	breaks [][2]int
	grcnt  int
} {
	L.Println("grouping ints")
	D.Println("starting jenks grouping")

	var m sync.Mutex
	var waitjenks sync.WaitGroup
	var groupings []struct {
		score  float64
		breaks [][2]int
		grcnt  int
	}

	// var iters []int = []int{100000, 50000, 10000, 5000, 1000, 500, 100}

	// scale group count down to order of magnitude of len(intarray) - 10^3
	lofa := len(db.Files[hash].Intz.I)
	Olofa := len(strconv.Itoa(lofa))

	if Olofa <= 1 {
		m.Lock()
		groupings = append(groupings, struct {
			score  float64
			breaks [][2]int
			grcnt  int
		}{0.1, [][2]int{{0, lofa - 1}}, 1})
		m.Unlock()
		D.Println("jenks group scaling will not run, because order of length is too small")
		return groupings
	}
	D.Println("jenks group scaling:", lofa, Olofa, int(math.Pow(10, float64(Olofa-1))))

	// 0.90^x for x in [0..15] will get us down to 20% of the original group count
	for threads := 0; threads < groupruns; threads++ {
		waitjenks.Add(1)
		go func(index int) {
			defer waitjenks.Done()

			//groups := int(math.Pow(10, float64(Olofa-index-1)))
			groups := int(math.Pow(10, float64(Olofa-1)) * math.Pow(scaler, float64(index)))

			D.Println("building jenks worker with", groups)

			// https://github.com/golang/go/wiki/InterfaceSlice
			var groupableSlice []jenks.Groupable = make([]jenks.Groupable, len(db.Files[hash].Intz.I))
			for i, d := range db.Files[hash].Intz.I {
				groupableSlice[i] = d
			}

			D.Println("executing jenks worker with", groups)
			rv := jenks.JenksGroup(groupableSlice, groups)

			sc := jenks.ScoreJenksGroup(groupableSlice, rv)
			//D.Println(groups, "groups scored as", sc, "containing", len(rv))

			m.Lock()
			groupings = append(groupings, struct {
				score  float64
				breaks [][2]int
				grcnt  int
			}{sc, rv, groups})
			m.Unlock()

			// posis := make([]uint64, len(rv))
			// for i, el := range rv {
			// 	posis[i] = uint64(el[1])
			// }
			// str := fmt.Sprint(posis)
			// ioutil.WriteFile("breaks"+strconv.Itoa(groups)+".csv", []byte(str), 0644)
		}(threads)
	}

	waitjenks.Wait()

	sort.Slice(groupings, func(i, j int) bool { return groupings[i].score < groupings[j].score })
	D.Println("finished jenks grouping")

	return groupings
}
