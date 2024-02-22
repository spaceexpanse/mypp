// Copyright (C) 2023-2024 The Xaya developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MYPP_URL_HPP
#define MYPP_URL_HPP

#include <map>
#include <string>

namespace mypp
{

/**
 * Helper class for parsing URLs that specify connections for the
 * MySQL server.  It has this form:
 *
 *    mysql://user:password@host:port/database[/table][?opt1=val1&opt2=val2]
 *
 * Arbitrary options can be parsed and added to a map.  They can be used
 * for instance to specify TLS client certificate settings for the connection.
 */
class UrlParser
{

private:

  std::string host;
  unsigned port;

  std::string user;
  std::string password;

  std::string database;
  std::string table;

  std::map<std::string, std::string> options;

public:

  UrlParser () = default;

  /**
   * Parses a URL, filling in the internal fields.  Throws mypp::Error if
   * something is invalid.
   */
  void Parse (const std::string& url);

  const std::string&
  GetHost () const
  {
    return host;
  }

  unsigned
  GetPort () const
  {
    return port;
  }

  const std::string&
  GetUser () const
  {
    return user;
  }

  const std::string&
  GetPassword () const
  {
    return password;
  }

  const std::string&
  GetDatabase () const
  {
    return database;
  }

  bool
  HasTable () const
  {
    return !table.empty ();
  }

  const std::string&
  GetTable () const
  {
    return table;
  }

  bool
  HasOption (const std::string& name) const
  {
    return options.count (name) > 0;
  }

  std::string
  GetOption (const std::string& name) const
  {
    const auto mit = options.find (name);
    if (mit == options.end ())
      return "";
    return mit->second;
  }

};

} // namespace mypp

#endif // MYPP_URL_HPP
