<!---
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

Author(s): Thomas Riedmaier, Junes Najah
-->

# Vision
Technological: `Build a fuzzer that can be used for security assessments / pentests of typical Siemens targets`

Business: `Every Siemens parser (embedded or binary) leaving the company will be fuzzed with our fuzzer`

# Mission
Native binaries parsing attacker-controlled input are potentially vulnerable to memory corruption attacks. This class of attack may give remote attackers full control of a system.

The mission of this project is to provide a solution that allows experts to test said binaries for this class of vulnerabilities. As our solution is meant to be optimized for security assessments of typical Siemens targets, it is designed for minimal setup effort and maximal target diversity (see also "Project summary").

# Target group / customers
Intended users of our system: 
- Penetration testers

Intended customers:
- Vulnerability Management (Red-Teaming)
- Product Management / Product Owners (e.g., for certification)
- People ordering penetration tests
- Developers that implement parsers (e.g., for integration tests)

Intended interface to FLUFFI:
a) Customer provides binaries that users turn into fuzzable binaries
b) Customer provides fuzzable version of binary (e.g. developers that implement parsers)

# Strategy
Our project is meant to provide both:
- a platform for research on cutting-edge fuzzing technologies
- a system that allows us to offer fuzzing-as-a-service.


# Project Summary


FLUFFI (Fully Localized Utility For Fuzzing Instantaneously) is a distributed feedback-based evolutionary fuzzer developed by Siemens STT (formerly CSA) designed specifically for the SIEMENS environment.


**"Designed specifically for the SIEMENS environment"** means that the fuzzer is optimized for those challenges that we usually face during our penetration tests / Red-Team activities for Siemens:
- Usually binary only
- Diversity of test targets: Windows/Linux, embedded/desktop, user-mode / kernel, different interfaces(file, network, API, ...)
- Limited time for harnessing in typical security assessments
- Fuzzing environment (hardware) needs to be available

**Feedback-based evolutionary fuzzer** in short means that FLUFFI does not require knowledge about what it fuzzes. It does not need to know that "The target's input is HTML, and HTML is specified as follows: ...".

Instead, FLUFFI monitors the behavior of the target program when confronted with a certain input. Currently, this means collecting all basic blocks the program used while processing an input (might be extended in the future). FLUFFI starts each FuzzJob with an initial population of known-to-be-valid inputs. Based on this set, FLUFFI creates input mutations, feeds them to the target and collects the covered basic block. Each mutation that created new coverage (i.e. new basic blocks) is subsequently added to the population.

Other popular feedback-based evolutionary fuzzers are AFL, WinAFL, and honggfuzz.

Pros:
- Can fuzz arbitrary (unknown) protocols and formats
- Evolutionary algorithm might be an interesting research area
- Testcase generation flexible (e.g. Concolic execution)

Cons:
- Needs a feedback loop (e.g. debugger, tracer)

### Further core design decisions:

####  Do not try to maximize test cases/sec. Instead try to generate inputs that are as good as possible.

As fuzzing binaries is slow, and fuzzing embedded system binaries is even slower, FLUFFI's principle design decisions are:

Pros:
- We can effectively fuzz slow binaries
- We can effectively fuzz embedded systems
- We can research fancy techniques such as concolic execution for testcase generation

Cons:
- Much less test cases/sec than e.g. AFL


####  Make FLUFFI a modular system that communicates over network

Pros:
- Test cases are not generated on the same system that runs them. A crash in a (kernel) runner does not lead to a loss of a crashing test case
- Binary fuzzing tends to mess up runner systems badly. Those should be restarted / reinitialized frequently while a fuzz job is running
- Costly testcase generation can be done on dedicated systems that do not run test cases 
- Embedded systems can be connected to the systems. They only need to run the runner and not the generator, etc...
- We can fuzz targets that require exclusive resources (such as "port 80" or "named pipe XYZ") in parallel without modifying them. This supports our vision of "minimal harnessing"
- Nicely scalable: Just plug in more components in a network
- Easy replacement and benchmarking of components (e.g. for research)
- Network communication would allow combining cloud systems (cheap, easily scalable) with real 
   systems (e.g. native ARM systems, hardware tracing, special hardware)

Cons:
- IO is high. An all-in-one process would be much faster. However, due to the first design decision of prioritizing quality over quantity, this is fine.


####  FLUFFI agents are stateless - Persistent Info is stored in DB to which only LM and GM connect

Pros:
- Agents may crash or disconnect - the system keeps working
- Runner systems can be power-cycled / reinstalled without losing data
- Agents can easily be added to the system for scalability
- DB bottlenecks can be solved with classical DB scaling (e.g., cluster)
- Info can easily be accessed and be visualized in a web application or used for sophisticated analysis

Cons:
- DB as data storage is very slow in comparison to in-memory storage


