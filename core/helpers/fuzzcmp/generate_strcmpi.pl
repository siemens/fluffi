# Copyright 2017-2020 Siemens AG
# 
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including without
# limitation the rights to use, copy, modify, merge, publish, distribute,
# sublicense, and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
# SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
# 
# Author(s): Thomas Riedmaier

use strict;
use warnings;

open(my $file, ">", "strcmpi.cpp") or die "Could not open out file ";

my $maxdetailedSteps=10;
my $mystrcmpiCopies = 31; # make it something that will result in good modulo spread (e.g. not 10 but 11)

print $file "#include \"stdafx.h\"\n";
print $file "#include \"fuzzcmp.h\"\n";
print $file "#include \"strcmpi.h\"\n";


print $file "volatile int lastUsedStrcmpiFunc=-1;\n";
for (my $k=0; $k < $mystrcmpiCopies; $k++){

	foreach my $lr ( "left","right"){
		print $file "int mystrcmpi".$k.$lr."(const char * str1, const char * str2) {\n";
		print $file "lastUsedStrcmpiFunc=".$k."+".( ($lr eq "left") ? 0 : $mystrcmpiCopies).";\n";
		for (my $j=0; $j < $maxdetailedSteps; $j++){
			if($j != 0){
				print $file "char".$j.":\n";
			}
			print $file "   char str1".$j."low = tolower(str1[".$j."]);\n";
			print $file "   char str2".$j."low = tolower(str2[".$j."]);\n";
			print $file "	switch (str1".$j."low) {\n";
			print $file "	case 0: {goto char".$j."StrNoMatch;}\n";
			for ( my $i=-128;$i<128;$i++){ 
				if ($i == 0){
					next;
				}
				print $file "	case ".$i.":{if (str2".$j."low == ".$i."){goto char".($j+1).";}else {goto char".$j."StrNoMatch;}}\n";
			}
			print $file "}\n";
			print $file "char".$j."StrNoMatch:\n";
			print $file "	return str1".$j."low - str2".$j."low;\n";
		}
		print $file "char".$maxdetailedSteps.":\n";
		print $file "	str1+=".$maxdetailedSteps.";\n";
		print $file "	str2+=".$maxdetailedSteps.";\n";
		print $file "	while(*str1 && (tolower(*str1) == tolower(*str2)))\n";
		print $file "	{\n";
		print $file "		str1++;\n";
		print $file "		str2++;\n";
		print $file "	}\n";
		print $file "	return tolower(*str1) -  tolower(*str2);\n";
		print $file "}\n\n";
	}
}

print $file "typedef int strcmpiTYPE(const char *, const char *);\n";


print $file "strcmpiTYPE *sli[".$mystrcmpiCopies."] = { ";
for (my $k=0;$k < ($mystrcmpiCopies-1);$k++){
	print $file " mystrcmpi".$k."left, ";
}
print $file " mystrcmpi".($mystrcmpiCopies-1)."left };\n";


print $file "strcmpiTYPE *sri[".$mystrcmpiCopies."] = { ";
for (my $k=0;$k < ($mystrcmpiCopies-1);$k++){
	print $file " mystrcmpi".$k."right, ";
}
print $file " mystrcmpi".($mystrcmpiCopies-1)."right };\n";


print $file "\nint __cdecl mystrcmpi_(size_t caller, const char * str1, const char * str2){\n";
print $file "size_t rva = addrToRVA(caller);\n";
print $file "int leftmatch = sli[rva % ".$mystrcmpiCopies."](str1, str2);\n";
print $file "int rightmatch = sri[rva % ".$mystrcmpiCopies."](str2, str1);\n";
print $file "return leftmatch | rightmatch;\n";
print $file "}\n";

close $file
