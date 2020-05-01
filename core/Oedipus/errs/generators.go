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

package errs

import (
	"log"
	"os"
	"strconv"
)

var tf *os.File

func FlogOpen()  {
	pid := os.Getpid()
	err := os.MkdirAll("oedilogs", 0755)
	if err != nil {
		log.Fatalln("error creating log dir:", err.Error())
	}
	tf, err := os.OpenFile("oedilogs/oedipus-"+strconv.Itoa(pid)+".log", os.O_RDWR | os.O_CREATE | os.O_APPEND, 0666)
	if err != nil {
		log.Fatalln("error opening logfile:", err.Error())
	}
	log.SetOutput(tf)
}

func FlogClose() {
	log.SetOutput(os.Stderr)
	tf.Close()
}

func Foe(prefix string) (func(e error, msg string), func(msg string)) {

	return func(e error, msg string) {
			if e != nil {
				log.Fatalln(prefix, msg, e.Error())
			}
		}, func(msg string) {
			log.Fatalln(prefix, msg)
		}
}
