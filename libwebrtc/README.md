## Building the libwebrtc builder docker image

This image gets as far as producing `libwebrtc-full.a` so that it can be used for application builds.

`docker build -t libwebrtc-builder:m132 -f Dockerfile-Builder --progress=plain .`

If the build fails:

1. Comment out the failing steps, generally that will be the ones from the `gn gen` command on.

2. `docker run -it --init --rm libwebrtc-builder:m132`

## Building echo application docker image

The application image. It builds the application on an instance of the builder image and then copies the binary to a new ubuntu image and installs the required shared library packages.

`docker build -t libwebrtc-webrtc-echo:m132 --progress=plain .`

If the build fails:

1. Comment out the failing steps, generally that will be the ones from the `cmake` command on.

2. `docker run -it --init --rm libwebrtc-webrtc-echo:m132`

## Running echo application docker image

`docker run -it --init --rm -p 8080:8080 libwebrtc-webrtc-echo:m132`

## Generate Ninja (GN) Reference

The options supplied to the gn command are critical for buiding a working webrtc.lib (and equivalent object files on linux) as well as ensuring all the required symbols are included.

gn --help # Get all command line args for gn.
gn args out/Default --list > gnargs.txt # See what options are available for controlling the libwebrtc build.

https://gn.googlesource.com/gn/+/main/docs/reference.md

## Building webrtc.lib on Windows

Follow the standard instructions at https://webrtc.github.io/webrtc-org/native-code/development/ and then use the steps below. Pay particular attention to the `--args` argument supplied to the `gn` command.

It seems the latest version of the build instructions are available directly in the source tree at https://webrtc.googlesource.com/src/+/refs/heads/main/docs/native-code/development/.

````
src> git checkout branch-heads/4430 -b m90 # See https://chromiumdash.appspot.com/branches for remote branch info.
src> set PATH=C:\Tools\depot_tools;%PATH%
src> glient sync
src> rmdir /q /s out\Default
src> gn gen out/Default --args="is_clang=false use_lld=false" --ide=vs # See https://gn.googlesource.com/gn/+/master/docs/reference.md#IDE-options for more info.
src> gn args out/Default --list # Check use_lld is false. is_clang always shows true but if the false option is not set then linker errors when using webrtc.lib.
src> gn args out/Default --list --short --overrides-only
src> gn clean out/Default # If previous compilation.
src> ninja -C out/Default
````

Update Dec 2024 for version m132.

NOTE: Only the clang build chain is now supported for the libwebrtc build. To use webrtc.lib with Visual Studio the clang build chain can be installed via the Visual Studio Installer and then selected as the option in the project settings.

Install the Google depot tools as per https://webrtc.googlesource.com/src/+/main/docs/native-code/development/prerequisite-sw/.

In the webrtc-checkout directory, e.g. c:\dev\webrtc-checkout:

````
c:\dev\webrtc-checkout\src> set PATH=C:\Tools\depot_tools;%PATH%
c:\dev\webrtc-checkout\src> git checkout branch-heads/6834 -b m132 # See https://chromiumdash.appspot.com/branches for remote branch info.
c:\dev\webrtc-checkout\src> gclient sync -D
c:\dev\webrtc-checkout\src> SET DEPOT_TOOLS_WIN_TOOLCHAIN=0 # Use Visual Studio installed clang toolchain instead of devtools one.
c:\dev\webrtc-checkout\src> rmdir /q /s out\Default
# Setting use_custom_libcxx=false is critical. If it is true, the build uses the libc++ standard library from the third-party source tree, 
# making it incompatible when linking with Visual Studio. By setting it to false, the MSVC standard library will be used (e.g msvcp140.dll), 
# ensuring that Visual Studio can properly resolve all the required symbols.
c:\dev\webrtc-checkout\src> gn gen out/Default --args="is_debug=false rtc_include_tests=false treat_warnings_as_errors=false use_custom_libcxx=false use_rtti=true"
c:\dev\webrtc-checkout\src> autoninja -C out/Default # DON'T use "ninja all -C out/Default", as listed on the build instructions site, it attempts to build everything in the out/Default directory rather than just libwebrtc.
# The resultant webrtc.lib should be in out/Default/obj.
# Note as an added bonus vcpkg installed binaries that are build the the standard msbuild toolcahin, e.g. cl and ld, are compatible with the clang and lld-ld linker.
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
src$ gn gen out/Default --args="use_custom_libcxx=false"
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

The main place to get support for building libwebrtc is the Google Group at https://groups.google.com/u/2/g/discuss-webrtc.
