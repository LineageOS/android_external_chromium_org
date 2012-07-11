// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HTTP_AUTH_HANDLER_NEGOTIATE_H_
#define NET_HTTP_HTTP_AUTH_HANDLER_NEGOTIATE_H_

#include <string>

#include "build/build_config.h"
#include "net/base/address_list.h"
#include "net/base/net_export.h"
#include "net/http/http_auth_handler.h"
#include "net/http/http_auth_handler_factory.h"

#if defined(OS_WIN)
#include "net/http/http_auth_sspi_win.h"
#elif defined(OS_POSIX)
#include "net/http/http_auth_gssapi_posix.h"
#endif

namespace net {

class HostResolver;
class SingleRequestHostResolver;
class URLSecurityManager;

// Handler for WWW-Authenticate: Negotiate protocol.
//
// See http://tools.ietf.org/html/rfc4178 and http://tools.ietf.org/html/rfc4559
// for more information about the protocol.

class NET_EXPORT_PRIVATE HttpAuthHandlerNegotiate : public HttpAuthHandler {
 public:
#if defined(OS_WIN)
  typedef SSPILibrary AuthLibrary;
  typedef HttpAuthSSPI AuthSystem;
#elif defined(OS_POSIX)
  typedef GSSAPILibrary AuthLibrary;
  typedef HttpAuthGSSAPI AuthSystem;
#endif

  class NET_EXPORT_PRIVATE Factory : public HttpAuthHandlerFactory {
   public:
    Factory();
    virtual ~Factory();

    // |disable_cname_lookup()| and |set_disable_cname_lookup()| get/set whether
    // the auth handlers generated by this factory should skip looking up the
    // canonical DNS name of the the host that they are authenticating to when
    // generating the SPN. The default value is false.
    bool disable_cname_lookup() const { return disable_cname_lookup_; }
    void set_disable_cname_lookup(bool disable_cname_lookup) {
      disable_cname_lookup_ = disable_cname_lookup;
    }

    // |use_port()| and |set_use_port()| get/set whether the auth handlers
    // generated by this factory should include the port number of the server
    // they are authenticating to when constructing a Kerberos SPN. The default
    // value is false.
    bool use_port() const { return use_port_; }
    void set_use_port(bool use_port) { use_port_ = use_port; }

    void set_host_resolver(HostResolver* host_resolver);

    // Sets the system library to use, thereby assuming ownership of
    // |auth_library|.
    void set_library(AuthLibrary* auth_library) {
      auth_library_.reset(auth_library);
    }

    virtual int CreateAuthHandler(
        HttpAuth::ChallengeTokenizer* challenge,
        HttpAuth::Target target,
        const GURL& origin,
        CreateReason reason,
        int digest_nonce_count,
        const BoundNetLog& net_log,
        scoped_ptr<HttpAuthHandler>* handler) OVERRIDE;

   private:
    bool disable_cname_lookup_;
    bool use_port_;
    HostResolver* resolver_;
#if defined(OS_WIN)
    ULONG max_token_length_;
    bool first_creation_;
#endif
    bool is_unsupported_;
    scoped_ptr<AuthLibrary> auth_library_;
  };

  HttpAuthHandlerNegotiate(AuthLibrary* sspi_library,
#if defined(OS_WIN)
                           ULONG max_token_length,
#endif
                           URLSecurityManager* url_security_manager,
                           HostResolver* host_resolver,
                           bool disable_cname_lookup,
                           bool use_port);

  virtual ~HttpAuthHandlerNegotiate();

  // These are public for unit tests
  std::wstring CreateSPN(const AddressList& address_list, const GURL& orign);
  const std::wstring& spn() const { return spn_; }

  // HttpAuthHandler:
  virtual HttpAuth::AuthorizationResult HandleAnotherChallenge(
      HttpAuth::ChallengeTokenizer* challenge) OVERRIDE;
  virtual bool NeedsIdentity() OVERRIDE;
  virtual bool AllowsDefaultCredentials() OVERRIDE;
  virtual bool AllowsExplicitCredentials() OVERRIDE;

 protected:
  virtual bool Init(HttpAuth::ChallengeTokenizer* challenge) OVERRIDE;

  virtual int GenerateAuthTokenImpl(const AuthCredentials* credentials,
                                    const HttpRequestInfo* request,
                                    const CompletionCallback& callback,
                                    std::string* auth_token) OVERRIDE;

 private:
  enum State {
    STATE_RESOLVE_CANONICAL_NAME,
    STATE_RESOLVE_CANONICAL_NAME_COMPLETE,
    STATE_GENERATE_AUTH_TOKEN,
    STATE_GENERATE_AUTH_TOKEN_COMPLETE,
    STATE_NONE,
  };

  void OnIOComplete(int result);
  void DoCallback(int result);
  int DoLoop(int result);

  int DoResolveCanonicalName();
  int DoResolveCanonicalNameComplete(int rv);
  int DoGenerateAuthToken();
  int DoGenerateAuthTokenComplete(int rv);
  bool CanDelegate() const;

  AuthSystem auth_system_;
  bool disable_cname_lookup_;
  bool use_port_;
  HostResolver* const resolver_;

  // Members which are needed for DNS lookup + SPN.
  AddressList address_list_;
  scoped_ptr<SingleRequestHostResolver> single_resolve_;

  // Things which should be consistent after first call to GenerateAuthToken.
  bool already_called_;
  bool has_credentials_;
  AuthCredentials credentials_;
  std::wstring spn_;

  // Things which vary each round.
  CompletionCallback callback_;
  std::string* auth_token_;

  State next_state_;

  const URLSecurityManager* url_security_manager_;
};

}  // namespace net

#endif  // NET_HTTP_HTTP_AUTH_HANDLER_NEGOTIATE_H_
