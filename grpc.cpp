// Use grpc_cpp_plugin and protoc to generate code
// SimpleMath.proto:
// service SimpleMath { rpc TimesTwo (ReqType) returns (RespType); }
int main() {
    auto ch = grpc::CreateChannel("localhost:54321", grpc::InsecureChannelCredentials());
    auto stub = SimpleMath::NewStub(ch);
    ReqType req; req.set_num(42);
    grpc::ClientContext ctx; RespType resp;
    auto status = stub->TimesTwo(&ctx, req, &resp);
    std::cout << "Result: " << resp.num() << std::endl;
}
