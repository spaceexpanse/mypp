// Copyright (C) 2023-2024 The Xaya developers
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

/**
 * Parses a "key=value" string as part of the arguments.
 */
bool
ParseOneOption (const std::string& str, std::string& key, std::string& value)
{
  const size_t sep = str.find ('=');
  if (sep == std::string::npos)
    return false;

  key = str.substr (0, sep);
  value = str.substr (sep + 1);

  return true;
}

/**
 * Parses URL options into a map.
 */
bool
ParseOptions (const std::string& str, std::map<std::string, std::string>& res)
{
  res.clear ();
  if (str.empty ())
    return true;

  std::string key;
  std::string value;

  size_t start = 0;
  while (true)
    {
      const size_t sep = str.find ('&', start);
      if (sep == std::string::npos)
        {
          if (!ParseOneOption (str.substr (start), key, value))
            return false;
          res.emplace (key, value);
          return true;
        }

      CHECK_GE (sep, start);
      if (!ParseOneOption (str.substr (start, sep - start), key, value))
        return false;
      res.emplace (key, value);

      start = sep + 1;
    }

  return true;
}

} // anonymous namespace

void
UrlParser::Parse (const std::string& url)
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

  const size_t optStart = url.find ('?', pathStart + 1);
  CHECK_GT (optStart, pathStart);
  std::string path;
  std::string opt;
  if (optStart == std::string::npos)
    {
      path = url.substr (pathStart + 1);
      opt = "";
    }
  else
    {
      path = url.substr (pathStart + 1, optStart - pathStart - 1);
      opt = url.substr (optStart + 1);
    }

  const size_t tableSep = path.find ('/');
  if (tableSep == std::string::npos)
    {
      database = path;
      table = "";
    }
  else
    {
      database = path.substr (0, tableSep);
      table = path.substr (tableSep + 1);
    }

  if (database.empty ())
    throw Error ("URL contains no database");

  if (!ParseOptions (opt, options))
    throw Error ("Invalid options in URL");
}

} // namespace mypp
