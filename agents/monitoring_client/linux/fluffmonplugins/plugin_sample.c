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

Author(s): Pascal Eckmann, Thomas Riedmaier
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// compile with 
// gcc -o plugin_c_sample -static plugin_sample.c

int main(int argc, char** argv, char** envp) {
	if (argc < 2) {
		return 1;
	}

	if (!strcmp(argv[1], "API")) {
		printf("FLUFFIMONITORING");
	} else if (!strcmp(argv[1], "TYPE")) {
		printf("ALERT");
	} else if (!strcmp(argv[1], "NAME")) {
		printf("C_IS_FUN");
	} else if (!strcmp(argv[1], "RUN")) {
		printf("YOUR PC IS BURNING");
	} else {
		return 1;
	}
}
