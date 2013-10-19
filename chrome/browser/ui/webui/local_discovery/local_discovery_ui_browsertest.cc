// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/basictypes.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "chrome/browser/local_discovery/test_service_discovery_client.h"
#include "chrome/browser/signin/profile_oauth2_token_service.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager.h"
#include "chrome/browser/signin/signin_manager_base.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/ui/webui/local_discovery/local_discovery_ui_handler.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/ui_test_utils.cc"
#include "chrome/test/base/web_ui_browsertest.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"

using testing::InvokeWithoutArgs;
using testing::Return;
using testing::AtLeast;
using testing::DoDefault;
using testing::DoAll;
using testing::InSequence;
using testing::StrictMock;
using testing::AnyNumber;

using testing::InvokeWithoutArgs;
using testing::Return;
using testing::AtLeast;

namespace local_discovery {

namespace {

const uint8 kQueryData[] = {
  // Header
  0x00, 0x00,
  0x00, 0x00,               // Flags not set.
  0x00, 0x01,               // Set QDCOUNT (question count) to 1, all the
  // rest are 0 for a query.
  0x00, 0x00,
  0x00, 0x00,
  0x00, 0x00,

  // Question
  0x07, '_', 'p', 'r', 'i', 'v', 'e', 't',
  0x04, '_', 't', 'c', 'p',
  0x05, 'l', 'o', 'c', 'a', 'l',
  0x00,

  0x00, 0x0c,               // QTYPE: A query.
  0x00, 0x01,               // QCLASS: IN class. Unicast bit not set.
};

const uint8 kAnnouncePacket[] = {
  // Header
  0x00, 0x00,               // ID is zeroed out
  0x80, 0x00,               // Standard query response, no error
  0x00, 0x00,               // No questions (for simplicity)
  0x00, 0x05,               // 5 RR (answers)
  0x00, 0x00,               // 0 authority RRs
  0x00, 0x00,               // 0 additional RRs

  0x07, '_', 'p', 'r', 'i', 'v', 'e', 't',
  0x04, '_', 't', 'c', 'p',
  0x05, 'l', 'o', 'c', 'a', 'l',
  0x00,
  0x00, 0x0c,        // TYPE is PTR.
  0x00, 0x01,        // CLASS is IN.
  0x00, 0x00,        // TTL (4 bytes) is 32768 second.
  0x10, 0x00,
  0x00, 0x0c,        // RDLENGTH is 12 bytes.
  0x09, 'm', 'y', 'S', 'e', 'r', 'v', 'i', 'c', 'e',
  0xc0, 0x0c,

  0x09, 'm', 'y', 'S', 'e', 'r', 'v', 'i', 'c', 'e',
  0xc0, 0x0c,
  0x00, 0x10,        // TYPE is TXT.
  0x00, 0x01,        // CLASS is IN.
  0x00, 0x00,        // TTL (4 bytes) is 32768 seconds.
  0x01, 0x00,
  0x00, 0x34,        // RDLENGTH is 69 bytes.
  0x03, 'i', 'd', '=',
  0x10, 't', 'y', '=', 'S', 'a', 'm', 'p', 'l', 'e', ' ',
        'd', 'e', 'v', 'i', 'c', 'e',
  0x1e, 'n', 'o', 't', 'e', '=',
        'S', 'a', 'm', 'p', 'l', 'e', ' ', 'd', 'e', 'v', 'i', 'c', 'e', ' ',
        'd', 'e', 's', 'c', 'r', 'i', 'p', 't', 'i', 'o', 'n',

  0x09, 'm', 'y', 'S', 'e', 'r', 'v', 'i', 'c', 'e',
  0xc0, 0x0c,
  0x00, 0x21,        // Type is SRV
  0x00, 0x01,        // CLASS is IN
  0x00, 0x00,        // TTL (4 bytes) is 32768 second.
  0x10, 0x00,
  0x00, 0x17,        // RDLENGTH is 23
  0x00, 0x00,
  0x00, 0x00,
  0x22, 0xb8,        // port 8888
  0x09, 'm', 'y', 'S', 'e', 'r', 'v', 'i', 'c', 'e',
  0x05, 'l', 'o', 'c', 'a', 'l',
  0x00,

  0x09, 'm', 'y', 'S', 'e', 'r', 'v', 'i', 'c', 'e',
  0x05, 'l', 'o', 'c', 'a', 'l',
  0x00,
  0x00, 0x01,        // Type is A
  0x00, 0x01,        // CLASS is IN
  0x00, 0x00,        // TTL (4 bytes) is 32768 second.
  0x10, 0x00,
  0x00, 0x04,        // RDLENGTH is 4
  0x01, 0x02, 0x03, 0x04,  // 1.2.3.4

  0x09, 'm', 'y', 'S', 'e', 'r', 'v', 'i', 'c', 'e',
  0x05, 'l', 'o', 'c', 'a', 'l',
  0x00,
  0x00, 0x1C,        // Type is AAAA
  0x00, 0x01,        // CLASS is IN
  0x00, 0x00,        // TTL (4 bytes) is 32768 second.
  0x10, 0x00,
  0x00, 0x10,        // RDLENGTH is 16
  0x01, 0x02, 0x03, 0x04,  // 1.2.3.4
  0x01, 0x02, 0x03, 0x04,
  0x01, 0x02, 0x03, 0x04,
  0x01, 0x02, 0x03, 0x04,
};


const uint8 kGoodbyePacket[] = {
  // Header
  0x00, 0x00,               // ID is zeroed out
  0x80, 0x00,               // Standard query response, RA, no error
  0x00, 0x00,               // No questions (for simplicity)
  0x00, 0x01,               // 1 RR (answers)
  0x00, 0x00,               // 0 authority RRs
  0x00, 0x00,               // 0 additional RRs

  0x07, '_', 'p', 'r', 'i', 'v', 'e', 't',
  0x04, '_', 't', 'c', 'p',
  0x05, 'l', 'o', 'c', 'a', 'l',
  0x00,
  0x00, 0x0c,        // TYPE is PTR.
  0x00, 0x01,        // CLASS is IN.
  0x00, 0x00,        // TTL (4 bytes) is 0 seconds.
  0x00, 0x00,
  0x00, 0x0c,        // RDLENGTH is 12 bytes.
  0x09, 'm', 'y', 'S', 'e', 'r', 'v', 'i', 'c', 'e',
  0xc0, 0x0c,
};

const uint8 kAnnouncePacketRegistered[] = {
  // Header
  0x00, 0x00,               // ID is zeroed out
  0x80, 0x00,               // Standard query response, RA, no error
  0x00, 0x00,               // No questions (for simplicity)
  0x00, 0x01,               // 1 RR (answers)
  0x00, 0x00,               // 0 authority RRs
  0x00, 0x00,               // 0 additional RRs

  0x09, 'm', 'y', 'S', 'e', 'r', 'v', 'i', 'c', 'e',
  0x07, '_', 'p', 'r', 'i', 'v', 'e', 't',
  0x04, '_', 't', 'c', 'p',
  0x05, 'l', 'o', 'c', 'a', 'l',
  0x00,
  0x00, 0x10,        // TYPE is TXT.
  0x00, 0x01,        // CLASS is IN.
  0x00, 0x00,        // TTL (4 bytes) is 32768 seconds.
  0x01, 0x00,
  0x00, 0x3b,        // RDLENGTH is 76 bytes.
  0x0a, 'i', 'd', '=', 's', 'o', 'm', 'e', '_', 'i', 'd',
  0x10, 't', 'y', '=', 'S', 'a', 'm', 'p', 'l', 'e', ' ',
        'd', 'e', 'v', 'i', 'c', 'e',
  0x1e, 'n', 'o', 't', 'e', '=',
        'S', 'a', 'm', 'p', 'l', 'e', ' ', 'd', 'e', 'v', 'i', 'c', 'e', ' ',
        'd', 'e', 's', 'c', 'r', 'i', 'p', 't', 'i', 'o', 'n',
};

const char kResponseInfo[] = "{"
    "     \"x-privet-token\" : \"MyPrivetToken\""
    "}";

const char kResponseRegisterStart[] = "{"
    "     \"action\": \"start\","
    "     \"user\": \"user@host.com\""
    "}";

const char kResponseRegisterClaimTokenNoConfirm[] = "{"
    "    \"action\": \"getClaimToken\","
    "    \"user\": \"user@host.com\","
    "    \"error\": \"pending_user_action\","
    "    \"timeout\": 1"
    "}";

const char kResponseRegisterClaimTokenConfirm[] = "{"
    "    \"action\": \"getClaimToken\","
    "    \"user\": \"user@host.com\","
    "    \"token\": \"MySampleToken\","
    "    \"claim_url\": \"http://someurl.com/\""
    "}";

const char kResponseCloudPrintConfirm[] = "{ \"success\": true }";

const char kResponseRegisterComplete[] = "{"
    "    \"action\": \"complete\","
    "    \"user\": \"user@host.com\","
    "    \"device_id\": \"my_id\""
    "}";

const char kResponseGaiaToken[] = "{"
    "  \"access_token\": \"at1\","
    "  \"expires_in\": 3600,"
    "  \"token_type\": \"Bearer\""
    "}";

const char kURLInfo[] = "http://1.2.3.4:8888/privet/info";

const char kURLRegisterStart[] =
    "http://1.2.3.4:8888/privet/register?action=start&user=user@host.com";

const char kURLRegisterClaimToken[] =
    "http://1.2.3.4:8888/privet/register?action=getClaimToken&"
    "user=user@host.com";

const char kURLCloudPrintConfirm[] =
    "https://www.google.com/cloudprint/confirm?token=MySampleToken";

const char kURLRegisterComplete[] =
    "http://1.2.3.4:8888/privet/register?action=complete&user=user@host.com";

const char kURLGaiaToken[] =
    "https://accounts.google.com/o/oauth2/token";

const char kSampleUser[] = "user@host.com";

class TestMessageLoopCondition {
 public:
  TestMessageLoopCondition() : signaled_(false),
                               waiting_(false) {
  }

  ~TestMessageLoopCondition() {
  }

  // Signal a waiting method that it can continue executing.
  void Signal() {
    signaled_ = true;
    if (waiting_)
      base::MessageLoop::current()->Quit();
  }

  // Pause execution and recursively run the message loop until |Signal()| is
  // called. Do not pause if |Signal()| has already been called.
  void Wait() {
    while (!signaled_) {
      waiting_ = true;
      base::MessageLoop::current()->Run();
      waiting_ = false;
    }
    signaled_ = false;
  }

 private:
  bool signaled_;
  bool waiting_;

  DISALLOW_COPY_AND_ASSIGN(TestMessageLoopCondition);
};

class MockableFakeURLFetcherCreator {
 public:
  MockableFakeURLFetcherCreator() {
  }

  ~MockableFakeURLFetcherCreator() {
  }

  MOCK_METHOD1(OnCreateFakeURLFetcher, void(const std::string& url));

  scoped_ptr<net::FakeURLFetcher> CreateFakeURLFetcher(
      const GURL& url,
      net::URLFetcherDelegate* delegate,
      const std::string& response,
      bool success) {
    OnCreateFakeURLFetcher(url.spec());
    return scoped_ptr<net::FakeURLFetcher>(
        new net::FakeURLFetcher(url, delegate, response, success));
  }

  net::FakeURLFetcherFactory::FakeURLFetcherCreator callback() {
    return base::Bind(&MockableFakeURLFetcherCreator::CreateFakeURLFetcher,
                      base::Unretained(this));
  }
};

class LocalDiscoveryUITest : public WebUIBrowserTest {
 public:
  LocalDiscoveryUITest() : fake_fetcher_factory_(
      &fetcher_impl_factory_,
      fake_url_fetcher_creator_.callback()) {
  }
  virtual ~LocalDiscoveryUITest() {
  }

  virtual void SetUpOnMainThread() OVERRIDE {
    WebUIBrowserTest::SetUpOnMainThread();

    test_service_discovery_client_ = new TestServiceDiscoveryClient();
    test_service_discovery_client_->Start();
    EXPECT_CALL(*test_service_discovery_client_, OnSendTo(
        std::string((const char*)kQueryData,
                    sizeof(kQueryData))))
        .Times(AtLeast(2))
        .WillOnce(InvokeWithoutArgs(&condition_devices_listed_,
                                    &TestMessageLoopCondition::Signal))
        .WillRepeatedly(Return());

    SigninManagerBase* signin_manager =
        SigninManagerFactory::GetForProfile(browser()->profile());

    DCHECK(signin_manager);
    signin_manager->SetAuthenticatedUsername(kSampleUser);

    fake_fetcher_factory().SetFakeResponse(
        GURL(kURLInfo),
        kResponseInfo,
        true);

    fake_fetcher_factory().SetFakeResponse(
        GURL(kURLRegisterStart),
        kResponseRegisterStart,
        true);

    fake_fetcher_factory().SetFakeResponse(
        GURL(kURLRegisterClaimToken),
        kResponseRegisterClaimTokenNoConfirm,
        true);

    fake_fetcher_factory().SetFakeResponse(
        GURL(kURLCloudPrintConfirm),
        kResponseCloudPrintConfirm,
        true);

    fake_fetcher_factory().SetFakeResponse(
        GURL(kURLRegisterComplete),
        kResponseRegisterComplete,
        true);

    fake_fetcher_factory().SetFakeResponse(
        GURL(kURLGaiaToken),
        kResponseGaiaToken,
        true);

    EXPECT_CALL(fake_url_fetcher_creator(), OnCreateFakeURLFetcher(
        kURLGaiaToken))
        .Times(AnyNumber());

    ProfileOAuth2TokenService* token_service =
        ProfileOAuth2TokenServiceFactory::GetForProfile(browser()->profile());

    token_service->UpdateCredentials("user@host.com",
                                     "MyFakeToken");

    AddLibrary(base::FilePath(FILE_PATH_LITERAL("local_discovery_ui_test.js")));
  }

  virtual void SetUpCommandLine(CommandLine* command_line) OVERRIDE {
    WebUIBrowserTest::SetUpCommandLine(command_line);
  }

  void RunFor(base::TimeDelta time_period) {
    base::CancelableCallback<void()> callback(base::Bind(
        &base::MessageLoop::Quit, base::Unretained(
            base::MessageLoop::current())));
    base::MessageLoop::current()->PostDelayedTask(
        FROM_HERE, callback.callback(), time_period);

    base::MessageLoop::current()->Run();
    callback.Cancel();
  }

  TestServiceDiscoveryClient* test_service_discovery_client() {
    return test_service_discovery_client_.get();
  }

  TestMessageLoopCondition& condition_devices_listed() {
    return condition_devices_listed_;
  }

  net::FakeURLFetcherFactory& fake_fetcher_factory() {
    return fake_fetcher_factory_;
  }

  MockableFakeURLFetcherCreator& fake_url_fetcher_creator() {
    return fake_url_fetcher_creator_;
  }

 private:
  scoped_refptr<TestServiceDiscoveryClient> test_service_discovery_client_;
  TestMessageLoopCondition condition_devices_listed_;

  net::URLFetcherImplFactory fetcher_impl_factory_;
  StrictMock<MockableFakeURLFetcherCreator> fake_url_fetcher_creator_;
  net::FakeURLFetcherFactory fake_fetcher_factory_;

  DISALLOW_COPY_AND_ASSIGN(LocalDiscoveryUITest);
};

IN_PROC_BROWSER_TEST_F(LocalDiscoveryUITest, EmptyTest) {
  ui_test_utils::NavigateToURL(browser(), GURL(
      chrome::kChromeUIDevicesURL));
  condition_devices_listed().Wait();
  EXPECT_TRUE(WebUIBrowserTest::RunJavascriptTest("checkNoDevices"));
}

IN_PROC_BROWSER_TEST_F(LocalDiscoveryUITest, AddRowTest) {
  ui_test_utils::NavigateToURL(browser(), GURL(
      chrome::kChromeUIDevicesURL));
  condition_devices_listed().Wait();

  test_service_discovery_client()->SimulateReceive(
      kAnnouncePacket, sizeof(kAnnouncePacket));

  base::MessageLoop::current()->RunUntilIdle();

  EXPECT_TRUE(WebUIBrowserTest::RunJavascriptTest("checkOneDevice"));

  test_service_discovery_client()->SimulateReceive(
      kGoodbyePacket, sizeof(kGoodbyePacket));

  RunFor(base::TimeDelta::FromMilliseconds(1100));

  EXPECT_TRUE(WebUIBrowserTest::RunJavascriptTest("checkNoDevices"));
}


IN_PROC_BROWSER_TEST_F(LocalDiscoveryUITest, RegisterTest) {
  TestMessageLoopCondition condition_token_claimed;

  ui_test_utils::NavigateToURL(browser(), GURL(
      chrome::kChromeUIDevicesURL));
  condition_devices_listed().Wait();

  test_service_discovery_client()->SimulateReceive(
      kAnnouncePacket, sizeof(kAnnouncePacket));

  base::MessageLoop::current()->RunUntilIdle();

  EXPECT_TRUE(WebUIBrowserTest::RunJavascriptTest("checkOneDevice"));

  EXPECT_TRUE(WebUIBrowserTest::RunJavascriptTest("registerShowOverlay"));

  {
    InSequence s;
    EXPECT_CALL(fake_url_fetcher_creator(), OnCreateFakeURLFetcher(kURLInfo));
    EXPECT_CALL(fake_url_fetcher_creator(), OnCreateFakeURLFetcher(
        kURLRegisterStart));
    EXPECT_CALL(fake_url_fetcher_creator(), OnCreateFakeURLFetcher(
        kURLRegisterClaimToken))
        .WillOnce(InvokeWithoutArgs(&condition_token_claimed,
                                    &TestMessageLoopCondition::Signal));
  }

  EXPECT_TRUE(WebUIBrowserTest::RunJavascriptTest("registerBegin"));

  condition_token_claimed.Wait();

  EXPECT_TRUE(WebUIBrowserTest::RunJavascriptTest("expectPageAdding1"));

  fake_fetcher_factory().SetFakeResponse(
      GURL(kURLRegisterClaimToken),
      kResponseRegisterClaimTokenConfirm,
      true);

  {
    InSequence s;
    EXPECT_CALL(fake_url_fetcher_creator(), OnCreateFakeURLFetcher(
        kURLRegisterClaimToken));
    EXPECT_CALL(fake_url_fetcher_creator(), OnCreateFakeURLFetcher(
        kURLCloudPrintConfirm));
    EXPECT_CALL(fake_url_fetcher_creator(), OnCreateFakeURLFetcher(
        kURLRegisterComplete))
        .WillOnce(InvokeWithoutArgs(&condition_token_claimed,
                                    &TestMessageLoopCondition::Signal));
  }

  condition_token_claimed.Wait();

  test_service_discovery_client()->SimulateReceive(
      kAnnouncePacketRegistered, sizeof(kAnnouncePacketRegistered));

  base::MessageLoop::current()->RunUntilIdle();

  EXPECT_TRUE(WebUIBrowserTest::RunJavascriptTest("expectRegisterDone"));
}

}  // namespace

}  // namespace local_discovery
