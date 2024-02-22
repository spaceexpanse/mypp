// Copyright (C) 2023-2024 The Xaya developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MYPP_CONNECTION_HPP
#define MYPP_CONNECTION_HPP

#include <mysql.h>

#include <string>

namespace mypp
{

/**
 * RAII wrapper around a MySQL / MariaDB database connection.  This contains
 * a MYSQL* handle (which can be directly accessed by users of the library as
 * well if needed), and manages its initialisation and destruction via RAII.
 */
class Connection
{

private:

  /** The underlying MYSQL handle.  */
  MYSQL* handle;

  /** Set to true if a connection is established.  */
  bool connected = false;

public:

  /**
   * Initialises the connection (underlying handle), but without
   * yet connecting to an actual database.
   */
  Connection ();

  /**
   * Closes down the connection, freeing all MySQL resources as needed.
   */
  ~Connection ();

  /**
   * Sets config for using a client certificate.  Must be called before
   * Connect is called.  The arguments must be paths to the corresponding
   * files holding the CA certificate, public certificate and private key.
   */
  void UseClientCertificate (const std::string& ca, const std::string& cert,
                             const std::string& key);

  /**
   * Establishes a connection to a MySQL database.  This must only be called
   * once (not if already connected).  If db is the empty string, then no
   * default database is set.
   */
  void Connect (const std::string& host, unsigned port,
                const std::string& user, const std::string& password,
                const std::string& db);

  /**
   * Returns true if the connection is ready.
   */
  inline operator bool () const
  {
    return connected;
  }

  /**
   * Returns the underlying MySQL handle.  Must only be used if connected.
   */
  MYSQL* operator* ();

  /**
   * Executes one or multiple queries specified by string.  They queries
   * are supposed to not return results (SELECT) but instead just
   * update the database state.
   */
  void Execute (const std::string& sql);

  /**
   * Sets the default database to use on the connection.
   */
  void SetDefaultDatabase (const std::string& db);

};

} // namespace mypp

#endif // MYPP_CONNECTION_HPP
