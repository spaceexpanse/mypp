// Copyright (C) 2023 The Xaya developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "url.hpp"

#include "error.hpp"

#include <glog/logging.h>

#include <sstream>

namespace mypp
{

namespace
{

/**
 * Parses an integer as port value.
 */
bool
ParsePort (const std::string& str, unsigned& port)
{
  std::istringstream input(str);
  input >> port;

  /* Serialise it back to a string and ensure it is the same output.  This makes
     sure that there is no whitespace or otherwise ignored/invalid stuff
     in the input.  */
  std::ostringstream output;
  output << port;

  return output.str () == str;
}

} // anonymous namespace

void
ParseUrl (const std::string& url, std::string& host, unsigned& port,
          std::string& user, std::string& password,
          std::string& database, std::string& table)
{
  const std::string prefix = "mysql://";
  if (url.substr (0, prefix.size ()) != prefix)
    throw Error ("URL does not have the expected prefix");

  const size_t atPos = url.find ('@');
  if (atPos == std::string::npos)
    throw Error ("URL does not contain user/password part");

  CHECK_GT (atPos, prefix.size ());
  const std::string userPass = url.substr (prefix.size (),
                                           atPos - prefix.size ());
  const size_t userColonPos = userPass.find (':');
  if (userColonPos == std::string::npos)
    throw Error ("URL does not contain user and password");
  user = userPass.substr (0, userColonPos);
  password = userPass.substr (userColonPos + 1);

  const size_t pathStart = url.find ('/', prefix.size ());
  if (pathStart == std::string::npos)
    throw Error ("URL does not contain path part");

  CHECK_GT (pathStart, atPos);
  const std::string hostPort = url.substr (atPos + 1, pathStart - atPos - 1);
  const size_t hostColonPos = hostPort.find (':');
  if (hostColonPos == std::string::npos)
    throw Error ("URL does not contain host and port");
  host = hostPort.substr (0, hostColonPos);
  if (!ParsePort (hostPort.substr (hostColonPos + 1), port))
    throw Error ("URL contains invalid port");

  const size_t tableSep = url.find ('/', pathStart + 1);
  if (tableSep == std::string::npos)
    {
      database = url.substr (pathStart + 1);
      table = "";
    }
  else
    {
      CHECK_GT (tableSep, pathStart);
      database = url.substr (pathStart + 1, tableSep - pathStart - 1);
      table = url.substr (tableSep + 1);
    }
}

} // namespace mypp
