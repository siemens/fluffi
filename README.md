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

Author(s): Thomas Riedmaier, Roman Bendt
-->

# FLUFFI

![FLUFFI greets you](srv/fluffi/data/fluffiweb/app/static/images/friendly_fluffi_md.jpg)

FLUFFI - A distributed evolutionary binary fuzzer for pentesters.

- [About the project](./docs/about.md)
- [High level overview](./docs/overview.md)
- [Getting started](./docs/getting_started.md)
- [Usage](./docs/usage.md)
- [HOWTOs](./docs/howtos/)
- [Technical Details](./docs/technical_details.md)
- [Contributing to FLUFFI](./CONTRIBUTING.md)
- [LICENSE](./LICENSE.md)


# Bugs found

So far, FLUFFI was almost exclusively used on SIEMENS products and solutions. Bugs found therein will not be published.

However, FLUFFI found the following published bugs (please help us keep this list up to date):
- [Buffer underflow in bc_shift_addsub](https://bugs.php.net/bug.php?id=78878)(CVE-2019-11046)
