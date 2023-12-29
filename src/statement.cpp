// Copyright (C) 2023-2024 The Xaya developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "statement.hpp"

#include "error.hpp"

#include <glog/logging.h>

#include <cstring>
#include <limits>
#include <sstream>
#include <type_traits>

namespace mypp
{

namespace
{

/**
 * Constructs an Error instance from the MySQL-reported error data
 * of the given statement.
 */
Error
StmtError (MYSQL_STMT* h)
{
  std::ostringstream out;
  out << "MySQL statement error " << mysql_stmt_errno (h)
      << " / " << mysql_stmt_sqlstate (h);

  return Error (out.str ());
}

/**
 * Returns a pointer to an integer that can hold the "length" of a MYSQL_BIND
 * buffer, using the memory in the location of the larger pointee.
 */
template <typename T>
  auto*
  GetBindLengthPtr (T* mem)
{
  using bndSizeT = std::remove_reference_t <decltype (*MYSQL_BIND::length)>;
  static_assert (sizeof (bndSizeT) <= sizeof (*mem),
                 "memory cannot hold the buffer size type");
  return reinterpret_cast<bndSizeT*> (mem);
}

} // anonymous namespace

Statement::Statement (MYSQL* h)
  : handle(h)
{
  Init ();
}

Statement::~Statement ()
{
  CleanUp ();
}

void
Statement::Init ()
{
  CHECK (stmt == nullptr);
  stmt = mysql_stmt_init (handle);
  CHECK (stmt != nullptr) << "Failed to initialise statement";
  state = State::INITIALISED;
}

void
Statement::CleanUp ()
{
  mysql_stmt_close (stmt);
  stmt = nullptr;
}

MYSQL_STMT*
Statement::operator* ()
{
  CHECK (state >= State::PREPARED) << "Statement is not yet prepared";
  CHECK (state < State::FINISHED) << "Statement is alrady finished";
  return stmt;
}

void
Statement::Prepare (unsigned numParams, const std::string& sql)
{
  if (state == State::FINISHED)
    {
      CleanUp ();
      Init ();
    }

  CHECK (state == State::INITIALISED) << "Statement is already prepared";

  if (mysql_stmt_prepare (stmt, sql.data (), sql.size ()) != 0)
    throw StmtError (stmt);

  state = State::PREPARED;

  params.resize (numParams);
  std::memset (params.data (), 0, numParams * sizeof (MYSQL_BIND));

  intParams.resize (numParams);
  stringParams.resize (numParams);
}

MYSQL_BIND*
Statement::BindRaw (const unsigned num)
{
  CHECK (state == State::PREPARED) << "Statement is not in prepared state";
  CHECK_LT (num, params.size ()) << "Parameter index out of bounds";
  return &params[num];
}

void
Statement::BindNull (const unsigned num)
{
  BindRaw (num)->buffer_type = MYSQL_TYPE_NULL;
}

template <>
  void
  Statement::Bind<int64_t> (const unsigned num, const int64_t& val)
{
  auto* bnd = BindRaw (num);

  intParams[num] = val;

  bnd->buffer_type = MYSQL_TYPE_LONGLONG;
  CHECK_GE (val, std::numeric_limits<decltype (intParams)::value_type>::min ())
      << "Bound integer out of bounds for param type";
  CHECK_LE (val, std::numeric_limits<decltype (intParams)::value_type>::max ())
      << "Bound integer out of bounds for param type";
  bnd->buffer = &intParams[num];
}

template <>
  void
  Statement::Bind<std::string> (const unsigned num, const std::string& val)
{
  BindBlob (num, val);
  BindRaw (num)->buffer_type = MYSQL_TYPE_STRING;
}

void
Statement::BindBlob (const unsigned num, const std::string& val)
{
  auto* bnd = BindRaw (num);

  stringParams[num] = val;
  auto* sizePtr = GetBindLengthPtr (&intParams[num]);
  *sizePtr = val.size ();

  bnd->buffer_type = MYSQL_TYPE_BLOB;
  bnd->buffer = const_cast<char*> (stringParams[num].data ());
  bnd->length = sizePtr;
}

void
Statement::Execute ()
{
  CHECK (state == State::PREPARED) << "Statement is not in prepared state";
  if (!params.empty () && mysql_stmt_bind_param (stmt, params.data ()) != 0)
    throw StmtError (stmt);

  if (mysql_stmt_execute (stmt) != 0)
    throw StmtError (stmt);

  state = State::FINISHED;

  params.clear ();
  intParams.clear ();
  stringParams.clear ();
}

} // namespace mypp
