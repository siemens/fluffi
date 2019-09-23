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
§§package main
§§
§§import (
§§	"encoding/gob"
§§	"encoding/json"
§§	"io/ioutil"
§§	"os"
§§
§§	//"sort"
§§	"sync"
§§)
§§
§§var db *Database
§§
§§type Database struct {
§§	Files map[string]File
§§	m     sync.Mutex
§§}
§§
§§func NewDB() *Database {
§§	var d Database
§§	d.Files = make(map[string]File)
§§	return &d
§§}
§§
§§type File struct {
§§	Name          string
§§	Size          uint64
§§	Content       []byte
§§	IsPureAscii   bool
§§	IsPureUtf8    bool
§§	IsPureUnicode bool
§§	Stringz       *Strings
§§	Magicz        *Magics
§§	Intz          *Ints
§§	Blockz        *Blocks
§§}
§§
§§func NewFile() File {
§§	f := File{Name: "", Size: 0, Content: make([]byte, 0), IsPureAscii: false, IsPureUtf8: false, IsPureUnicode: false, Stringz: new(Strings), Magicz: new(Magics), Intz: new(Ints), Blockz: new(Blocks)}
§§	return f
§§}
§§
§§type Strings struct {
§§	S      []Text
§§	Breaks [][2]int
§§	m      sync.Mutex
§§}
§§
§§func (s *Strings) Add(t Text) {
§§	s.m.Lock()
§§	s.S = append(s.S, t)
§§	s.m.Unlock()
§§}
§§
§§type Text struct {
§§	Enc  string // should be sth like ascii, utf8, utf16
§§	Pos  uint64
§§	Len  uint64
§§	Term bool
§§}
§§
§§func (i Text) Order() int {
§§	return int(i.Pos)
§§}
§§
§§type Magics struct {
§§	M []MagiVal
§§	m sync.Mutex
§§}
§§
§§func (m *Magics) Add(v MagiVal) {
§§	m.m.Lock()
§§	m.M = append(m.M, v)
§§	m.m.Unlock()
§§}
§§
§§type MagiVal struct {
§§	Type string
§§	Pos  uint64
§§	Len  uint64
§§}
§§
§§type Ints struct {
§§	I      []Int
§§	Breaks [][2]int
§§	m      sync.Mutex
§§}
§§
§§func (m *Ints) Add(v Int) {
§§	m.m.Lock()
§§	m.I = append(m.I, v)
§§	m.m.Unlock()
§§}
§§
§§type Int struct {
§§	Enc   string
§§	ByteW uint64
§§	Pos   uint64
§§	ValS  int64
§§	ValU  uint64
§§}
§§
§§func (i Int) Order() int {
§§	return int(i.Pos)
§§}
§§
§§type Blocks struct {
§§	m sync.Mutex
§§	B []Block
§§}
§§
§§// func (b *Blocks) FindText(t Text) (Block, bool) {
§§// 	b.m.Lock()
§§// 	defer b.m.Unlock()
§§//
§§// 	index := sort.Search(len(b.B), func(i int) bool {
§§// 		return b.B[i].T.Pos >= t.Pos
§§// 	})
§§// 	if index < len(b.B) {
§§// 		return b.B[index], true
§§// 	}
§§// 	return Block{}, false
§§// }
§§
§§func (b *Blocks) Add(n Block) {
§§	b.m.Lock()
§§	b.B = append(b.B, n)
§§	b.m.Unlock()
§§}
§§
§§type IntStringGroupCorrelation struct {
§§	IntBreaks [2]int
§§	StrBreaks [2]int
§§}
§§
§§type Block struct {
§§	T       []Text
§§	Offsets IntStringGroupCorrelation
§§	Pcomp   map[int]int
§§	P       []int
§§	Length  []Int
§§}
§§
§§func loadDB() {
§§	db = NewDB()
§§	fi, e := os.Stat("carrot.gob")
§§	if e != nil {
§§		return
§§	}
§§	if !fi.Mode().IsRegular() {
§§		return
§§	}
§§	if fi.Size() == 0 {
§§		return
§§	}
§§
§§	f, e := os.Open("carrot.gob")
§§	if e != nil {
§§		ec <- e
§§		return
§§	}
§§	defer f.Close()
§§
§§	gd := gob.NewDecoder(f)
§§	e = gd.Decode(&db)
§§	if e != nil {
§§		ec <- e
§§		return
§§	}
§§}
§§
§§func saveDB() {
§§	f, e := os.Create("carrot.gob")
§§	if e != nil {
§§		ec <- e
§§		return
§§	}
§§	defer f.Close()
§§
§§	ge := gob.NewEncoder(f)
§§	e = ge.Encode(db)
§§	if e != nil {
§§		ec <- e
§§		return
§§	}
§§
§§	if *fJson {
§§		b, e := json.MarshalIndent(db, "", "  ")
§§		if e != nil {
§§			ec <- e
§§			return
§§		}
§§		e = ioutil.WriteFile("carrot.db", b, 0644)
§§		if e != nil {
§§			ec <- e
§§			return
§§		}
§§	}
§§	db = nil
§§
§§}
