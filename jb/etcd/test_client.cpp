#include <jb/etcd/test.pb.h>
#include <jb/etcd/test.grpc.pb.h>
#include <jb/config_object.hpp>

#include <grpc++/grpc++.h>
#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[]) try {
  std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(
      "localhost:50050", grpc::InsecureChannelCredentials());
  auto stub = std::make_unique<test::Echo::Stub>(channel);

  test::EchoRequest request;
  request.set_value("blah blah");
  test::EchoResponse response;
  grpc::ClientContext context;
  auto status = stub->echo(&context, request, &response);
  if (status.ok()) {
    std::cout << "got " << response.value() << "\n";
  } else {
    std::cerr << "RPC failed: " << status.error_message() << "["
              << status.error_code() << "]" << std::endl;
  }
  return 0;
} catch (jb::usage const& u) {
  std::cerr << u.what() << std::endl;
  return u.exit_status();
} catch (std::exception const& ex) {
  std::cerr << "Standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "Unknown exception raised" << std::endl;
  return 1;
}
