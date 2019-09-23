/*
Copyright 2017-2019 Siemens AG

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Author(s): Thomas Riedmaier, Pascal Eckmann
*/

#include <vector>
#include <string.h>
#include <string>
#include <fstream>

//g++ --std=c++11 -o fluffiTester fluffiTester.cpp

std::vector<char> readAllBytesFromFile(std::string filename)
{
	std::ifstream ifs(filename, std::ios::binary | std::ios::ate);
	if (!ifs.is_open()) {
		return{};
	}

	std::ifstream::pos_type fileSize = ifs.tellg();

	if ((unsigned int)fileSize == 0) {
		return{};
	}

§§	std::vector<char> result((unsigned int)fileSize);

	ifs.seekg(0, std::ios::beg);
§§	ifs.read((char*)&result[0], fileSize);

	return result;
}

§§void parseInput(char* recvbuf) {

	char bufferThatIsTooSmall[0x42];

	if (recvbuf[0]==0x41) {
		strcpy(bufferThatIsTooSmall, recvbuf);

		for (int i = 0; i < sizeof(bufferThatIsTooSmall); i++) {
			bufferThatIsTooSmall[i]++;
		}

		strcpy( recvbuf, bufferThatIsTooSmall);
	}


}

int main(int argc, char* argv[])
{
    if(argc<2){
        printf("No file to parse specified");
	return -1;
    }

    std::vector<char> fileContent = readAllBytesFromFile(argv[1]);
    parseInput(&fileContent[0]);

    return 0;
}
