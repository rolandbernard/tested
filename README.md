Tested
======
This is a small program to run language independent file based tests.

## Building from source
```
$ git clone https://github.com/rolandbernard/tested
$ cd tested
$ make BUILD=release
$ ./build/release/bin/tested --help
```

## Usage
The test runner will run all the test inside the given directories.
Test instructions are specified using line comments at the very beginning of a file.
Every file that begins with a line comment of the form `// test` is considered a test (`//` can also be either `#`, `;` or `REM`).  
The first line can also contain a name for the test: `test: this is a name for this test`.
Every line that follows this an is also a line comment will be interpreted as either an instruction or constraint.
In every directory the program will also try to load a file named `tested.default` which can contain default options and constraint.
In the build, run and cleanup instrunctions single `%` characters will be replaced with the filepath of the test.

Constraint are specified using `=`, `!=`, `>=`, `>`, `<=` or `<` followed by a value.
Every type of constraint will only exist once and when setting the same constraint multiple times, 
only the last one will be considered. 
Setting a `=` constraint after having set a `!=` constraint will also overwrite the original `=` constraint.
Time can be specified using the `s`, `ms` or `us` units (e.g.: `1234ms`, `5s`, `100000us`).

The following instructions are understood:
* `test: string` sets the name of the test. (e.g.: `test: a test name`)
* `runs: number` sets the number of times the test should be run. (e.g.: `runs: 42`)
* `build: command` sets the build command (executed before run). (e.g.: `build: gcc -o %.out %`)
* `run: command` sets the run command. (e.g. `run: %.out`)
* `cleanup: command` sets the cleanup command (executed after run). (e.g.: `cleanup: rm %.out`)
* `stdin: string` sets the input to be given to the run command. (e.g.: `stding: hello`)
* `timesout: boolean` if true, the run command is expected to timeout (e.g.: `timesout: true`)
* `buildtimesout: boolean` if true, the build command is expected to timeout (e.g.: `buildtimesout: true`)

The following constraints are understood:
* `buildtime: time constraints` constrains the real time of the build command. (e.g.: `buildtime: < 1s`)
* `time: time constraints` constrains the real time of the run command. (e.g.: `time: < 100s`)
* `exit: number constraints` constrains the exit code of the run command. (e.g.: `exit: = 0`)
* `stderr: string constraints` constrains the stderr of the run command. (e.g.: `stderr: =`)
* `stdout: string constraints` constrains the stdout of the run command. (e.g.: `stdout: = Hello world\n`)
