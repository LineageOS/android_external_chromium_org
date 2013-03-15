// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/tests/test_host_resolver_private.h"

#include "ppapi/c/private/ppb_net_address_private.h"
#include "ppapi/cpp/module_impl.h"
#include "ppapi/cpp/private/host_resolver_private.h"
#include "ppapi/cpp/private/tcp_socket_private.h"
#include "ppapi/cpp/var.h"
#include "ppapi/tests/test_utils.h"
#include "ppapi/tests/testing_instance.h"

REGISTER_TEST_CASE(HostResolverPrivate);

TestHostResolverPrivate::TestHostResolverPrivate(TestingInstance* instance)
    : TestCase(instance) {
}

bool TestHostResolverPrivate::Init() {
  bool host_resolver_private_is_available =
      pp::HostResolverPrivate::IsAvailable();
  if (!host_resolver_private_is_available)
    instance_->AppendError("PPB_HostResolver_Private interface not available");

  bool tcp_socket_private_is_available = pp::TCPSocketPrivate::IsAvailable();
  if (!tcp_socket_private_is_available)
    instance_->AppendError("PPB_TCPSocket_Private interface not available");

  bool init_host_port =
      GetLocalHostPort(instance_->pp_instance(), &host_, &port_);
  if (!init_host_port)
    instance_->AppendError("Can't init host and port");

  return host_resolver_private_is_available &&
      tcp_socket_private_is_available &&
      init_host_port &&
      CheckTestingInterface() &&
      EnsureRunningOverHTTP();
}

void TestHostResolverPrivate::RunTests(const std::string& filter) {
  RUN_TEST(Empty, filter);
  RUN_TEST_FORCEASYNC_AND_NOT(Resolve, filter);
  RUN_TEST_FORCEASYNC_AND_NOT(ResolveIPv4, filter);
}

std::string TestHostResolverPrivate::SyncConnect(pp::TCPSocketPrivate* socket,
                                                 const std::string& host,
                                                 uint16_t port) {
  TestCompletionCallback callback(instance_->pp_instance(), force_async_);
  int32_t rv = socket->Connect(host.c_str(), port, callback.GetCallback());
  if (force_async_ && rv != PP_OK_COMPLETIONPENDING)
    return ReportError("PPB_TCPSocket_Private::Connect force_async", rv);
  if (rv == PP_OK_COMPLETIONPENDING)
    rv = callback.WaitForResult();
  ASSERT_EQ(PP_OK, rv);
  PASS();
}

std::string TestHostResolverPrivate::SyncConnect(
    pp::TCPSocketPrivate* socket,
    const PP_NetAddress_Private& address) {
  TestCompletionCallback callback(instance_->pp_instance(), force_async_);
  int32_t rv = socket->ConnectWithNetAddress(&address, callback.GetCallback());
  if (force_async_ && rv != PP_OK_COMPLETIONPENDING)
    return ReportError("PPB_TCPSocket_Private::Connect force_async", rv);
  if (rv == PP_OK_COMPLETIONPENDING)
    rv = callback.WaitForResult();
  ASSERT_EQ(PP_OK, rv);
  PASS();
}

std::string TestHostResolverPrivate::SyncRead(pp::TCPSocketPrivate* socket,
                                              char* buffer,
                                              int32_t num_bytes,
                                              int32_t* bytes_read) {
  TestCompletionCallback callback(instance_->pp_instance(), force_async_);
  int32_t rv = socket->Read(buffer, num_bytes, callback.GetCallback());
  if (force_async_ && rv != PP_OK_COMPLETIONPENDING)
    return ReportError("PPB_TCPSocket_Private::Read force_async", rv);
  if (rv == PP_OK_COMPLETIONPENDING)
    rv = callback.WaitForResult();
  if (num_bytes != rv)
    return ReportError("PPB_TCPSocket_Private::Read", rv);
  *bytes_read = rv;
  PASS();
}

std::string TestHostResolverPrivate::SyncWrite(pp::TCPSocketPrivate* socket,
                                               const char* buffer,
                                               int32_t num_bytes,
                                               int32_t* bytes_written) {
  TestCompletionCallback callback(instance_->pp_instance(), force_async_);
  int32_t rv = socket->Write(buffer, num_bytes, callback.GetCallback());
  if (force_async_ && rv != PP_OK_COMPLETIONPENDING)
    return ReportError("PPB_TCPSocket_Private::Write force_async", rv);
  if (rv == PP_OK_COMPLETIONPENDING)
    rv = callback.WaitForResult();
  if (num_bytes != rv)
    return ReportError("PPB_TCPSocket_Private::Write", rv);
  *bytes_written = rv;
  PASS();
}

std::string TestHostResolverPrivate::CheckHTTPResponse(
    pp::TCPSocketPrivate* socket,
    const std::string& request,
    const std::string& response) {
  int32_t rv = 0;
  ASSERT_SUBTEST_SUCCESS(
      SyncWrite(socket, request.c_str(), request.size(), &rv));
  std::vector<char> response_buffer(response.size());
  ASSERT_SUBTEST_SUCCESS(
      SyncRead(socket, &response_buffer[0], response.size(), &rv));
  std::string actual_response(&response_buffer[0], rv);
  if (response != actual_response) {
    return "CheckHTTPResponse failed, expected: " + response +
        ", actual: " + actual_response;
  }
  PASS();
}

std::string TestHostResolverPrivate::SyncResolve(
    pp::HostResolverPrivate* host_resolver,
    const std::string& host,
    uint16_t port,
    const PP_HostResolver_Private_Hint& hint) {
  TestCompletionCallback callback(instance_->pp_instance(), force_async_);
  int32_t rv = host_resolver->Resolve(host, port, hint, callback.GetCallback());
  if (force_async_ && rv != PP_OK_COMPLETIONPENDING)
    return ReportError("PPB_HostResolver_Private::Resolve force_async", rv);
  if (rv == PP_OK_COMPLETIONPENDING)
    rv = callback.WaitForResult();
  if (rv != PP_OK)
    return ReportError("PPB_HostResolver_Private::Resolve", rv);
  PASS();
}

std::string TestHostResolverPrivate::TestEmpty() {
  pp::HostResolverPrivate host_resolver(instance_);
  ASSERT_EQ(0, host_resolver.GetSize());
  PP_NetAddress_Private address;
  ASSERT_FALSE(host_resolver.GetNetAddress(0, &address));

  PASS();
}

std::string TestHostResolverPrivate::ParametrizedTestResolve(
    const PP_HostResolver_Private_Hint &hint) {
  pp::HostResolverPrivate host_resolver(instance_);

  ASSERT_SUBTEST_SUCCESS(SyncResolve(&host_resolver, host_, port_, hint));

  const size_t size = host_resolver.GetSize();
  ASSERT_TRUE(size >= 1);

  PP_NetAddress_Private address;
  for (size_t i = 0; i < size; ++i) {
    ASSERT_TRUE(host_resolver.GetNetAddress(i, &address));

    pp::TCPSocketPrivate socket(instance_);
    ASSERT_SUBTEST_SUCCESS(SyncConnect(&socket, address));
    ASSERT_SUBTEST_SUCCESS(CheckHTTPResponse(&socket,
                                             "GET / HTTP/1.0\r\n\r\n",
                                             "HTTP"));
    socket.Disconnect();
  }

  ASSERT_FALSE(host_resolver.GetNetAddress(size, &address));
  pp::Var canonical_name = host_resolver.GetCanonicalName();
  ASSERT_TRUE(canonical_name.is_string());
  pp::TCPSocketPrivate socket(instance_);
  ASSERT_SUBTEST_SUCCESS(SyncConnect(&socket,
                                     canonical_name.AsString(),
                                     port_));
  ASSERT_SUBTEST_SUCCESS(CheckHTTPResponse(&socket,
                                           "GET / HTTP/1.0\r\n\r\n",
                                           "HTTP"));
  socket.Disconnect();

  PASS();
}

std::string TestHostResolverPrivate::TestResolve() {
  PP_HostResolver_Private_Hint hint;
  hint.family = PP_NETADDRESSFAMILY_UNSPECIFIED;
  hint.flags = PP_HOST_RESOLVER_FLAGS_CANONNAME;
  return ParametrizedTestResolve(hint);
}

std::string TestHostResolverPrivate::TestResolveIPv4() {
  PP_HostResolver_Private_Hint hint;
  hint.family = PP_NETADDRESSFAMILY_IPV4;
  hint.flags = PP_HOST_RESOLVER_FLAGS_CANONNAME;
  return ParametrizedTestResolve(hint);
}
