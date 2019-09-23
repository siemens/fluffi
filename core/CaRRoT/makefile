# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Roman Bendt, Thomas Riedmaier

§§all:
§§	#@go test -bench=. ./...
§§	@find -type f -name '*.go' -exec goimports -w "{}" \;
§§	@go build
§§	#@GOOS=windows GOARCH=amd64 go build -o ~/VMShare/CaRRoT.exe .
§§
§§release:
§§	@CGO_ENABLED=0 GOARCH=386 GOOS=linux go build -o CaRRoT-386 -v -a -ldflags '-s -w -extldflags "-static"' .
§§	@CGO_ENABLED=0 GOARCH=amd64 GOOS=linux go build -o CaRRoT-amd64 -v -a -ldflags '-s -w -extldflags "-static"' .
§§	@CGO_ENABLED=0 GOARCH=arm GOOS=linux go build -o CaRRoT-arm -v -a -ldflags '-s -w -extldflags "-static"' .
§§	@CGO_ENABLED=0 GOARCH=arm64 GOOS=linux go build -o CaRRoT-arm64 -v -a -ldflags '-s -w -extldflags "-static"' .
§§
§§native:
§§	@CGO_ENABLED=0 go build -v -a -ldflags '-s -w -extldflags "-static"' .
§§
§§clean:
§§	@rm -f carrot.db carrot.gob carrot_* ~/VMShare/CaRRoT.exe CaRRoT CaRRoT-386 CaRRoT-amd64 CaRRoT-arm CaRRoT-arm64
