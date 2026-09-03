# protocols-cpp

![CI](https://github.com/cear12/protocols-cpp/actions/workflows/ci.yml/badge.svg)

Small, focused reference implementations of common network-protocol
clients/servers in modern C++20: raw TCP, WebSocket (client + server),
FTP, SMTP, gRPC, WebRTC data channels, and a browser Native Messaging
host. Each file is a minimal, readable example of *that specific
protocol's* C++ integration pattern rather than a production framework.

## Contents

- [Layout](#layout)
- [What builds where](#what-builds-where)
- [Building](#building)
- [Testing](#testing)
- [Notes per protocol](#notes-per-protocol)

## Layout

| File | Protocol | External dependency |
|---|---|---|
| `include/protocolscpp/tcp_echo_server.h`, `src/tcp_echo_server.cpp` | Raw TCP | none (POSIX sockets only) |
| `web_socket_client.cpp` | WebSocket (client) | none -- vendored (`third_party/easywsclient/`, MIT) |
| `web_socket_server.cpp` | WebSocket (server) | websocketpp + Boost.Asio |
| `ftp_client.cpp` | FTP | libcurl |
| `smtp_email_client.cpp` | SMTP | libcurl |
| `native_messaging_host.cpp` | Chrome/Firefox Native Messaging | jsoncpp |
| `grpc_client_demo.cpp` + `proto/simple_math.proto` | gRPC | Protobuf + gRPC (codegen) |
| `web_rtc_data_channel.cpp` | WebRTC data channel | libdatachannel (fetched via CMake) |

## What builds where

Two things in this repo have **zero un-vendored dependencies** and are
built + unit-tested in every environment, including this repo's own CI:
`protocolscpp_core` (the TCP echo server) and `web_socket_client`
(against the vendored easywsclient). Both were compiled and tested
locally while writing this repo.

Everything else wraps a real third-party protocol library that isn't
vendored (curl, jsoncpp, websocketpp, gRPC, libdatachannel) --
`CMakeLists.txt` uses `find_package(...)` for each one and only adds that
target if the dependency is actually found, so `cmake` configure never
hard-fails on a machine that's missing one of them. **These targets were
written and reviewed for correctness but not compiled in the environment
that produced this repo** (no root/package-manager access there to
install the system libraries) -- CI (below) installs the real libraries
on a Linux runner and builds every target for real on every push.

## Building

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

To build everything, including the optional dependency-heavy targets, on
Ubuntu:

```bash
sudo apt-get install libcurl4-openssl-dev libjsoncpp-dev libwebsocketpp-dev libboost-dev \
                      libgrpc++-dev protobuf-compiler-grpc
cmake -B build -DPROTOCOLSCPP_ENABLE_GRPC=ON -DPROTOCOLSCPP_ENABLE_WEBRTC=ON
cmake --build build
```

`PROTOCOLSCPP_ENABLE_WEBRTC` fetches libdatachannel via CMake's
`FetchContent` (it isn't in standard apt repos); expect the first
configure to take a while.

## Building in Visual Studio

With several independent demo/test executables (some only appearing when
their optional dependency is found) and no CMakePresets.json, Visual
Studio's Open Folder / CMake integration has no default startup item
configured. Pressing **Debug/Run** (not Build) then pops a blocking
"Select Startup Item" dialog -- easy to mistake for the project failing
to build, even though **Build > Build All** (Ctrl+Shift+B) succeeds
regardless of what's selected there. `CMakePresets.json` sets
`CMAKE_VS_STARTUP_PROJECT` to `tcp_echo_server_demo`, the one target with
no optional-dependency guard around it, so it's always present and
Debug/Run works immediately; pick a different target from the dropdown
next to the Run button to debug any of the others that happen to be
available.

## Testing

`protocolscpp_core`'s `TcpEchoServer` is the one class in this repo with
enough self-contained logic to unit-test meaningfully without a real
external service, so it's the one that has a Catch2 suite: binds to an
OS-assigned port, starts/stops cleanly and idempotently, and echoes
multiple sequential real (loopback) connections correctly. The other
files are thin, mostly line-for-line bindings onto a third-party
library's own client/server API -- the value in testing them lies in
testing *that library*, not this code, so they're demonstrated via their
`PROTOCOLSCPP_BUILD_STANDALONE_DEMO` `main()` instead of duplicated with
hand-written tests.

## Notes per protocol

- **gRPC**: `proto/simple_math.proto` defines the `SimpleMath` service
  `grpc_client_demo.cpp` calls; `protobuf_generate_cpp`/`grpc_generate_cpp`
  in `CMakeLists.txt` generate the corresponding headers at build time.
  The demo is a client only -- point it at a running `SimpleMath` server
  on `localhost:54321` (or adapt the address).
- **Native Messaging**: also needs a `manifest.json` and a browser-side
  registration (registry key on Windows, a manifest directory on
  Linux/macOS) pointing at the built binary; that's extension-packaging
  plumbing outside this file's scope.
- **WebRTC**: `createOffer()` returns a local SDP offer that still needs
  to reach a remote peer through some signaling channel (a WebSocket
  server, for instance) -- signaling is intentionally left to the
  application.
