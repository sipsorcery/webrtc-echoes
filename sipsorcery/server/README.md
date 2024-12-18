**Description**

A .NET console application that runs a WebRTC Echo Server.

**Prerequisites**

The .NET 8 SDK needs to be installed as per https://dotnet.microsoft.com/download/dotnet/8.0.

Note the full SDK install is required, and not the runtime-only option, so as the console application can be built from source.

**Usage**

By default the in built web server will listen on `http://*:8080/`.

`dotnet run`

POST an SDP offer to `http://*:8080/offer` or open `http://localhost:8080/` in a browser to use the included `index.html` demo page.

**Additional Usage**

The listening URL can be adjusted with a command line argument:

`dotnet run -- http://*:8081`

Preset IP addresses can also be provided to insert static ICE candidates into the SDP answer generated by the Echo Server:

`dotnet run -- http://*:8080 20.73.112.13 2603:1020:203:3::6`
