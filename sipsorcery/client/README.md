**Description**

A .NET console application that acts as a peer for a WebRTC Echo Server.

**Prerequisites**

The .NET 8 SDK needs to be installed as per https://dotnet.microsoft.com/download/dotnet/8.0.

Note the full SDK install is required, and not the runtime-only option, so as the console application can be built from source.

**Usage**

By default the in built client will attempt to POST its SDP offer to an echo server at `http://localhost:8080/`.

 - Make sure the echo test server is running.
 - `dotnet run`
