// Copyright (C) 2023 The Xaya developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MYPP_URL_HPP
#define MYPP_URL_HPP

#include <string>

namespace mypp
{

/**
 * Tries to parse a URL for a MySQL database connection into its parts.
 * Throws a mypp::Error if something goes wrong.
 *
 * The URL has this form:
 *
 *    mysql://user:password@host:port/database[/table]
 *
 * If the table is not specified, it will be set as the empty string.
 */
void ParseUrl (const std::string& url, std::string& host, unsigned& port,
               std::string& user, std::string& password,
               std::string& database, std::string& table);

} // namespace mypp

#endif // MYPP_URL_HPP
