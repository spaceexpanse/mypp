// Copyright (C) 2023-2024 The Xaya developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "url.hpp"

#include "error.hpp"

#include <gtest/gtest.h>

namespace mypp
{
namespace
{

class UrlTests : public testing::Test
{

protected:

  UrlParser p;

};

TEST_F (UrlTests, FullUrl)
{
  p.Parse ("mysql://domob:foo:bar@example.com:123/database/table");
  EXPECT_EQ (p.GetHost (), "example.com");
  EXPECT_EQ (p.GetPort (), 123);
  EXPECT_EQ (p.GetUser (), "domob");
  EXPECT_EQ (p.GetPassword (), "foo:bar");
  EXPECT_EQ (p.GetDatabase (), "database");
  EXPECT_TRUE (p.HasTable ());
  EXPECT_EQ (p.GetTable (), "table");
}

TEST_F (UrlTests, WithoutTable)
{
  p.Parse ("mysql://domob:pwd@example.com:123/database");
  EXPECT_EQ (p.GetHost (), "example.com");
  EXPECT_EQ (p.GetPort (), 123);
  EXPECT_EQ (p.GetUser (), "domob");
  EXPECT_EQ (p.GetPassword (), "pwd");
  EXPECT_EQ (p.GetDatabase (), "database");
  EXPECT_FALSE (p.HasTable ());
}

TEST_F (UrlTests, Options)
{
  p.Parse ("mysql://a:b@c.d:0/x");
  EXPECT_FALSE (p.HasOption ("foo"));

  p.Parse ("mysql://a:b@c.d:0/x?");
  EXPECT_FALSE (p.HasOption ("foo"));

  p.Parse ("mysql://a:b@c.d:0/x/y?foo=");
  EXPECT_TRUE (p.HasOption ("foo"));
  EXPECT_EQ (p.GetOption ("foo"), "");

  p.Parse ("mysql://a:b@c.d:0/x?foo=1/2&bar=34=5&=");
  EXPECT_EQ (p.GetOption ("foo"), "1/2");
  EXPECT_EQ (p.GetOption ("bar"), "34=5");
  EXPECT_EQ (p.GetOption (""), "");
}

TEST_F (UrlTests, Invalid)
{
  EXPECT_THROW (p.Parse ("domob:pwd@example.com:123/database"), Error);
  EXPECT_THROW (p.Parse ("mysql://example.com:123/database"), Error);
  EXPECT_THROW (p.Parse ("mysql://domob@example.com:123/database"), Error);
  EXPECT_THROW (p.Parse ("mysql://domob:pwd@example.com:123"), Error);
  EXPECT_THROW (p.Parse ("mysql://domob:pwd@example.com/database"), Error);
  EXPECT_THROW (p.Parse ("mysql://domob:pwd@example.com:03/database"), Error);
  EXPECT_THROW (p.Parse ("mysql://domob:pwd@example.com:123/"), Error);
  EXPECT_THROW (p.Parse ("mysql://domob:pwd@example.com:123/?foo=bar"), Error);
  EXPECT_THROW (p.Parse ("mysql://a:b@c.d:0/x?foo"), Error);
  EXPECT_THROW (p.Parse ("mysql://a:b@c.d:0/x?foo=bar&"), Error);
  EXPECT_THROW (p.Parse ("mysql://a:b@c.d:0/x?foo=bar&&x=y"), Error);
}

} // anonymous namespace
} // namespace mypp
