// Copyright 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "generator/internal/stub_cc_generator.h"
#include "generator/internal/data_model.h"
#include "generator/internal/gapic_utils.h"
#include "generator/internal/printer.h"
#include <google/protobuf/descriptor.h>

namespace pb = google::protobuf;

namespace google {
namespace api {
namespace codegen {
namespace internal {

std::vector<std::string> BuildClientStubCCIncludes(
    pb::ServiceDescriptor const* service) {
  return {
      LocalInclude(
          absl::StrCat(CamelCaseToSnakeCase(service->name()), "_stub.gapic.h")),
      LocalInclude(absl::StrCat(
          absl::StripSuffix(service->file()->name(), ".proto"), ".grpc.pb.h")),
      LocalInclude("gax/call_context.h"), LocalInclude("gax/retry_loop.h"),
      LocalInclude("gax/status.h"), LocalInclude("grpcpp/client_context.h"),
      LocalInclude("grpcpp/channel.h"), LocalInclude("grpcpp/create_channel.h"),
      SystemInclude("chrono"), SystemInclude("thread")};
}

std::vector<std::string> BuildClientStubCCNamespaces(
    pb::ServiceDescriptor const* /* service */) {
  return {};
}

bool GenerateClientStubCC(pb::ServiceDescriptor const* service,
                          std::map<std::string, std::string> const& vars,
                          Printer& p, std::string* /* error */) {
  auto includes = BuildClientStubCCIncludes(service);
  auto namespaces = BuildClientStubCCNamespaces(service);

  p->Print(vars,
           "// Generated by the GAPIC C++ plugin.\n"
           "// If you make any local changes, they will be lost.\n"
           "// source: $proto_file_name$\n"
           "\n");

  for (auto const& include : includes) {
    p->Print("#include $include$\n", "include", include);
  }
  for (auto const& nspace : namespaces) {
    p->Print("namespace $namespace$ {\n", "namespace", nspace);
  }

  p->Print("\n");

  // Abstract stub method definitions
  DataModel::PrintMethods(
      service, vars, p,
      "google::gax::Status\n"
      "$stub_class_name$::$method_name$(\n"
      "  google::gax::CallContext&,\n"
      "  $request_object$ const&,\n"
      "  $response_object$*) {\n"
      "  return google::gax::Status(google::gax::StatusCode::kUnimplemented,\n"
      "    \"$method_name$ not implemented\");\n"
      "}\n"
      "\n",
      NoStreamingPredicate);

  p->Print(vars,
           "$stub_class_name$::~$stub_class_name$() {}"
           "\n"
           "\n");

  // gRPC aware stub class declaration and method definition
  p->Print(vars,
           "namespace {\n"
           "class Default$stub_class_name$ : public $stub_class_name$ {\n"
           " public:\n"
           "  Default$stub_class_name$(std::unique_ptr<$grpc_stub_fqn$::"
           "StubInterface> grpc_stub)\n"
           "    : grpc_stub_(std::move(grpc_stub)) {}\n"
           "\n"
           "  Default$stub_class_name$(Default$stub_class_name$ const&) = "
           "delete;\n"
           "  Default$stub_class_name$& operator=(Default$stub_class_name$ "
           "const&) = delete;\n"
           "\n");

  DataModel::PrintMethods(
      service, vars, p,
      "  google::gax::Status\n"
      "  $method_name$(google::gax::CallContext& context,\n"
      "    $request_object$ const& request,\n"
      "    $response_object$* response) override {\n"
      "    grpc::ClientContext grpc_ctx;\n"
      "    context.PrepareGrpcContext(&grpc_ctx);\n"
      "    return google::gax::GrpcStatusToGaxStatus("
      "grpc_stub_->$method_name$(&grpc_ctx, request, response));\n"
      "  }\n"
      "\n",
      NoStreamingPredicate);

  p->Print(vars,
           " private:\n"
           "  std::unique_ptr<$grpc_stub_fqn$::StubInterface> grpc_stub_;\n"
           "};  // Default$stub_class_name$\n"
           "\n");

  // Retrying stub that decorates another stub
  p->Print(vars,
           "class Retry$stub_class_name$ : public $stub_class_name$ {\n"
           " public:\n"
           "  Retry$stub_class_name$(std::unique_ptr<$stub_class_name$> stub,\n"
           "                          google::gax::RetryPolicy const& "
           "retry_policy,\n"
           "                          google::gax::BackoffPolicy const& "
           "backoff_policy) :\n"
           "            next_stub_(std::move(stub)),\n"
           "            default_retry_policy_(retry_policy.clone()),\n"
           "            default_backoff_policy_(backoff_policy.clone()) {}\n"
           "\n");

  DataModel::PrintMethods(
      service, vars, p,
      "  google::gax::Status\n"
      "  $method_name$(google::gax::CallContext& context,\n"
      "             $request_object$ const& request,\n"
      "             $response_object$* response) override {\n"
      "    auto invoke_stub = [this](google::gax::CallContext& c,\n"
      "                $request_object$ const& req,\n"
      "                $response_object$* resp) {\n"
      "              return this->next_stub_->$method_name$(c, req, resp);\n"
      "            };\n"
      "    return google::gax::MakeRetryCall<$request_object$,\n"
      "                                      $response_object$,\n"
      "                                      decltype(invoke_stub)>(\n"
      "        context, request, response, std::move(invoke_stub),\n"
      "        clone_retry(context), clone_backoff(context));\n"
      "  }\n"
      "\n",
      NoStreamingPredicate);

  p->Print(
      vars,
      " private:\n"
      "  std::unique_ptr<google::gax::RetryPolicy>\n"
      "  clone_retry(google::gax::CallContext const &context) const {\n"
      "    auto context_retry = context.RetryPolicy();\n"
      "    return context_retry ? std::move(context_retry)\n"
      "                         : std::move(default_retry_policy_->clone());\n"
      "  }\n"
      "\n"
      "  std::unique_ptr<google::gax::BackoffPolicy>\n"
      "  clone_backoff(google::gax::CallContext const &context) const {\n"
      "    auto context_backoff = context.BackoffPolicy();\n"
      "    return context_backoff ? std::move(context_backoff)\n"
      "                           : "
      "std::move(default_backoff_policy_->clone());\n"
      "  }\n"
      "\n"
      "  std::unique_ptr<$stub_class_name$> next_stub_;\n"
      "  const std::unique_ptr<google::gax::RetryPolicy const> "
      "default_retry_policy_;\n"
      "  const std::unique_ptr<google::gax::BackoffPolicy const>  "
      "default_backoff_policy_;\n"
      "};  // Retry$stub_class_name$\n");

  p->Print(vars,
           "}  // namespace\n"
           "\n"
           "std::unique_ptr<$stub_class_name$> Create$stub_class_name$() {\n"
           "  auto credentials = grpc::GoogleDefaultCredentials();\n"
           "  return Create$stub_class_name$(std::move(credentials));\n"
           "}\n"
           "\n"
           "std::unique_ptr<$stub_class_name$>\n"
           "Create$stub_class_name$(std::shared_ptr<grpc::ChannelCredentials> "
           "creds) {\n"
           "  auto channel = grpc::CreateChannel(\"$service_endpoint$\",\n"
           "    std::move(creds));\n"
           "  auto grpc_stub = $grpc_stub_fqn$::NewStub(std::move(channel));\n"
           "  auto default_stub = std::unique_ptr<$stub_class_name$>(new\n"
           "    Default$stub_class_name$(std::move(grpc_stub)));\n"
           "  using ms = std::chrono::milliseconds;\n"
           "  // Note: these retry and backoff times are dummy stand ins.\n"
           "  // More appopriate default values will be chosen later.\n"
           "  google::gax::LimitedDurationRetryPolicy<> retry_policy(ms(500), "
           "ms(500));\n"
           "  google::gax::ExponentialBackoffPolicy backoff_policy(ms(20), "
           "ms(100));\n"
           "  return std::unique_ptr<$stub_class_name$>(new "
           "Retry$stub_class_name$(\n"
           "                       std::move(default_stub),\n"
           "                       retry_policy,\n"
           "                       backoff_policy));\n"
           "}\n"
           "\n");

  for (auto const& nspace : namespaces) {
    p->Print("}  // namespace $namespace$\n", "namespace", nspace);
  }

  return true;
}

}  // namespace internal
}  // namespace codegen
}  // namespace api
}  // namespace google
