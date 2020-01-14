# Copyright 2017-2019 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
# Author(s): Thomas Riedmaier

use strict;
use warnings;

open(my $file, ">", "memcmp.cpp") or die "Could not open out file ";
my $maxdetailedSteps = 10;
my $mymemcmpCopies = 31; # make it something that will result in good modulo spread (e.g. not 10 but 11)

print $file "#include \"stdafx.h\"\n";
print $file "#include \"fuzzcmp.h\"\n";
print $file "#include \"memcmp.h\"\n";


print $file "volatile int lastUsedMemcmpFunc=-1;\n";
for (my $k=0; $k < $mymemcmpCopies; $k++){

	foreach my $lr ( "left","right"){
		print $file "int mymemcmp".$k.$lr."(const unsigned char * ptr1, const unsigned char * ptr2, size_t num) {\n";
		print $file "	lastUsedMemcmpFunc=".$k."+".( ($lr eq "left") ? 0 : $mymemcmpCopies).";\n";
		for (my $j=0; $j < $maxdetailedSteps; $j++){
			if($j != 0){
				print $file "char".$j.":\n";
			}
			print $file "	if(num == ".$j.") return 0;\n";
			print $file "	switch (ptr1[".$j."]) {\n";
			for ( my $i=0;$i<256;$i++){
				print $file "	case ".$i.": {if (ptr2[".$j."] == ".$i.") {goto char".($j+1)."; } else { goto char".$j."MemNoMatch;}}\n";
			}
			print $file "}\n";
			print $file "char".$j."MemNoMatch:\n";
			print $file "	return ptr1[".$j."] < ptr2[".$j."] ? -1 : 1;\n";
		}
		print $file "char".$maxdetailedSteps.":\n";
		print $file "	ptr1+=".$maxdetailedSteps.";\n";
		print $file "	ptr2+=".$maxdetailedSteps.";\n";
		print $file "	num-=".$maxdetailedSteps.";\n";
		print $file "	while(num-- > 0)\n";
		print $file "	{\n";
		print $file "		if (*ptr1++ != *ptr2++) {return ptr1[-1] < ptr2[-1] ? -1 : 1;}\n";
		print $file "	}\n";
		print $file "	return 0;\n";
		print $file "}\n\n";
	}
}


print $file "typedef int memcmpTYPE(const unsigned char * ptr1, const unsigned char * ptr2, size_t num);\n";


print $file "memcmpTYPE *ml[".$mymemcmpCopies."] = { ";
for (my $k=0;$k < ($mymemcmpCopies-1);$k++){
	print $file " mymemcmp".$k."left, ";
}
print $file " mymemcmp".($mymemcmpCopies-1)."left };\n";


print $file "memcmpTYPE *mr[".$mymemcmpCopies."] = { ";
for (my $k=0;$k < ($mymemcmpCopies-1);$k++){
	print $file " mymemcmp".$k."right, ";
}
print $file " mymemcmp".($mymemcmpCopies-1)."right };\n";


print $file "\nint __cdecl mymemcmp_(size_t caller, const void * ptr1, const void * ptr2, size_t num){\n";
print $file "size_t rva = addrToRVA(caller);\n";
print $file "int leftmatch = ml[rva % ".$mymemcmpCopies."](static_cast<const unsigned char *>(ptr1), static_cast<const unsigned char *>(ptr2), num);\n";
print $file "int rightmatch = mr[rva % ".$mymemcmpCopies."](static_cast<const unsigned char *>(ptr1), static_cast<const unsigned char *>(ptr2), num);\n";
print $file "return leftmatch | rightmatch;\n";
print $file "}\n";

close $file
