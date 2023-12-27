// Copyright (C) 2023 The Xaya developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MYPP_ERROR_HPP
#define MYPP_ERROR_HPP

#include <stdexcept>

namespace mypp
{

/**
 * Base exception thrown when something goes wrong in a MyPP call.
 */
class Error : public std::runtime_error
{

public:

  explicit Error (const std::string& msg)
    : std::runtime_error (msg)
  {}

};

} // namespace mypp

#endif // MYPP_ERROR_HPP
