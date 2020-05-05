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
	"regexp"
	"strings"
)

func SearchSuperstructures(hash string) {
	L.Println("searching superstructures in strings")

	if db.Files[hash].IsPureAscii || db.Files[hash].IsPureUtf8 || db.Files[hash].IsPureUnicode {
		// first convert content into a string
		ctnt := string(db.Files[hash].Content)

		// check if xml or json
		var pXML, pJSON float64
		{
			n := 2
			dc := make(chan struct{}, n)
			ncnt := normalizeString(ctnt)
			go func() {
				pXML = assessXML(ncnt)
				dc <- struct{}{}
			}()
			go func() {
				pJSON = assessJSON(ncnt)
				dc <- struct{}{}
			}()
			for n != 0 {
				<-dc
				n--
			}
		}
		L.Println("XML:", pXML, "JSON:", pJSON)

		// search for separators like &, =, \n, \r\n, ' ' and look for preiodicity

	} else {
		// go through all strings and do the above :/
		L.Println("not implemented")
	}
}

func normalizeString(in string) (out string) {
	var sb strings.Builder
	m := make(map[rune]struct{})
	m['\n'] = struct{}{}
	m[' '] = struct{}{}
	m['\r'] = struct{}{}
	m['\t'] = struct{}{}

	greenlight := true

	for _, rn := range in {
		if rn == '"' {
			greenlight = !greenlight
		}
		if greenlight {
			if _, r := m[rn]; !r {
				sb.WriteRune(rn)
			}
		} else {
			sb.WriteRune(rn)
		}
	}
	out = sb.String()
	//D.Println(out)
	if !greenlight {
		D.Println("warning: there are an uneven number of gänsefüßchen runes in the string!")
	}
	return
}

func assessXML(input string) (prb float64) {
	// check if we find <?xml
	inx := strings.Index(input, "<?xml")
	if inx >= 0 {
		D.Println("xml: found xml marker")
		prb = 0.8
	}
	// count all < and >
	opening := strings.Count(input, "<")
	closing := strings.Count(input, ">")

	if opening+closing > 30 {
		prb = (prb + 0.2) / 1.2
	} else if opening > 100 || closing > 100 {
		prb = (prb + 0.5) / 1.5
	}

	D.Println("xml: pointy braces subsum:", opening-closing)

	// if the difference is less than lets say 5%
	if math.Abs(float64(opening-closing))/math.Max(float64(opening), float64(closing)) < 0.05 {
		prb = (prb + 0.1) / 1.1
	}

	// find </ and />
	tagterm := strings.Count(input, "</")
	nulltags := strings.Count(input, "/>")
	D.Println("xml: tag terminators:", tagterm+nulltags)

	D.Println("xml: circa tags:", (opening-nulltags-1.0)/2.0)
	if math.Abs(float64((opening-nulltags-1.0)/2.0)) > 10.0 {
		prb = (prb + 0.2) / 1.2
	} else if math.Abs(float64((opening-nulltags-1.0)/2.0)) > 100.0 {
		prb = (prb + 0.5) / 1.5
	}

	return
}

func assessJSON(input string) (prb float64) {
	// look for { and }
	opening := strings.Count(input, "{")
	closing := strings.Count(input, "}")

	if opening+closing > 10 {
		prb = (prb + 0.2) / 1.2
	} else if opening > 30 || closing > 30 {
		prb = (prb + 0.8) / 1.8
	}

	D.Println("json: curly braces subsum:", opening-closing)

	// if the difference is less than lets say 5%

	if math.Abs(float64(opening-closing))/math.Max(float64(opening), float64(closing)) < 0.05 {
		prb = (prb + 0.1) / 1.1
	}
	// try to identify 'key: val,' lines
	// note: this is not a very exact regex, but we just need to approximate, so this works
	r := regexp.MustCompile("\"?[a-z,A-Z, ,_,-,0-9]*\"?:\\s*\"?[a-z,A-Z, ,_,-,0-9]*\"?,?")
	hits := r.FindAllString(input, -1)
	D.Println("json: key val pairs:", len(hits))
	if len(hits) > 10 {
		prb = (prb + 0.3) / 1.3
	} else if len(hits) > 100 {
		prb = (prb + 0.7) / 1.7
	}

	return
}
