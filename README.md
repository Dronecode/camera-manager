## Camera Streaming Daemon ##

### Compilation and installation ###

In order to compile you need the following packages:

  - GCC or Clang compiler
  - C and C++ standard libraries

#### Build ####

Build system follows the usual configure/build/install cycle. Configuration is needed
to be done only once. A typical configuration is shown below:

    $ ./autogen.sh && ./configure CFLAGS='-g -O2' \
            --sysconfdir=/etc --localstatedir=/var --libdir=/usr/lib64 \
	    --prefix=/usr

Build:

    $ make

Install:

    $ make install
    $ # or... to another root directory:
    $ make DESTDIR=/tmp/root/dir install

### Running ###


### Contributing ###

Pull-requests are accepted on GitHub. Make sure to check coding style with the
provided script in tools/checkpatch, check for memory leaks with valgrind and
test on real hardware.
