## Building the libwebrtc builder docker image

This image gets as far as producing `libwebrtc-full.a` so that it can be used for application builds.

`docker build -t libwebrtc-builder:m132 -f Dockerfile-Builder --progress=plain .`

## Building docker image

The application image. It builds the application on an instance of the builder image and then copies the binary to a new ubuntu image and installs the required shared library packages.

`docker build -t libwebrtc-webrtc-echo:m132 --progress=plain .`

## Running docker image

`docker run -it --init --rm -p 8080:8080 libwebrtc-webrtc-echo:0.9`

## Generate Ninja (GN) Reference

The options supplied to the gn command are critical for buiding a working webrtc.lib (and equivalent object files on linux) as well as ensuring all the required symbols are included.

gn --help # Get all command line args for gn.
gn args out/Default --list > gnargs.txt # See what options are available for controlling the libwebrtc build.

https://gn.googlesource.com/gn/+/main/docs/reference.md

## Building webrtc.lib on Windows

Follow the standard instructions at https://webrtc.github.io/webrtc-org/native-code/development/ and then use the steps below. Pay particular attention to the `--args` argument supplied to the `gn` command.

It seems the latest version of the build instructions are available directly in the source tree at https://webrtc.googlesource.com/src/+/refs/heads/main/docs/native-code/development/.

````
src> git checkout branch-heads/6834 -b m132 # See https://chromiumdash.appspot.com/branches for remote branch info.
src> set PATH=C:\Tools\depot_tools;%PATH%
src> gclient sync
src> SET DEPOT_TOOLS_WIN_TOOLCHAIN=0 # Use installed clang toolchain instead of devtools one.
src> rmdir /q /s out\Default
src> gn gen out/Default --args="is_component_build=false" # See https://gn.googlesource.com/gn/+/main/docs/reference.md#IDE-options for more info.
src> gn args out/Default --list --short --overrides-only
src> gn clean out/Default # If previous compilation.
src> autoninja -C out/Default # DON'T use "ninja all -C out/Default", it attempts to build everything in the out/Default directory rather than just the libwebrtc bits.
````

## Building libwebrtc-full.a on Ubuntu

Follow the standard instructions at https://webrtc.github.io/webrtc-org/native-code/development/ and then use the steps below. Pay particular attention to the `--args` argument supplied to the `gn` command.

It seems the latest version of the build instructions are available directly in the source tree at https://webrtc.googlesource.com/src/+/refs/heads/main/docs/native-code/development/.

To update to the latest source or build a specific branch (see https://chromiumdash.appspot.com/branches for Chromium branch names):

````
git pull origin master
````
or
````
git checkout branch-heads/4430 -b m90
````

And then to build:

First time:

````
src$ gclient sync
src$ build/install-build-deps.sh
````

Subsequently:

````
src$ gclient sync
````

or if there are problems with `gclient sync`:

````
src$ gclient sync --force 
````

And then:

````
src$ gn gen out/Default --args="is_component_build=false use_custom_libcxx=false use_custom_libcxx_for_host=false rtc_enable_protobuf=false"
src$ gn args out/Default --list --short --overrides-only
src$ ninja -C out/Default
src$ out/Default/webrtc_lib_link_test
src$ cd out/Default
src$ ar crs out/Default/libwebrtc-full.a $(find out/Default/obj -name '*\.o')
sudo apt install software-properties-common
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt update
sudo apt install libstdc++-9-dev libevent-dev
````

## Support Group

The main place to get supprot for building libwebrtc is the Google Group at https://groups.google.com/u/2/g/discuss-webrtc.
