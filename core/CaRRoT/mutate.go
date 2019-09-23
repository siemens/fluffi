/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Roman Bendt, Thomas Riedmaier
*/

package main

import (
	"crypto/rand"
	"encoding/binary"
	"math/big"
)

func ran(max uint64) uint64 {
	if max == 0 {
		return 0
	}
	n, _ := rand.Int(rand.Reader, big.NewInt(int64(max)))
	return uint64(n.Int64())
}

type Mutator interface {
	Do(hash string, file []byte) (mutated []byte)
}

func randomMutationEngine(hash string) Mutator {
	var engines []Mutator

	if len(db.Files[hash].Stringz.S) > 0 && !db.Files[hash].IsPureAscii && !db.Files[hash].IsPureUnicode && !db.Files[hash].IsPureUtf8 {
		engines = append(engines, StringM{})
	}

	if len(engines) == 0 {
		engines = append(engines, DummyM{})
	}

	return engines[ran(uint64(len(engines)))]
}

type DummyM struct{}

func (d DummyM) Do(hash string, b []byte) (ret []byte) {
	L.Println("DummyMutator running")

	switch ran(2) {
	case 0: // leave filelength as is
		finish := ran(uint64(len(b)))
		start := ran(finish)
		ret = make([]byte, len(b))
		copy(ret, b)
		for i := start; i < finish; i++ {
			ret[i] = byte(ran(255))
		}
	case 1: // change file length
		nl := ran(uint64(2 * len(b)))
		ret = make([]byte, nl)
		if nl > uint64(len(b)) {
			split := ran(uint64(len(b)))

			copy(ret[:split], b[:split])
			copy(ret[nl-(uint64(len(b))-split):], b[split:])

			for i := split; i < nl-(uint64(len(b))-split); i++ {
				ret[i] = byte(ran(255))
			}
		} else {
			ofs := ran(uint64(len(b)) - nl)
			copy(ret, b[ofs:])
		}

	}

	return ret
}

type StringM struct{}

func (m StringM) Do(hash string, file []byte) (mutated []byte) {
	L.Println("StringMutator running")
	mutated = make([]byte, 0)

	// write nonstrings to string
	// exchange strings, matching length fields
	// blow up string and correct length fields
	blks := db.Files[hash].Blockz.B
	if len(blks) > 0 {
		blk := blks[ran(uint64(len(blks)))]

		if len(blk.Length) > 0 && len(blk.T) > 0 {
			var newlength uint64

			t := blk.T[0]
			// modify string
			switch ran(15) {
			case 0: // leave
				D.Println("StringMutator choosing to leave string at", t.Pos, "unmodified")
				mutated = make([]byte, len(file))
				copy(mutated, file)
				newlength = t.Len
			case 1: // shorter
				fallthrough
			case 2:
				short := ran(t.Len)
				D.Println("StringMutator choosing to shorten string at", t.Pos, "to", short)
				newlength = short
				mutated = make([]byte, uint64(len(file))-t.Len+short)
				copy(mutated[:t.Pos+short], file[:t.Pos+short])
				copy(mutated[t.Pos+short:], file[t.Pos+t.Len:])
			case 3: // longer
				fallthrough
			case 4:
				fallthrough
			case 5:
				fallthrough
			case 6:
				long := t.Len + ran(db.Files[hash].Size)
				D.Println("StringMutator choosing to lengthen string at", t.Pos, "to", long)
				newlength = long
				bs := fillString(hash, t, long)
				mutated = make([]byte, uint64(len(file))-t.Len+long)
				copy(mutated[:t.Pos], file[:t.Pos])
				copy(mutated[t.Pos:t.Pos+uint64(len(bs))], bs)
				copy(mutated[t.Pos+uint64(len(bs)):], file[t.Pos+t.Len:])
			case 7: // absent
				D.Println("StringMutator choosing to remove string at", t.Pos)
				mutated = make([]byte, uint64(len(file))-t.Len)
				copy(mutated[:t.Pos], file[:t.Pos])
				copy(mutated[t.Pos:], file[t.Pos+t.Len:])
				newlength = 0
			case 8: // reeeeeeeeally long
				long := t.Len + ran(1000*db.Files[hash].Size)
				D.Println("StringMutator choosing to lengthen string extremely at", t.Pos, "to", long)
				newlength = long
				bs := fillString(hash, t, long)
				mutated = make([]byte, uint64(len(file))-t.Len+long)
				copy(mutated[:t.Pos], file[:t.Pos])
				copy(mutated[t.Pos:t.Pos+uint64(len(bs))], bs)
				copy(mutated[t.Pos+uint64(len(bs)):], file[t.Pos+t.Len:])
			case 9: // ctrl chars
				fallthrough
			case 10:
				fallthrough
			case 11:
				bs := fillCtrl(t)
				D.Println("StringMutator choosing to replace string at", t.Pos, "with ctrl chars")

				mutated = make([]byte, len(file))
				copy(mutated[:t.Pos], file[:t.Pos])
				copy(mutated[t.Pos:t.Pos+t.Len], bs)
				copy(mutated[t.Pos+t.Len:], file[t.Pos+t.Len:])
				newlength = t.Len
			case 12: // nonchar/invalid chars
				fallthrough
			case 13:
				fallthrough
			case 14:
				bs := fillInvalid(t)
				D.Println("StringMutator choosing to replace string at", t.Pos, "with invalid chars")
				mutated = make([]byte, len(file))
				copy(mutated[:t.Pos], file[:t.Pos])
				copy(mutated[t.Pos:t.Pos+t.Len], bs)
				copy(mutated[t.Pos+t.Len:], file[t.Pos+t.Len:])
				newlength = t.Len
			default:
				D.Println("StringMutator: something went wrong on string replacer.")
			}

			if len(blk.Length) > 0 {

				changes := uint64(1)
				if len(blk.Length) > 2 {
					changes = ran(uint64(len(blk.Length) / 2))
				}
				for i := uint64(0); i < changes; i++ {
					l := blk.Length[ran(uint64(len(blk.Length)))]
					// modify length field
					switch ran(11) {
					case 0: // leave
						D.Println("StringMutator choosing to leave length field at", l.Pos)
					case 1: // smaller
						fallthrough
					case 2:
						fallthrough
					case 3:
						D.Println("StringMutator choosing to write smaller length field at", l.Pos)
						writeNewValue(mutated, l, newlength-ran(newlength))
					case 4: // fitting
						D.Println("StringMutator choosing to write correct length field at", l.Pos)
						writeNewValue(mutated, l, newlength)
					case 5: // greater
						fallthrough
					case 6:
						D.Println("StringMutator choosing to write larger length field at", l.Pos)
						writeNewValue(mutated, l, newlength+ran(100*newlength))
					case 7: // zero
						fallthrough
					case 8:
						fallthrough
					case 9:
						D.Println("StringMutator choosing to zero length field at", l.Pos)
						for i := uint64(0); i < l.ByteW; i++ {
							mutated[l.Pos+i] = 0x00
						}
					case 10: // max
						D.Println("StringMutator choosing to max out length field at", l.Pos)
						for i := uint64(0); i < l.ByteW; i++ {
							mutated[l.Pos+i] = 0xff
						}
					default:
						D.Println("StringMutator: something went wrong on int replacer.")
					}
				}
			}

			switch ran(2) {
			case 0: // remove terminator if present
				t := blk.T[0]
				if t.Term {
					D.Println("StringMutator choosing to remove terminating bytes")
					firstnonterm := t.Pos + newlength
					for firstnonterm < uint64(len(mutated)) {
						if mutated[firstnonterm] != 0x00 {
							break
						}
						firstnonterm++
					}
					D.Println("Pos", t.Pos, "len", newlength, "nonterm", firstnonterm, "len mutated", len(mutated))
					mutated = append(mutated[:t.Pos+newlength], mutated[firstnonterm:]...)
				} else {
					D.Println("StringMutator choosing to remove terminating bytes, but no terminator detected")
				}

			case 1: // do nothing
				D.Println("StringMutator choosing to leave terminating bytes")
			}
		}

	} else {
		D.Println("StringMutator can't do anything if there are zero blocks")
	}

	return
}

func writeNewValue(target []byte, i Int, val uint64) {
	switch i.Enc {
	case "uint16be":
		fallthrough
	case "int16be":
		binary.BigEndian.PutUint16(target[i.Pos:], uint16(val))
	case "uint32be":
		fallthrough
	case "int32be":
		binary.BigEndian.PutUint32(target[i.Pos:], uint32(val))
	case "uint64be":
		fallthrough
	case "int64be":
		binary.BigEndian.PutUint64(target[i.Pos:], val)
	case "uint16le":
		fallthrough
	case "int16le":
		binary.LittleEndian.PutUint16(target[i.Pos:], uint16(val))
	case "uint32le":
		fallthrough
	case "int32le":
		binary.LittleEndian.PutUint32(target[i.Pos:], uint32(val))
	case "uint64le":
		fallthrough
	case "int64le":
		binary.LittleEndian.PutUint64(target[i.Pos:], val)
	}
}

func fillCtrl(t Text) (b []byte) {
	b = make([]byte, t.Len)
	for i := uint64(0); i < t.Len; i++ {
		b[i] = byte(ran(0x20))
	}
	return
}

func fillInvalid(t Text) (b []byte) {
	switch t.Enc {
	case "ascii":
		b = make([]byte, t.Len)
		for i := uint64(0); i < t.Len; i++ {
			b[i] = byte(ran(255)) | 0x80
		}
	case "utf8":
		b = make([]byte, t.Len)
		for i := uint64(0); i < t.Len; i++ {
			b[i] = (byte(ran(255)) | 0x80) & 0xBF // make every char an extended one, but without the start sequence
		}
	case "unicode":
		// utf16 basically allows everything :/ so we just fill it with random shit.
		b = make([]byte, t.Len)
		for i := uint64(0); i < t.Len; i++ {
			b[i] = byte(ran(255))
		}
	}
	return
}
func fillString(hash string, t Text, length uint64) (b []byte) {
	switch t.Enc {
	case "ascii":
		b = make([]byte, length)
		copy(b[:t.Len], db.Files[hash].Content[t.Pos:t.Pos+t.Len])
		for i := t.Len; i < length; i++ {
			b[i] = byte(ran(0x7E-0x20) + 0x20)
			//b[i] = db.Files[hash].Content[t.Pos+(i%t.Len)]
		}
	case "utf8":
		b = make([]byte, length)
		copy(b[:t.Len], db.Files[hash].Content[t.Pos:t.Pos+t.Len])
		for i := t.Len; i < length; {
			switch ran(10) {
			case 0:
				if length-i >= 4 {
					b[i] = (byte(ran(255)) | 0xF0) & 0xF7
					i++
					b[i] = (byte(ran(255)) | 0x80) & 0xBF
					i++
					b[i] = (byte(ran(255)) | 0x80) & 0xBF
					i++
					b[i] = (byte(ran(255)) | 0x80) & 0xBF
					i++
					continue
				}
				fallthrough
			case 1:
				fallthrough
			case 2:
				if length-i >= 3 {
					b[i] = (byte(ran(255)) | 0xE0) & 0xEF
					i++
					b[i] = (byte(ran(255)) | 0x80) & 0xBF
					i++
					b[i] = (byte(ran(255)) | 0x80) & 0xBF
					i++
					continue
				}
				fallthrough
			case 3:
				fallthrough
			case 4:
				fallthrough
			case 5:
				if length-i >= 2 {
					b[i] = (byte(ran(255)) | 0xC0) & 0xDF
					i++
					b[i] = (byte(ran(255)) | 0x80) & 0xBF
					i++
					continue
				}
				fallthrough
			case 6:
				fallthrough
			case 7:
				fallthrough
			case 8:
				fallthrough
			case 9:
				b[i] = byte(ran(0x7E-0x20) + 0x20)
				i++
			}
		}
	case "unicode":
		b = make([]byte, length)
		copy(b[:t.Len], db.Files[hash].Content[t.Pos:t.Pos+t.Len])
		for i := t.Len; i < length; i++ {
			if b[i-1] == 0x00 {
				r := ran(0x7E-0x20) + 0x20
				b[i] = byte(r)
			} else {
				b[i] = 0x00
			}
		}

	}
	return
}
