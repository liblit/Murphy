Murphy v1.0
===========

Written by:

* [Zach Miller](mailto:zmiller@cs.wisc.edu)
* [Todd Tannenbaum](mailto:tannenba@cs.wisc.edu)
* with help from [Ben Liblit](mailto:liblit@cs.wisc.edu)

Please contacts us with questions or comments!


Subdirectories and Licenses
---------------------------

* `classads-1.0.10`

    This is the ClassAd Library source, used as the policy language in Murphy.  Distributed via the Apache 2.0 License from <http://research.cs.wisc.edu/condor/classad/>.

* `murphy-1.0`

    This is the Murphy source code, implemented as a derivative work of the Parrot tool included as part of the Cooperative Computing Tools (cctools).  cctools is released under the GPL v2 License from <http://nd.edu/~ccl/software/>.


To Build
--------

On a 64-bit Linux system, in this subdirectory enter:

    ./build_murphy

If all goes well with the build, the binary “`Murphy`” will end up in this subdirectory.

__Note:__ Murphy will not run properly on a 32-bit system.


To Run
------

To use Murphy, quick and dirty, in this directory enter “`source setup.sh`” or “`source setup.csh`”, then “`Murphy ./my_program`”.


More Detailed Info
------------------

### Gremlin Configuration File ###

The environment variable `MURPHY_CONFIG` should point to a gremlin configuration file.  There is an example in `murphy_config.example` that has all gremlins listed but disabled.  You can modify this according to what you want.

The format is one entry per line, with four colon-delimited fields on each line:

    gremlin:percent:seed:constraint

1. `gremlin` can be: `readone`, `readless`, `readone_s`, `writeone`, `writeone_s`, `writezero`, `eintr`, `eagain`, `enospc`, `closefail`, or `selectfdset`

2. `percent` is an integer between 0 and 100 inclusive, if you want random invocation.

3. `seed` is an integer used to seed the pseudo-random number generator

4. `constraint` is a classad constraint which must evaluate to true in order for the gremlin to be invoked.  See the [classad language and reference manual](http://research.cs.wisc.edu/condor/classad/refman/).

In the configuration file, blank lines and lines starting with “`#`” are ignored.

### Running Murphy ###

Add the directory containing `Murphy` to your path.  This isn’t strictly necessary, but is probably a good idea and will save you time.

To support programs that use `vfork()`, you must set the environment variable `PARROT_HELPER` to point to the file `libparrot_helper.so`.

To run Murphy, use “`Murphy [options] <command> ...`”, where options and environment variables are:

* `-c <file>`

    Path to Murphy config file, defaulting to `$MURPHY_CONFIG`.

* `-g <file>`

    Send gremlin activation log to this file.

* `-r <file>`

    Replay a gremlin activation log from this file.

* `-a`

    Attach with `gdb` when replay of a gremlin activation log completes.

* `-d <name>`

    Enable debugging for the named sub-system, one of: `syscall`, `notice`, `process`, `pstree`, `alloc`, `cache`, `poll`, `debug`, `murphy`, `user`, `all`, `time`, and `pid`.

    The most useful ones for Murphy are “`-d murphy`” and “`-d syscall`”.

* `-H`

    Disable use of helper library.

* `-h`

    Show brief help information.

* `-l <path>`

    Path to `ld.so` to use.

* `-o <file>`

    Send debugging messages to this file, defaulting to `stderr`.

* `-O <bytes>`

    Rotate debug files of this size.

### Run-Time Steering ###

The Murphy run-time control function is `mkdir()`.  Four commands are currently implemented:

* `mkdir /Murphy/set-metadata/<MeTaDaTa>`

    Set the per-process metadata to the string “`<MeTaDaTa>`”.  The string is limited to 1000 characters, but any non-null character is legal.

* `mkdir /Murphy/update-config/<config-entry>`

    Dynamically set a new config entry, using the same format that is used in the `murphy_config` file.

* `mkdir /Murphy/suspend`

    Suspend your program and detach Murphy.  You are now free to attach with your own debugger, send it `SIGCONT` to continue, kill it, etc.  Don’t forget to clean it up or it will lurk forever.

* `mkdir /Murphy/suspend-and-debug`

    Detach `Murphy` from the program under test, then reattach to it with `gdb`.  This is useful for most people and allows you to inspect data, get a stack trace, etc. 
