// Copyright (C) 2023 The Xaya developers
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

  std::string host, user, password, db, tbl;
  unsigned port;

  void
  Parse (const std::string& url)
  {
    ParseUrl (url, host, port, user, password, db, tbl);
  }

};

TEST_F (UrlTests, FullUrl)
{
  Parse ("mysql://domob:foo:bar@example.com:123/database/table");
  EXPECT_EQ (host, "example.com");
  EXPECT_EQ (port, 123);
  EXPECT_EQ (user, "domob");
  EXPECT_EQ (password, "foo:bar");
  EXPECT_EQ (db, "database");
  EXPECT_EQ (tbl, "table");
}

TEST_F (UrlTests, WithoutTable)
{
  Parse ("mysql://domob:pwd@example.com:123/database");
  EXPECT_EQ (host, "example.com");
  EXPECT_EQ (port, 123);
  EXPECT_EQ (user, "domob");
  EXPECT_EQ (password, "pwd");
  EXPECT_EQ (db, "database");
  EXPECT_EQ (tbl, "");
}

TEST_F (UrlTests, Invalid)
{
  EXPECT_THROW (Parse ("domob:pwd@example.com:123/database"), Error);
  EXPECT_THROW (Parse ("mysql://example.com:123/database"), Error);
  EXPECT_THROW (Parse ("mysql://domob@example.com:123/database"), Error);
  EXPECT_THROW (Parse ("mysql://domob:pwd@example.com:123"), Error);
  EXPECT_THROW (Parse ("mysql://domob:pwd@example.com/database"), Error);
  EXPECT_THROW (Parse ("mysql://domob:pwd@example.com:03/database"), Error);
}

} // anonymous namespace
} // namespace mypp
