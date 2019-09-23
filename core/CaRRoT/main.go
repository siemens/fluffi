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
§§	"crypto/sha1"
§§	"encoding/hex"
§§	"flag"
§§	"fmt"
§§	"io/ioutil"
§§	"log"
§§	"os"
§§	"strconv"
§§)
§§
§§var L *log.Logger
§§var D *log.Logger
§§var E *log.Logger
§§
§§var ec chan error
§§var quit chan struct{}
§§
§§var fStringMinLen *int
§§var fJson *bool
§§var fIntDedup *bool
§§
§§func main() {
§§	ec = make(chan error)
§§	quit = make(chan struct{})
§§
§§	L = log.New(os.Stderr, "[-CRT-] ", log.Ldate|log.Ltime)
§§	D = log.New(ioutil.Discard, "", 0)
§§	E = log.New(os.Stderr, "[ERROR] ", log.Ldate|log.Ltime)
§§	L.Println("starting CaRRoT")
§§
§§	fDebug := flag.Bool("d", false, "enable debug log")
§§	fNum := flag.Int("n", 1, "how many mutations to produce")
§§	fStringMinLen = flag.Int("sl", 5, "minimal length to detect a string")
§§	fPrefix := flag.String("o", "carrot_", "output folder and file name prefix")
§§	fReanalyze := flag.Bool("r", false, "force reanalyze if file is in db")
§§	fGroups := flag.Int("g", 15, "how many reduceed group runs to perform")
§§	fScaler := flag.Float64("gs", 0.9, "scaling factor for reduced group runs")
§§	fLengthCorrelation := flag.Bool("no-lc", false, "turn of length correlation")
§§	fOffsetCorrelation := flag.Bool("no-oc", false, "turn of offset correlation")
§§	fJson = flag.Bool("j", false, "print results to json file")
§§	fIntDedup = flag.Bool("idd", false, "attempt to deduplicate found ints")
§§
§§	flag.Usage = func() {
§§		fmt.Fprintln(os.Stderr, carrot())
§§		fmt.Fprintln(os.Stderr, "CaЯRo⊥ - a more intelligent fuzzcase generator")
§§		fmt.Fprintln(os.Stderr, "USAGE: CaЯRo⊥ [OPTS] FILENAME")
§§		fmt.Fprintln(os.Stderr, "OPTS:")
§§		flag.PrintDefaults()
§§		fmt.Fprintln(os.Stderr, "FILENAME:")
§§		fmt.Fprintln(os.Stderr, "	name of the goodput file to analyze")
§§	}
§§
§§	flag.Parse()
§§
§§	if *fDebug {
§§		L = log.New(os.Stderr, "[-LOG-] ", log.Ldate|log.Ltime|log.Lshortfile)
§§		D = log.New(os.Stderr, "[DEBUG] ", log.Ldate|log.Ltime|log.Lshortfile)
§§	}
§§
§§	// start errorhandler
§§	go func() {
§§		D.Println("starting error handler routine")
§§		select {
§§		case <-quit:
§§		case e := <-ec:
§§			if e != nil {
§§				E.Println("An ERROR occurred:", e.Error())
§§			}
§§			close(quit)
§§			if e != nil {
§§				os.Exit(1)
§§			}
§§		}
§§	}()
§§
§§	// load analysis database
§§	D.Println("loading analysis db")
§§	loadDB()
§§
§§	D.Println("checking file permissions")
§§	// make sure a file was passed
§§	args := flag.Args()
§§	if len(args) == 0 {
§§		flag.Usage()
§§		os.Exit(1)
§§	}
§§	fi, e := os.Stat(args[0])
§§	if e != nil {
§§		flag.Usage()
§§		os.Exit(1)
§§	}
§§	if !fi.Mode().IsRegular() {
§§		flag.Usage()
§§		os.Exit(1)
§§	}
§§
§§	D.Println("reading file")
§§	// sha the file. if we have seen it before,
§§	// no need to reanalyze it. if it is new, reanalyze.
§§	buf, e := ioutil.ReadFile(args[0])
§§	if e != nil {
§§		ec <- e
§§		select {
§§		case <-quit:
§§		}
§§		return
§§	}
§§	D.Println("hashing file")
§§	hb := sha1.Sum(buf)
§§	hash := hex.EncodeToString(hb[:])
§§
§§	// analyze the file
§§	if _, known := db.Files[hash]; !known || *fReanalyze {
§§		if known {
§§			L.Println("analyzing stuff, even though we already have!")
§§		} else {
§§			L.Println("analyzing!")
§§		}
§§		f := NewFile()
§§		f.Name = args[0]
§§		f.Size = uint64(len(buf))
§§		f.Content = buf
§§		db.Files[hash] = f
§§		ScanStrings(buf, hash)
§§		ScanInts(buf, hash)
§§
§§		if !*fLengthCorrelation {
§§			CorrelateIntStrings(hash)
§§		}
§§
§§		if !*fOffsetCorrelation {
§§			done := make(chan struct{})
§§			go func() {
§§				defer func() { done <- struct{}{} }()
§§				groups := GroupInts(hash, *fGroups, *fScaler)
§§				for _, i := range groups {
§§					D.Println("Int:", len(i.breaks), "/", i.grcnt, "scored as", i.score)
§§				}
§§				db.Files[hash].Intz.Breaks = groups[0].breaks
§§			}()
§§
§§			go func() {
§§				defer func() { done <- struct{}{} }()
§§				strgroups := GroupStrings(hash, *fGroups, *fScaler)
§§				for _, i := range strgroups {
§§					D.Println("Str:", len(i.breaks), "/", i.grcnt, "scored as", i.score)
§§				}
§§				db.Files[hash].Stringz.Breaks = strgroups[0].breaks
§§			}()
§§			<-done
§§			<-done
§§
§§			// now we check if we can feasibly run correlation (which is O(n²) in time and MUCH MUCH WORSE in space)
§§			complraw := uint64(len(db.Files[hash].Intz.I) * len(db.Files[hash].Stringz.S))
§§			L.Println("strings:", len(db.Files[hash].Stringz.S), "ints:", len(db.Files[hash].Intz.I), "complexity:", complraw)
§§
§§			complred := uint64(len(db.Files[hash].Intz.Breaks) * len(db.Files[hash].Stringz.Breaks))
§§			L.Println("string groups:", len(db.Files[hash].Stringz.Breaks), "intgroups:", len(db.Files[hash].Intz.Breaks), "complexity:", complred)
§§
§§			L.Println("reduction in complexity:", float64(complraw)/float64(complred))
§§			CorrelateGroupStrings(hash)
§§		}
§§
§§		// now we can search all strings (or the entire file) for signs of superstructures: xml, json, ...
§§		// SearchSuperstructures(hash)
§§		// AnalyzeEntropy(hash)
§§
§§	} else {
§§		L.Println("file is known!")
§§	}
§§
§§	for _, f := range db.Files[hash].Blockz.B {
§§		D.Println(f)
§§	}
§§
§§	// choose a (TODO: random) mutation engine
§§	mut := randomMutationEngine(hash)
§§	// generate the required amount of mutations
§§	for i := 1; i <= *fNum; i++ {
§§		D.Println("writing file", i)
§§
§§		mb := make([]byte, 0)
§§		fb := make([]byte, len(buf))
§§		copy(fb, buf)
§§		mb = mut.Do(hash, fb)
§§		e = ioutil.WriteFile(((*fPrefix) + strconv.Itoa(i)), mb, 0644)
§§		if e != nil {
§§			ec <- e
§§			select {
§§			case <-quit:
§§			}
§§			return
§§		}
§§	}
§§
§§	D.Println("done generating")
§§	D.Println("strings:", len(db.Files[hash].Stringz.S))
§§	D.Println("ints:", len(db.Files[hash].Intz.I))
§§	D.Println("blocks:", len(db.Files[hash].Blockz.B))
§§
§§	D.Println("start saving")
§§	saveDB()
§§	D.Println("done saving")
§§	ec <- nil
§§	select {
§§	case <-quit:
§§	}
§§	L.Println("exiting CaRRoT")
§§}
