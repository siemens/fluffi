# Fuzzcmp

## The problem

Many targets compare input values to magic constants. E.g., strcmp(method,"POST"), or memcmp(method,post,4).

These compares are very difficult to handle for a coverage-based black-box fuzzer. The reason therefore is that these functions are essentially a loop with two possible exits: Success or Fail.

```c
  do
    {
      c1 = (unsigned char) *s1++;
      c2 = (unsigned char) *s2++;
      if (c1 == '\0')
        return c1 - c2;
    }
  while (c1 == c2);
```

FLUFFI, as a coverage based fuzzer treats any input as interesting that generates new/different coverage. However, strcmp('totallywrong!','totallyright!') and strcmp('totallyright$','totallyright!') produce exactly the same coverage. So after trying 'totallywrong!' there is no incentive for the fuzzer to mutate the input towards 'totallyright!'.


## The solution

So we played a little bit around with alternative implementations for compare functions and came up with fuzzcmp. fuzzcmp is a collection of alternative implementations, e.g., for strcmp, that produce exactly the same output as the original functions. However, in the process of doing so, the fuzzcmp functions utilize a lot of basic blocks. This gives FLUFFI an incentive to move towards the hard coded values.

## The testcase

We created the following small test program:

```c
int main(int argc, char* argv[])
{

	if (argc < 2) {
		printf("Need some input\n");
		return -1;
	}

	std::vector<char> inputFromFile = readAllBytesFromFile(argv[1]);
	std::string inputFromFile_s(inputFromFile.begin(), inputFromFile.end());

	std::vector<std::string> splittedArgv = splitString(inputFromFile_s, "-");
	if (splittedArgv.size() < 3) {
		printf("Need at least three \"-\" in input\n");
		return -1;
	}

	if (0 == strcmp(splittedArgv[0].c_str(), "It's") && 0 == strcmp(splittedArgv[1].c_str(), "actual") && 0 == strcmp("magic", splittedArgv[2].c_str())) {
		*(int *)0x1234 = 14;
	}
	else {
		printf("Sorry try again\n");
	}

	return 0;
}
```

### 1) Running without fuzzcmp

We fuzzed the program with FLUFFI without using fuzzcmp. The result thereof is shown here:

![Population and coverage when running the testcase program without fuzzcmp](without_fuzzcmp.png)

As you can see, the coverage increased at the beginning up to about 500 blocks and then didn't change anymore for almost two weeks. Furhtermore, you can see, that the crash, i.e. the magic string, was never discovered.
 
### 2) Running with fuzzcmp

Then we fuzzed the same program while using fuzzcmp.The result thereof is shown here:

![Population and coverage when running the testcase program with fuzzcmp](with_fuzzcmp.png)

As you can see, there was a constant increase of blocks and FLUFFI step-by-step figured out the magic string. Within 12 hours, the first crashes appeared (meaning, the magic string was discovered).

As you can see also, the overal amount of covered blocks is significantly higher than when running without fuzzcmp.

## Compiling

### 1) Windows

Compile the fuzzcmp.sln. If you want to do this on command line look in the [build.bat](../../build.bat) for how this is done.

### 2) Linux 

Look in the  [fuzzcmp.cpp](fuzzcmp.cpp). It contains the build commands above the c code.


Note: You should check if your target uses strong symbols, e.g. strcmp@@GLIBC_2.2.5 . If that is the case you need to adapt the [libfuzzcmp.version](libfuzzcmp.version).

## Usage

### 1) Windows

You need to inject the fuzzcmp.dll into the target process. It will then automatically hijack all calls to compare functions.

An easy way to do so is to use the APPInit_DLLs mechanism.

### 2) Linux

Use LD_PRELOAD, e.g. 
```
LD_PRELOAD=./libfuzzcmp.so ./yourtarget
```


