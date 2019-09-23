§§/*
§§Copyright 2017-2019 Siemens AG
§§
§§Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
§§
§§The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
§§
§§THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
§§
§§Author(s): Thomas Riedmaier, Pascal Eckmann
§§*/
§§
§§#define _GNU_SOURCE
§§#include <unistd.h>
§§#include<stdio.h>
§§#include<stdlib.h>
§§#include <sys/stat.h>
§§#include <string.h>
§§
§§/*
§§sudo su
§§cd /home/odroid/TestRoot/cpio-root
§§gcc -static -msoft-float -mfpu=vfpv3-d16 -o cgicaller cgicaller.c
§§chroot . ./cgicaller
§§
§§gdb --args chroot . /cgicaller
§§set stop-on-solib-events 1
§§break *0x9884
§§
§§*/
§§int main(int argc, char* argv[])
§§{
§§	if (argc < 2) {
§§		printf("Usage %s <Inputfile>\n", argv[0]);
§§		return -1;
§§	}
§§
§§	struct stat st;
§§	int exist = stat(argv[1], &st);
§§	if (exist != 0) {
§§		printf("%s can not be opened as input file\n", argv[1]);
§§		return -1;
§§	}
§§
§§	char content_length[32];
§§	memset(content_length, 0, sizeof(content_length));
§§	strcpy(content_length, "CONTENT_LENGTH=");
§§	sprintf(content_length + 15, "%d", (int)st.st_size);
§§
§§	//http://www.cgi101.com/book/ch3/text.html
§§	char* envp[] = { content_length, "CONTENT_TYPE=application/x-www-form-urlencoded","QUERY_STRING=","REMOTE_ADDR=127.0.0.1","REMOTE_PORT=22222","REQUEST_METHOD=POST" , NULL };
§§	char* target[] = { "/etc/httpd/swarm.cgi", NULL };
§§
§§	stdin = freopen(argv[1], "r", stdin);
§§
§§	execvpe(target[0], target, envp);
§§
§§	//should never reach this
§§	return 0;
§§}
