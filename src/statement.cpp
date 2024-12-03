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
  if (resMeta != nullptr)
    {
      mysql_free_result (resMeta);
      resMeta = nullptr;
    }

  mysql_stmt_close (stmt);
  stmt = nullptr;
}

void
Statement::ResizeParams (const size_t num)
{
  params.resize (num);
  std::memset (params.data (), 0, num * sizeof (MYSQL_BIND));

  intParams.resize (num);
  stringParams.resize (num);
  isNull.resize (num);
}

MYSQL_STMT*
Statement::operator* ()
{
  CHECK (state >= State::PREPARED) << "Statement is not yet prepared";
  CHECK (state < State::FINISHED) << "Statement is alrady finished";
  return stmt;
}

void
Statement::Prepare (unsigned n, const std::string& sql)
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
  numParams = n;
  ResizeParams (numParams);
}

void
Statement::Reset ()
{
  CHECK (state != State::INITIALISED) << "Statement is not prepared yet";

  if (resMeta != nullptr)
    {
      mysql_free_result (resMeta);
      resMeta = nullptr;
    }

  if (mysql_stmt_reset (stmt) != 0)
    throw StmtError (stmt);

  state = State::PREPARED;
  ResizeParams (numParams);
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
  Statement::Bind<bool> (const unsigned num, const bool& val)
{
  Bind<int64_t> (num, val);
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

void
Statement::Query ()
{
  Execute ();
  state = State::QUERIED;

  my_bool update = 1;
  CHECK_EQ (mysql_stmt_attr_set (stmt, STMT_ATTR_UPDATE_MAX_LENGTH, &update),
            0);

  if (mysql_stmt_store_result (stmt) != 0)
    throw StmtError (stmt);

  resMeta = mysql_stmt_result_metadata (stmt);
  if (resMeta == nullptr)
    throw Error ("No result metadata returned for statement query");

  const unsigned numFields = mysql_num_fields (resMeta);
  ResizeParams (numFields);

  /* We process all fields.  While doing so, we store their names, so the user
     can look up result fields by name (instead of index).  And we also
     check their type, and apply an appropriate bind for them to local
     memory (in the instance).  This abstracts the binding part away from
     the caller, and they can just step through the result and get the
     values via function calls.  */
  for (unsigned i = 0; i < numFields; ++i)
    {
      const auto* field = mysql_fetch_field_direct (resMeta, i);
      resFields.push_back (field);
      columnsByName.emplace (field->name, i);

      auto* bnd = &params[i];
      bnd->is_null = &isNull[i];
      switch (field->type)
        {
        case MYSQL_TYPE_TINY:
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_LONGLONG:
          bnd->buffer_type = MYSQL_TYPE_LONGLONG;
          bnd->buffer = &intParams[i];
          break;

        case MYSQL_TYPE_STRING:
        case MYSQL_TYPE_VAR_STRING:
        case MYSQL_TYPE_TINY_BLOB:
        case MYSQL_TYPE_BLOB:
        case MYSQL_TYPE_MEDIUM_BLOB:
        case MYSQL_TYPE_LONG_BLOB:
          stringParams[i].resize (field->max_length);
          bnd->buffer_type = MYSQL_TYPE_LONG_BLOB;
          bnd->buffer = const_cast<char*> (stringParams[i].data ());
          bnd->buffer_length = stringParams[i].size ();
          bnd->length = GetBindLengthPtr (&intParams[i]);
          break;

        default:
          LOG (FATAL)
              << "Output type of " << field->type << " is not yet implemented";
        }
    }

  if (!params.empty () && mysql_stmt_bind_result (stmt, params.data ()) != 0)
    throw StmtError (stmt);
}

bool
Statement::Fetch ()
{
  CHECK (state == State::QUERIED) << "Statement is not in queried state";

  const int res = mysql_stmt_fetch (stmt);
  if (res == MYSQL_NO_DATA)
    {
      state = State::FINISHED;
      return false;
    }

  /* Truncation should be impossible since we set the buffer sizes
     explicitly large enough.  */
  CHECK_NE (res, MYSQL_DATA_TRUNCATED) << "MySQL data truncated";

  if (res != 0)
    throw StmtError (stmt);

  return true;
}

unsigned
Statement::GetIndex (const std::string& col) const
{
  CHECK (state == State::QUERIED) << "Statement is not in queried state";
  auto mit = columnsByName.find (col);
  CHECK (mit != columnsByName.end ())
      << "Column '" << col << "' is not in the result set";
  return mit->second;
}

bool
Statement::IsNull (const std::string& col) const
{
  return isNull[GetIndex (col)];
}

template <>
  int64_t
  Statement::Get<int64_t> (const std::string& col) const
{
  const unsigned ind = GetIndex (col);
  CHECK (!isNull[ind]) << "Column '" << col << "' is null";
  CHECK_EQ (params[ind].buffer_type, MYSQL_TYPE_LONGLONG)
      << "Column '" << col << "' is not of integer type";

  const auto value = intParams[ind];
  CHECK_GE (value, std::numeric_limits<int64_t>::min ())
      << "Value of '" << col << "' is out of bounds for int64_t";
  CHECK_LE (value, std::numeric_limits<int64_t>::max ())
      << "Value of '" << col << "' is out of bounds for int64_t";
  return value;
}

template <>
  bool
  Statement::Get<bool> (const std::string& col) const
{
  const auto val = Get<int64_t> (col);
  return val != 0;
}

template <>
  std::string
  Statement::Get<std::string> (const std::string& col) const
{
  const unsigned ind = GetIndex (col);
  CHECK (!isNull[ind]) << "Column '" << col << "' is null";
  CHECK_EQ (params[ind].buffer_type, MYSQL_TYPE_LONG_BLOB)
      << "Column '" << col << "' is not of string type";

  return stringParams[ind].substr (0, intParams[ind]);
}

std::string
Statement::GetBlob (const std::string& col) const
{
  return Get<std::string> (col);
}

} // namespace mypp
