// Copyright (C) 2024 The Xaya developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tempdb.hpp"

#include "error.hpp"
#include "statement.hpp"
#include "url.hpp"

#include <glog/logging.h>

namespace mypp
{

TempDb::TempDb (const std::string& host, const unsigned port,
                const std::string& user, const std::string& password,
                const std::string& db)
{
  Construct (host, port, user, password, db);
}

TempDb::TempDb (const std::string& url)
{
  std::string host, user, password, db, table;
  unsigned port;

  ParseUrl (url, host, port, user, password, db, table);
  CHECK (table.empty ()) << "Explicit table passed to TempDb";

  Construct (host, port, user, password, db);
}

TempDb::~TempDb ()
{
  if (!initialised)
    return;

  try
    {
      Get ().Execute ("DROP DATABASE `" + database + "`");
    }
  catch (const Error& exc)
    {
      LOG (ERROR) << "Error dropping temporary database: " << exc.what ();
      /* Don't rethrow, so that an error during cleanup does not mess up
         the entire destruction flow.  */
    }
}

void
TempDb::Construct (const std::string& host, const unsigned port,
                   const std::string& user, const std::string& password,
                   const std::string& db)
{
  database = db;
  connection.Connect (host, port, user, password, "");
}

void
TempDb::Initialise ()
{
  Get ().Execute ("CREATE DATABASE `" + database + "`");
  initialised = true;

  connection.SetDefaultDatabase (database);
  SetUpDatabase ();
}

} // namespace mypp
