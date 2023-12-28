// Copyright (C) 2023-2024 The Xaya developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "connection.hpp"

#include "error.hpp"

#include <glog/logging.h>

#include <sstream>

namespace mypp
{

namespace
{

/**
 * Constructs an Error instance from the MySQL-reported error data
 * of the given handle.
 */
Error
MySqlError (MYSQL* h)
{
  std::ostringstream out;
  out << "MySQL error " << mysql_errno (h) << " / " << mysql_sqlstate (h)
      << ": " << mysql_error (h);

  return Error (out.str ());
}

} // anonymous namespace

Connection::Connection ()
{
  handle = mysql_init (nullptr);
  CHECK (handle != nullptr) << "Failed to initialise MySQL";
}

Connection::~Connection ()
{
  mysql_close (handle);
}

void
Connection::Connect (const std::string& host, const unsigned port,
                     const std::string& user, const std::string& password,
                     const std::string& db)
{
  CHECK (!connected) << "MySQL connection is already up";

  const char* dbToUse = (db.empty () ? nullptr : db.c_str ());
  unsigned long flags = CLIENT_COMPRESS | CLIENT_MULTI_STATEMENTS;
  if (mysql_real_connect (handle, host.c_str (),
                          user.c_str (), password.c_str (),
                          dbToUse, port, nullptr, flags)
          == nullptr)
    throw MySqlError (handle);

  connected = true;
}

MYSQL*
Connection::operator* ()
{
  CHECK (connected) << "MySQL is not connected";
  return handle;
}

void
Connection::Execute (const std::string& sql)
{
  CHECK (connected) << "MySQL is not connected";
  if (mysql_real_query (handle, sql.data (), sql.size ()) != 0)
    throw MySqlError (handle);

  /* Process (ignore) all potential row-count indicators.  */
  while (true)
    {
      const int rc = mysql_next_result (handle);
      if (rc == 0)
        continue;
      if (rc == -1)
        break;
      throw MySqlError (handle);
    }
}

void
Connection::SetDefaultDatabase (const std::string& db)
{
  CHECK (connected) << "MySQL is not connected";
  if (mysql_select_db (handle, db.c_str ()) != 0)
    throw MySqlError (handle);
}

} // namespace mypp
