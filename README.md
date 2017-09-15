## memctl-physmem-core

<!-- Brandon Azad -->

memctl-physmem-core is a [memctl] core for macOS up to version 10.12.1 that leverages the [physmem]
vulnerability to access the kernel task port. It will not work on macOS 10.12.2 or newer.

[memctl]: https://github.com/bazad/memctl
[physmem]: https://github.com/bazad/physmem

### Building

To build memctl using this core:

	$ git clone https://github.com/bazad/memctl
	$ cd memctl
	$ git clone https://github.com/bazad/memctl-physmem-core
	$ ln -s memctl-physmem-core core
	$ cd memctl-physmem-core
	$ make MACOSX_DEPLOYMENT_TARGET=10.12.1
	$ cd ..
	$ make ARCH=x86_64 SDK=macosx

### Running

You can run memctl without any arguments to drop into a REPL:

	$ bin/memctl
	memctl> fc AppleMobileFileIntegrity
	setting up kernel function call...
	0xffffff803485d0a0
	memctl> 

### License

memctl-physmem-core is released under the MIT license.
