# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Thomas Riedmaier, Roman Bendt

native:
	@CGO_ENABLED=0 go build -v -a -ldflags '-s -w -extldflags "-static"' ./cmd/Oedipus

release:
	@CGO_ENABLED=0 GOARCH=386 GOOS=linux go build -o Oedipus-386 -v -a -ldflags '-s -w -extldflags "-static"' ./cmd/Oedipus
	@CGO_ENABLED=0 GOARCH=amd64 GOOS=linux go build -o Oedipus-amd64 -v -a -ldflags '-s -w -extldflags "-static"' ./cmd/Oedipus
	@CGO_ENABLED=0 GOARCH=arm GOOS=linux go build -o Oedipus-arm -v -a -ldflags '-s -w -extldflags "-static"' ./cmd/Oedipus
	@CGO_ENABLED=0 GOARCH=arm64 GOOS=linux go build -o Oedipus-arm64 -v -a -ldflags '-s -w -extldflags "-static"' ./cmd/Oedipus

windows:
	@CGO_ENABLED=0 GOARCH=386 GOOS=windows go build -o Oedipus-win86 -v -a -ldflags '-s -w -extldflags "-static"' ./cmd/Oedipus
	@CGO_ENABLED=0 GOARCH=amd64 GOOS=windows go build -o Oedipus-win64 -v -a -ldflags '-s -w -extldflags "-static"' ./cmd/Oedipus

clean:
	@rm -f Oedipus Oedipus-386 Oedipus-amd64 Oedipus-arm Oedipus-arm64
