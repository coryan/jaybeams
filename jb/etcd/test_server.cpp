#include <jb/etcd/test.pb.h>
#include <jb/etcd/test.grpc.pb.h>
#include <jb/config_object.hpp>

#include <grpc++/grpc++.h>

#include <iostream>
#include <stdexcept>

class Echo_impl final : public test::Echo::Service {
  grpc::Status echo(
      grpc::ServerContext* context, test::EchoRequest const* request,
      test::EchoResponse* response) override {
    response->set_value(request->value());
    return grpc::Status::OK;
  }
};

int main(int argc, char* argv[]) try {
  Echo_impl service;
  grpc::ServerBuilder builder;
  builder.AddListeningPort("0.0.0.0:50050", grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
  std::cout << "Server running on 0.0.0.0:50050" << std::endl;
  server->Wait();

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
