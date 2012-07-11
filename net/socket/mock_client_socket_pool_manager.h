// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_MOCK_CLIENT_SOCKET_POOL_MANAGER_H_
#define NET_SOCKET_MOCK_CLIENT_SOCKET_POOL_MANAGER_H_

#include "base/basictypes.h"
#include "net/socket/client_socket_pool_manager.h"
#include "net/socket/client_socket_pool_manager_impl.h"

namespace net {

class MockClientSocketPoolManager : public ClientSocketPoolManager {
 public:
  MockClientSocketPoolManager();
  virtual ~MockClientSocketPoolManager();

  // Sets "override" socket pools that get used instead.
  void SetTransportSocketPool(TransportClientSocketPool* pool);
  void SetSSLSocketPool(SSLClientSocketPool* pool);
  void SetSocketPoolForSOCKSProxy(const HostPortPair& socks_proxy,
                                  SOCKSClientSocketPool* pool);
  void SetSocketPoolForHTTPProxy(const HostPortPair& http_proxy,
                                 HttpProxyClientSocketPool* pool);
  void SetSocketPoolForSSLWithProxy(const HostPortPair& proxy_server,
                                    SSLClientSocketPool* pool);

  // ClientSocketPoolManager methods:
  virtual void FlushSocketPools() OVERRIDE;
  virtual void CloseIdleSockets() OVERRIDE;
  virtual TransportClientSocketPool* GetTransportSocketPool() OVERRIDE;
  virtual SSLClientSocketPool* GetSSLSocketPool() OVERRIDE;
  virtual SOCKSClientSocketPool* GetSocketPoolForSOCKSProxy(
      const HostPortPair& socks_proxy) OVERRIDE;
  virtual HttpProxyClientSocketPool* GetSocketPoolForHTTPProxy(
      const HostPortPair& http_proxy) OVERRIDE;
  virtual SSLClientSocketPool* GetSocketPoolForSSLWithProxy(
      const HostPortPair& proxy_server) OVERRIDE;
  virtual base::Value* SocketPoolInfoToValue() const OVERRIDE;

 private:
  typedef internal::OwnedPoolMap<HostPortPair, TransportClientSocketPool*>
      TransportSocketPoolMap;
  typedef internal::OwnedPoolMap<HostPortPair, SOCKSClientSocketPool*>
      SOCKSSocketPoolMap;
  typedef internal::OwnedPoolMap<HostPortPair, HttpProxyClientSocketPool*>
      HTTPProxySocketPoolMap;
  typedef internal::OwnedPoolMap<HostPortPair, SSLClientSocketPool*>
      SSLSocketPoolMap;

  scoped_ptr<TransportClientSocketPool> transport_socket_pool_;
  scoped_ptr<SSLClientSocketPool> ssl_socket_pool_;
  SOCKSSocketPoolMap socks_socket_pools_;
  HTTPProxySocketPoolMap http_proxy_socket_pools_;
  SSLSocketPoolMap ssl_socket_pools_for_proxies_;

  DISALLOW_COPY_AND_ASSIGN(MockClientSocketPoolManager);
};

}  // namespace net

#endif  // NET_SOCKET_MOCK_CLIENT_SOCKET_POOL_MANAGER_H_
