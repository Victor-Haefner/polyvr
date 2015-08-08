# Generate a Makefile for Polyvr

## Generation

This uses the binary of cbp2make to easily generate a makefile out of the Polyvr.cbp file. Example:

    ./extcbp2make.linux-x86_64 -in ../../PolyVR.cbp -out ../../Makefile	// generate makefile

Instead of doing so, you can also use the `generate.sh` script.
To get more information about the generation or usage, use

    ./cbp2make.linux-x86_64 --help

Note that this is the linux binary of cbp2make. To get it for other systems, visit:

    http://sourceforge.net/projects/cbp2make/

## Usage

Examples:

    make debug    // complete debug process, inclusive before- and after-build.
    make -j 2 build_debug    // build process only, using two jobs.
    make clean
    make <TARGET>

Target list:

    debug
    before_debug
    build_debug
    after_debug
    out_debug
    clean_debug

    release
    before_release
    build_release
    after_release
    out_release
    clean_release

    all
    before_build
    after_build
    clean

More Help: `man make`
