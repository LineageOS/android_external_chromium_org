// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_content_disposition.h"

#include "base/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

namespace {

struct FileNameCDCase {
  const char* header;
  const char* referrer_charset;
  const wchar_t* expected;
};

}  // anonymous namespace

TEST(HttpContentDispositionTest, Filename) {
  const FileNameCDCase tests[] = {
    // Test various forms of C-D header fields emitted by web servers.
    {"inline; filename=\"abcde.pdf\"", "", L"abcde.pdf"},
    {"inline; name=\"abcde.pdf\"", "", L"abcde.pdf"},
    {"attachment; filename=abcde.pdf", "", L"abcde.pdf"},
    {"attachment; name=abcde.pdf", "", L"abcde.pdf"},
    {"attachment; filename=abc,de.pdf", "", L"abc,de.pdf"},
    {"filename=abcde.pdf", "", L"abcde.pdf"},
    {"filename= abcde.pdf", "", L"abcde.pdf"},
    {"filename =abcde.pdf", "", L"abcde.pdf"},
    {"filename = abcde.pdf", "", L"abcde.pdf"},
    {"filename\t=abcde.pdf", "", L"abcde.pdf"},
    {"filename \t\t  =abcde.pdf", "", L"abcde.pdf"},
    {"name=abcde.pdf", "", L"abcde.pdf"},
    {"inline; filename=\"abc%20de.pdf\"", "",
     L"abc de.pdf"},
    // Unbalanced quotation mark
    {"filename=\"abcdef.pdf", "", L"abcdef.pdf"},
    // Whitespaces are converted to a space.
    {"inline; filename=\"abc  \t\nde.pdf\"", "",
     L"abc    de.pdf"},
    // %-escaped UTF-8
    {"attachment; filename=\"%EC%98%88%EC%88%A0%20"
     "%EC%98%88%EC%88%A0.jpg\"", "", L"\xc608\xc220 \xc608\xc220.jpg"},
    {"attachment; filename=\"%F0%90%8C%B0%F0%90%8C%B1"
     "abc.jpg\"", "", L"\U00010330\U00010331abc.jpg"},
    {"attachment; filename=\"%EC%98%88%EC%88%A0 \n"
     "%EC%98%88%EC%88%A0.jpg\"", "", L"\xc608\xc220  \xc608\xc220.jpg"},
    // RFC 2047 with various charsets and Q/B encodings
    {"attachment; filename=\"=?EUC-JP?Q?=B7=DD=BD="
     "D13=2Epng?=\"", "", L"\x82b8\x8853" L"3.png"},
    {"attachment; filename==?eUc-Kr?b?v7m8+iAzLnBuZw==?=",
     "", L"\xc608\xc220 3.png"},
    {"attachment; filename==?utf-8?Q?=E8=8A=B8=E8"
     "=A1=93_3=2Epng?=", "", L"\x82b8\x8853 3.png"},
    {"attachment; filename==?utf-8?Q?=F0=90=8C=B0"
     "_3=2Epng?=", "", L"\U00010330 3.png"},
    {"inline; filename=\"=?iso88591?Q?caf=e9_=2epng?=\"",
     "", L"caf\x00e9 .png"},
    // Space after an encoded word should be removed.
    {"inline; filename=\"=?iso88591?Q?caf=E9_?= .png\"",
     "", L"caf\x00e9 .png"},
    // Two encoded words with different charsets (not very likely to be emitted
    // by web servers in the wild). Spaces between them are removed.
    {"inline; filename=\"=?euc-kr?b?v7m8+iAz?="
     " =?ksc5601?q?=BF=B9=BC=FA=2Epng?=\"", "",
     L"\xc608\xc220 3\xc608\xc220.png"},
    {"attachment; filename=\"=?windows-1252?Q?caf=E9?="
     "  =?iso-8859-7?b?4eI=?= .png\"", "", L"caf\x00e9\x03b1\x03b2.png"},
    // Non-ASCII string is passed through and treated as UTF-8 as long as
    // it's valid as UTF-8 and regardless of |referrer_charset|.
    {"attachment; filename=caf\xc3\xa9.png",
     "iso-8859-1", L"caf\x00e9.png"},
    {"attachment; filename=caf\xc3\xa9.png",
     "", L"caf\x00e9.png"},
    // Non-ASCII/Non-UTF-8 string. Fall back to the referrer charset.
    {"attachment; filename=caf\xe5.png",
     "windows-1253", L"caf\x03b5.png"},
#if 0
    // Non-ASCII/Non-UTF-8 string. Fall back to the native codepage.
    // TODO(jungshik): We need to set the OS default codepage
    // to a specific value before testing. On Windows, we can use
    // SetThreadLocale().
    {"attachment; filename=\xb0\xa1\xb0\xa2.png",
     "", L"\xac00\xac01.png"},
#endif
    // Failure cases
    // Invalid hex-digit "G"
    {"attachment; filename==?iiso88591?Q?caf=EG?=", "",
     L""},
    // Incomplete RFC 2047 encoded-word (missing '='' at the end)
    {"attachment; filename==?iso88591?Q?caf=E3?", "", L""},
    // Extra character at the end of an encoded word
    {"attachment; filename==?iso88591?Q?caf=E3?==",
     "", L""},
    // Extra token at the end of an encoded word
    {"attachment; filename==?iso88591?Q?caf=E3?=?",
     "", L""},
    {"attachment; filename==?iso88591?Q?caf=E3?=?=",
     "",  L""},
    // Incomplete hex-escaped chars
    {"attachment; filename==?windows-1252?Q?=63=61=E?=",
     "", L""},
    {"attachment; filename=%EC%98%88%EC%88%A", "", L""},
    // %-escaped non-UTF-8 encoding is an "error"
    {"attachment; filename=%B7%DD%BD%D1.png", "", L""},
    // Two RFC 2047 encoded words in a row without a space is an error.
    {"attachment; filename==?windows-1252?Q?caf=E3?="
     "=?iso-8859-7?b?4eIucG5nCg==?=", "", L""},

    // RFC 5987 tests with Filename*  : see http://tools.ietf.org/html/rfc5987
    {"attachment; filename*=foo.html", "", L""},
    {"attachment; filename*=foo'.html", "", L""},
    {"attachment; filename*=''foo'.html", "", L""},
    {"attachment; filename*=''foo.html'", "", L""},
    {"attachment; filename*=''f\"oo\".html'", "", L""},
    {"attachment; filename*=bogus_charset''foo.html'",
     "", L""},
    {"attachment; filename*='en'foo.html'", "", L""},
    {"attachment; filename*=iso-8859-1'en'foo.html", "",
      L"foo.html"},
    {"attachment; filename*=utf-8'en'foo.html", "",
      L"foo.html"},
    // charset cannot be omitted.
    {"attachment; filename*='es'f\xfa.html'", "", L""},
    // Non-ASCII bytes are not allowed.
    {"attachment; filename*=iso-8859-1'es'f\xfa.html", "",
      L""},
    {"attachment; filename*=utf-8'es'f\xce\xba.html", "",
      L""},
    // TODO(jshin): Space should be %-encoded, but currently, we allow
    // spaces.
    {"inline; filename*=iso88591''cafe foo.png", "",
      L"cafe foo.png"},

    // Filename* tests converted from Q-encoded tests above.
    {"attachment; filename*=EUC-JP''%B7%DD%BD%D13%2Epng",
     "", L"\x82b8\x8853" L"3.png"},
    {"attachment; filename*=utf-8''"
      "%E8%8A%B8%E8%A1%93%203%2Epng", "", L"\x82b8\x8853 3.png"},
    {"attachment; filename*=utf-8''%F0%90%8C%B0 3.png", "",
      L"\U00010330 3.png"},
    {"inline; filename*=Euc-Kr'ko'%BF%B9%BC%FA%2Epng", "",
     L"\xc608\xc220.png"},
    {"attachment; filename*=windows-1252''caf%E9.png", "",
      L"caf\x00e9.png"},

    // http://greenbytes.de/tech/tc2231/ filename* test cases.
    // attwithisofn2231iso
    {"attachment; filename*=iso-8859-1''foo-%E4.html", "",
      L"foo-\xe4.html"},
    // attwithfn2231utf8
    {"attachment; filename*="
      "UTF-8''foo-%c3%a4-%e2%82%ac.html", "", L"foo-\xe4-\x20ac.html"},
    // attwithfn2231noc : no encoding specified but UTF-8 is used.
    {"attachment; filename*=''foo-%c3%a4-%e2%82%ac.html",
      "", L""},
    // attwithfn2231utf8comp
    {"attachment; filename*=UTF-8''foo-a%cc%88.html", "",
      L"foo-\xe4.html"},
#ifdef ICU_SHOULD_FAIL_CONVERSION_ON_INVALID_CHARACTER
    // This does not work because we treat ISO-8859-1 synonymous with
    // Windows-1252 per HTML5. For HTTP, in theory, we're not
    // supposed to.
    // attwithfn2231utf8-bad
    {"attachment; filename*="
      "iso-8859-1''foo-%c3%a4-%e2%82%ac.html", "", L""},
#endif
    // attwithfn2231ws1
    {"attachment; filename *=UTF-8''foo-%c3%a4.html", "",
      L""},
    // attwithfn2231ws2
    {"attachment; filename*= UTF-8''foo-%c3%a4.html", "",
      L"foo-\xe4.html"},
    // attwithfn2231ws3
    {"attachment; filename* =UTF-8''foo-%c3%a4.html", "",
      L"foo-\xe4.html"},
    // attwithfn2231quot
    {"attachment; filename*=\"UTF-8''foo-%c3%a4.html\"",
      "", L""},
    // attfnboth
    {"attachment; filename=\"foo-ae.html\"; "
      "filename*=UTF-8''foo-%c3%a4.html", "", L"foo-\xe4.html"},
    // attfnboth2
    {"attachment; filename*=UTF-8''foo-%c3%a4.html; "
      "filename=\"foo-ae.html\"", "", L"foo-\xe4.html"},
    // attnewandfn
    {"attachment; foobar=x; filename=\"foo.html\"", "",
      L"foo.html"},
  };
  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(tests); ++i) {
    HttpContentDisposition header(tests[i].header, tests[i].referrer_charset);
    EXPECT_EQ(tests[i].expected,
        UTF8ToWide(header.filename()))
        << "Failed on input: " << tests[i].header;
  }
}

// Test cases from http://greenbytes.de/tech/tc2231/
TEST(HttpContentDispositionTest, tc2231) {
  const struct FileNameCDCase {
    const char* header;
    net::HttpContentDisposition::Type expected_type;
    const wchar_t* expected_filename;
  } tests[] = {
    // http://greenbytes.de/tech/tc2231/#inlonly
    { "inline",
      net::HttpContentDisposition::INLINE,
      L""
    },
    // http://greenbytes.de/tech/tc2231/#inlonlyquoted
    { "\"inline\"",
      net::HttpContentDisposition::ATTACHMENT,
      L""
    },
    // http://greenbytes.de/tech/tc2231/#inlwithasciifilename
    { "inline; filename=\"foo.html\"",
      net::HttpContentDisposition::INLINE,
      L"foo.html"
    },
    // http://greenbytes.de/tech/tc2231/#inlwithfnattach
    { "inline; filename=\"Not an attachment!\"",
      net::HttpContentDisposition::INLINE,
      L"Not an attachment!"
    },
    // http://greenbytes.de/tech/tc2231/#inlwithasciifilenamepdf
    { "inline; filename=\"foo.pdf\"",
      net::HttpContentDisposition::INLINE,
      L"foo.pdf"
    },
    // http://greenbytes.de/tech/tc2231/#attonly
    { "attachment",
      net::HttpContentDisposition::ATTACHMENT,
      L""
    },
    // http://greenbytes.de/tech/tc2231/#attonlyquoted
    { "\"attachment\"",
      net::HttpContentDisposition::ATTACHMENT,
      L""
    },
    // http://greenbytes.de/tech/tc2231/#attonly403
    // TODO(abarth): This isn't testable in this unit test.
    // http://greenbytes.de/tech/tc2231/#attonlyucase
    { "ATTACHMENT",
      net::HttpContentDisposition::ATTACHMENT,
      L""
    },
    // http://greenbytes.de/tech/tc2231/#attwithasciifilename
    { "attachment; filename=\"foo.html\"",
      net::HttpContentDisposition::ATTACHMENT,
      L"foo.html"
    },
    // http://greenbytes.de/tech/tc2231/#attwithasciifnescapedchar
    { "attachment; filename=\"f\\oo.html\"",
      net::HttpContentDisposition::ATTACHMENT,
      L"foo.html"
    },
    // http://greenbytes.de/tech/tc2231/#attwithasciifnescapedquote
    { "attachment; filename=\"\\\"quoting\\\" tested.html\"",
      net::HttpContentDisposition::ATTACHMENT,
      L"\"quoting\" tested.html"
    },
    // http://greenbytes.de/tech/tc2231/#attwithquotedsemicolon
    { "attachment; filename=\"Here's a semicolon;.html\"",
      net::HttpContentDisposition::ATTACHMENT,
      L"Here's a semicolon;.html"
    },
    // http://greenbytes.de/tech/tc2231/#attwithfilenameandextparam
    { "attachment; foo=\"bar\"; filename=\"foo.html\"",
      net::HttpContentDisposition::ATTACHMENT,
      L"foo.html"
    },
    // http://greenbytes.de/tech/tc2231/#attwithfilenameandextparamescaped
    { "attachment; foo=\"\\\"\\\\\";filename=\"foo.html\"",
      net::HttpContentDisposition::ATTACHMENT,
      L"foo.html"
    },
    // http://greenbytes.de/tech/tc2231/#attwithasciifilenameucase
    { "attachment; FILENAME=\"foo.html\"",
      net::HttpContentDisposition::ATTACHMENT,
      L"foo.html"
    },
    // http://greenbytes.de/tech/tc2231/#attwithasciifilenamenq
    { "attachment; filename=foo.html",
      net::HttpContentDisposition::ATTACHMENT,
      L"foo.html"
    },
    // http://greenbytes.de/tech/tc2231/#attwithasciifilenamenqs
    // Note: tc2231 says we should fail to parse this header.
    { "attachment; filename=foo.html ;",
      net::HttpContentDisposition::ATTACHMENT,
      L"foo.html"
    },
    // http://greenbytes.de/tech/tc2231/#attemptyparam
    // Note: tc2231 says we should fail to parse this header.
    { "attachment; ;filename=foo",
      net::HttpContentDisposition::ATTACHMENT,
      L"foo"
    },
    // http://greenbytes.de/tech/tc2231/#attwithasciifilenamenqws
    // Note: tc2231 says we should fail to parse this header.
    { "attachment; filename=foo bar.html",
      net::HttpContentDisposition::ATTACHMENT,
      L"foo bar.html"
    },
    // http://greenbytes.de/tech/tc2231/#attwithfntokensq
    { "attachment; filename='foo.bar'",
      net::HttpContentDisposition::ATTACHMENT,
      L"foo.bar"  // Should be L"'foo.bar'"
    },
    // http://greenbytes.de/tech/tc2231/#attwithisofnplain
    { "attachment; filename=\"foo-\xE4html\"",
      net::HttpContentDisposition::ATTACHMENT,
      L""  // Should be L"foo-\xE4.html"
    },
    // http://greenbytes.de/tech/tc2231/#attwithutf8fnplain
    // Note: We'll UTF-8 decode the file name, even though tc2231 says not to.
    { "attachment; filename=\"foo-\xC3\xA4.html\"",
      net::HttpContentDisposition::ATTACHMENT,
      L"foo-\xE4.html"
    },
    // http://greenbytes.de/tech/tc2231/#attwithfnrawpctenca
    { "attachment; filename=\"foo-%41.html\"",
      net::HttpContentDisposition::ATTACHMENT,
      L"foo-A.html"  // Should be L"foo-%41.html"
    },
    // http://greenbytes.de/tech/tc2231/#attwithfnusingpct
    { "attachment; filename=\"50%.html\"",
      net::HttpContentDisposition::ATTACHMENT,
      L"50%.html"
    },
    // http://greenbytes.de/tech/tc2231/#attwithfnrawpctencaq
    { "attachment; filename=\"foo-%\\41.html\"",
      net::HttpContentDisposition::ATTACHMENT,
      L"foo-A.html"  // Should be L"foo-%41.html"
    },
    // http://greenbytes.de/tech/tc2231/#attwithnamepct
    { "attachment; name=\"foo-%41.html\"",
      net::HttpContentDisposition::ATTACHMENT,
      L"foo-A.html"  // Should be L"foo-%41.html"
    },
    // http://greenbytes.de/tech/tc2231/#attwithfilenamepctandiso
    { "attachment; filename=\"\xE4-%41.html\"",
      net::HttpContentDisposition::ATTACHMENT,
      L""  // Should be L"\xE4-%41.htm"
    },
    // http://greenbytes.de/tech/tc2231/#attwithfnrawpctenclong
    { "attachment; filename=\"foo-%c3%a4-%e2%82%ac.html\"",
      net::HttpContentDisposition::ATTACHMENT,
      L"foo-\xE4-\u20AC.html"  // Should be L"foo-%c3%a4-%e2%82%ac.html"
    },
    // http://greenbytes.de/tech/tc2231/#attwithasciifilenamews1
    { "attachment; filename =\"foo.html\"",
      net::HttpContentDisposition::ATTACHMENT,
      L"foo.html"
    },
    // http://greenbytes.de/tech/tc2231/#attwith2filenames
    // Note: tc2231 says we should fail to parse this header.
    { "attachment; filename=\"foo.html\"; filename=\"bar.html\"",
      net::HttpContentDisposition::ATTACHMENT,
      L"bar.html"  // Probably should be foo.html to match other browsers.
    },
    // http://greenbytes.de/tech/tc2231/#attfnbrokentoken
    // Note: tc2231 says we should fail to parse this header.
    { "attachment; filename=foo[1](2).html",
      net::HttpContentDisposition::ATTACHMENT,
      L"foo[1](2).html"
    },
    // http://greenbytes.de/tech/tc2231/#attfnbrokentokeniso
    // Note: tc2231 says we should fail to parse this header.
    { "attachment; filename=foo-\xE4.html",
      net::HttpContentDisposition::ATTACHMENT,
      L""
    },
    // http://greenbytes.de/tech/tc2231/#attfnbrokentokenutf
    // Note: tc2231 says we should fail to parse this header.
    { "attachment; filename=foo-\xC3\xA4.html",
      net::HttpContentDisposition::ATTACHMENT,
      L"foo-\xE4.html"
    },
    // http://greenbytes.de/tech/tc2231/#attmissingdisposition
    // Note: tc2231 says we should fail to parse this header.
    { "filename=foo.html",
      net::HttpContentDisposition::INLINE,
      L"foo.html"
    },
    // http://greenbytes.de/tech/tc2231/#attmissingdisposition2
    // Note: tc2231 says we should fail to parse this header.
    { "x=y; filename=foo.html",
      net::HttpContentDisposition::INLINE,
      L"foo.html"
    },
    // http://greenbytes.de/tech/tc2231/#attmissingdisposition3
    // Note: tc2231 says we should fail to parse this header.
    { "\"foo; filename=bar;baz\"; filename=qux",
      net::HttpContentDisposition::ATTACHMENT,
      L"bar"  // Firefox gets qux
    },
    // http://greenbytes.de/tech/tc2231/#attmissingdisposition4
    // Note: tc2231 says we should fail to parse this header.
    { "filename=foo.html, filename=bar.html",
      net::HttpContentDisposition::INLINE,
      L"foo.html, filename=bar.html"
    },
    // http://greenbytes.de/tech/tc2231/#emptydisposition
    // Note: tc2231 says we should fail to parse this header.
    { "; filename=foo.html",
      net::HttpContentDisposition::ATTACHMENT,  // Should be INLINE?
      L"foo.html"
    },
    // http://greenbytes.de/tech/tc2231/#attandinline
    // Note: tc2231 says we should fail to parse this header.
    { "inline; attachment; filename=foo.html",
      net::HttpContentDisposition::INLINE,
      L""
    },
    // http://greenbytes.de/tech/tc2231/#attandinline2
    // Note: tc2231 says we should fail to parse this header.
    { "attachment; inline; filename=foo.html",
      net::HttpContentDisposition::ATTACHMENT,
      L""
    },
    // http://greenbytes.de/tech/tc2231/#attbrokenquotedfn
    // Note: tc2231 says we should fail to parse this header.
    { "attachment; filename=\"foo.html\".txt",
      net::HttpContentDisposition::ATTACHMENT,
      L"foo.html\".txt"
    },
    // http://greenbytes.de/tech/tc2231/#attbrokenquotedfn2
    // Note: tc2231 says we should fail to parse this header.
    { "attachment; filename=\"bar",
      net::HttpContentDisposition::ATTACHMENT,
      L"bar"
    },
    // http://greenbytes.de/tech/tc2231/#attbrokenquotedfn3
    // Note: tc2231 says we should fail to parse this header.
    { "attachment; filename=foo\"bar;baz\"qux",
      net::HttpContentDisposition::ATTACHMENT,
      L"foo\"bar;baz\"qux"
    },
    // http://greenbytes.de/tech/tc2231/#attmultinstances
    { "attachment; filename=foo.html, attachment; filename=bar.html",
    // Note: tc2231 says we should fail to parse this header.
      net::HttpContentDisposition::ATTACHMENT,
      L"bar.html"
    },
    // http://greenbytes.de/tech/tc2231/#attmissingdelim
    { "attachment; foo=foo filename=bar",
      net::HttpContentDisposition::ATTACHMENT,
      L""
    },
    // http://greenbytes.de/tech/tc2231/#attreversed
    // Note: tc2231 says we should fail to parse this header.
    { "filename=foo.html; attachment",
      net::HttpContentDisposition::INLINE,
      L"foo.html"
    },
    // http://greenbytes.de/tech/tc2231/#attconfusedparam
    { "attachment; xfilename=foo.html",
      net::HttpContentDisposition::ATTACHMENT,
      L""
    },
    // http://greenbytes.de/tech/tc2231/#attabspath
    { "attachment; filename=\"/foo.html\"",
      net::HttpContentDisposition::ATTACHMENT,
      L"/foo.html"
    },
    // http://greenbytes.de/tech/tc2231/#attabspathwin
    { "attachment; filename=\"\\\\foo.html\"",
      net::HttpContentDisposition::ATTACHMENT,
      L"\\foo.html"
    },
    // http://greenbytes.de/tech/tc2231/#dispext
    { "foobar",
      net::HttpContentDisposition::ATTACHMENT,
      L""
    },
    // http://greenbytes.de/tech/tc2231/#dispextbadfn
    { "attachment; example=\"filename=example.txt\"",
      net::HttpContentDisposition::ATTACHMENT,
      L""
    },
    // http://greenbytes.de/tech/tc2231/#attnewandfn
    { "attachment; foobar=x; filename=\"foo.html\"",
      net::HttpContentDisposition::ATTACHMENT,
      L"foo.html"
    },
    // TODO(abarth): Add the filename* tests, but check
    //              HttpContentDispositionTest.Filename for overlap.
    // TODO(abarth): http://greenbytes.de/tech/tc2231/#attrfc2047token
    // TODO(abarth): http://greenbytes.de/tech/tc2231/#attrfc2047quoted
  };
  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(tests); ++i) {
    HttpContentDisposition header(tests[i].header, std::string());
    EXPECT_EQ(tests[i].expected_type, header.type())
        << "Failed on input: " << tests[i].header;
    EXPECT_EQ(tests[i].expected_filename, UTF8ToWide(header.filename()))
        << "Failed on input: " << tests[i].header;
  }
}

}  // namespace net
