# MyPP

MyPP is a simple C++ wrapper around the
[MariaDB C connector](https://mariadb.com/kb/en/mariadb-connector-c/).  It
allows connecting to a MySQL or MariaDB database from C++, using concepts
like RAII and templates to make the interaction easier (and safer) than
using the C connector directly.

The library can be used general purpose, but is built specifically
for the needs of projects at Xaya.  Thus some very basic features may be
missing, but feature and pull requests are welcome.
