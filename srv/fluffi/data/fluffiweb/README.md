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

Author(s): Junes Najah, Thomas Riedmaier, Pascal Eckmann
-->

# Fluffi Web GUI

FLUFFI - A distributed evolutionary binary fuzzer for pentesters.

## Local development

### Prerequisites for local dev

- Install Python Version >= 3.5 

If you also have a old python 2.7 version installed, use pip3 and python3 to start the app and on unix `sudo` before the pip command

```
$ pip install -r requirements.txt
```

- Setup local mysql database with username `root` and password `toor` 
- Run get_static_dependencies.bat or get_static_dependencies.sh 
- Set LOCAL_DEV to True in [config.py](config.py)

To run the app on localhost:5000: 
```
$ python main.py
```

## Testing

Prerequisites:

- install selenium with `pip install selenium`
- download [geckodriver](https://github.com/mozilla/geckodriver/releases) and add it to your PATH environment variables (if necessary)
- add test population file for the upload under `C:\TestDev\test_files\example1.dll` or change path to your file

Run unit tests:

```
$ python -W ignore -m unittest discover
```

## Config

Add new RunnerTypes, GeneratorTypes, EvaluatorTypes or Templates to [config.json](app/static/config.json).
