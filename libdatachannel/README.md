**Description**

C++17 WebRTC echo client and server with libdatachannel.

**Prerequisites**

You need cmake and the development libraries with header files for either OpenSSL or GnuTLS. On Debian/Ubuntu, you can install them with either `$ apt install libssl-dev` or `$ apt install libgnutls28-dev`.

Additionally, be sure the submodules are updated with `git submodule update --init --recursive`.

**Build**

`$ cmake -B build` for OpenSSL or `$ cmake -B build -DUSE_GNUTLS=1` for GnuTLS

`$ (cd build; make -j4)`

**Usage**

Server: `$ build/server [PORT]`

Client: `$ build/client [URL]`

The server listens on port 8080 by default and the client uses the URL http://127.0.0.1:8080/offer by default.

