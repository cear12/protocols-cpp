// Minimal unary gRPC client for the SimpleMath service defined in
// proto/simple_math.proto.
//
// Build requirement: gRPC + Protobuf, plus a corresponding SimpleMath
// server listening on localhost:54321 to actually call (this file is a
// client only). Ubuntu/Debian:
//   apt-get install libgrpc++-dev protobuf-compiler-grpc
// Wired into CMakeLists.txt behind PROTOCOLSCPP_ENABLE_GRPC (off by
// default, since it needs protoc codegen); not compiled as part of this
// environment's own verification pass -- see README.md.
#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>

#include "simple_math.grpc.pb.h"  // generated from proto/simple_math.proto

namespace protocolscpp {

class SimpleMathClient {
 public:
  explicit SimpleMathClient(const std::shared_ptr<grpc::Channel>& channel)
      : stub_(SimpleMath::NewStub(channel)) {}

  // Returns the server's answer, or throws std::runtime_error with the
  // gRPC status message on failure (connection refused, deadline
  // exceeded, ...).
  std::int64_t timesTwo(std::int64_t value) {
    NumberRequest request;
    request.set_num(value);

    NumberResponse response;
    grpc::ClientContext context;
    grpc::Status status = stub_->TimesTwo(&context, request, &response);

    if (!status.ok()) {
      throw std::runtime_error("SimpleMath.TimesTwo RPC failed: " +
                               status.error_message());
    }
    return response.num();
  }

 private:
  std::unique_ptr<SimpleMath::Stub> stub_;
};

}  // namespace protocolscpp

int main() {
  auto channel = grpc::CreateChannel("localhost:54321",
                                     grpc::InsecureChannelCredentials());
  protocolscpp::SimpleMathClient client(channel);

  try {
    std::cout << "TimesTwo(21) = " << client.timesTwo(21) << "\n";
  } catch (const std::exception& e) {
    std::cerr << "RPC failed (expected without a running SimpleMath server): "
              << e.what() << "\n";
    return 1;
  }
  return 0;
}
