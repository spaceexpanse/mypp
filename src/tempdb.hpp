// Copyright (C) 2024 The Xaya developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MYPP_TEMPDB_HPP
#define MYPP_TEMPDB_HPP

#include "connection.hpp"

#include <mysql.h>

#include <string>

namespace mypp
{

/**
 * This class holds a connection to a "temporary database":  The database
 * is created when the class is instantiated, initialised with some stuff
 * (like the user schema), can be used, and is dropped when the instance
 * is destructed.
 *
 * A potential use for this is testing, where such a TempDB can be part of
 * e.g. a unit-test fixture.
 */
class TempDb
{

private:

  /** The underlying database connection.  */
  Connection connection;

  /** Set to true when Initialise() has been called.  */
  bool initialised = false;

  /** The database name.  */
  std::string database;

  /**
   * Internal intialisation routine, to share code between the constructors.
   */
  void Construct (const std::string& host, unsigned port,
                  const std::string& user, const std::string& password,
                  const std::string& db);

protected:

  /**
   * Subclasses can implement this method to do whatever initialisation
   * they want to be done on the database.
   */
  virtual void
  SetUpDatabase ()
  {}

public:

  /**
   * Constructs the instance, connecting to the given server.
   * The database passed (db) must not yet exist on the server, and is what
   * the instance will create (and drop again afterwards).
   */
  explicit TempDb (const std::string& host, unsigned port,
                   const std::string& user, const std::string& password,
                   const std::string& db);

  /**
   * Constructs the instance based on a URL.
   */
  explicit TempDb (const std::string& url);

  /**
   * Drops the database again on the server and closes the connection.
   */
  virtual ~TempDb ();

  /**
   * Runs initialisation, which creates the temporary database, and then
   * performs the subclass-specific database setup.
   */
  void Initialise ();

  /**
   * Returns the underlying connection.
   */
  Connection&
  Get ()
  {
    return connection;
  }

  /**
   * Returns the underlying MYSQL handle.
   */
  MYSQL*
  GetMySql ()
  {
    return *connection;
  }

};

} // namespace mypp

#endif // MYPP_TEMPDB_HPP
