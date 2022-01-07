/*
 * Copyright 2021 tsurugi project.
 *
 * Licensed under the Apache License, version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "test/helper/foreign_table_helper.h"

#include <gtest/gtest.h>

#include <boost/format.hpp>
#include <memory>
#include <string>

#include "manager/metadata/dao/common/config.h"
#include "manager/metadata/dao/postgresql/dbc_utils.h"

namespace {

manager::metadata::db::postgresql::ConnectionSPtr connection;

}  // namespace

namespace manager::metadata::testing {

using db::postgresql::DbcUtils;

/**
 * @brief create a table metadata for testing.
 * @param (table_name)  [in]   table name.
 * @param (role_name)   [in]   role name.
 * @param (privileges)  [in]   privileges.
 * @return table_id.
 */
ObjectIdType ForeignTableHelper::create_table(std::string_view table_name,
                                              std::string_view role_name,
                                              std::string_view privileges) {
  ObjectIdType table_id;

  boost::format statement;
  PGresult* res = nullptr;

  // db connection.
  db_connection();

  // create dummy data for TABLE.
  statement =
      boost::format("CREATE TABLE tsurugi_catalog.%s (id bigint, name text)") %
      table_name;
  res = PQexec(connection.get(), statement.str().c_str());
  PQclear(res);

  // set dummy data for privileges.
  if (!privileges.empty()) {
    statement = boost::format("GRANT %s ON tsurugi_catalog.%s TO %s") %
                privileges % table_name % role_name;
  } else {
    statement = boost::format("REVOKE ALL ON tsurugi_catalog.%s TO %s") %
                table_name % role_name;
  }
  res = PQexec(connection.get(), statement.str().c_str());
  PQclear(res);

  // get dummy data for TABLE.
  statement =
      boost::format("SELECT oid FROM pg_class WHERE relname='%s'") % table_name;
  res = PQexec(connection.get(), statement.str().c_str());
  DbcUtils::str_to_integral(PQgetvalue(res, 0, 0), table_id);
  PQclear(res);

  return table_id;
}

/**
 * @brief remove a table for testing.
 * @param (table_name)  [in]   table name.
 */
void ForeignTableHelper::drop_table(std::string_view table_name) {
  // db connection.
  db_connection();

  // remove dummy data for TABLE.
  boost::format statement =
      boost::format("DROP TABLE tsurugi_catalog.%s") % table_name;
  PGresult* res = PQexec(connection.get(), statement.str().c_str());
  PQclear(res);
}

/**
 * @brief insert a foreign table for testing.
 * @param (table_name)  [in]   table name.
 * @return table_id.
 */
ObjectIdType ForeignTableHelper::insert_foreign_table(
    std::string_view table_name) {
  ObjectIdType table_id;

  std::string ft_statement_sub =
      "SELECT CAST(MAX(ftrelid) AS INTEGER) num FROM pg_foreign_table";
  boost::format statement =
      boost::format(
          "INSERT into pg_foreign_table VALUES"
          " ((%s) + 1, 1"
          " , '{schema_name=tsurugi_catalog,table_name=%s}')"
          " RETURNING ftrelid") %
      ft_statement_sub % table_name;
  PGresult* res = PQexec(connection.get(), statement.str().c_str());
  DbcUtils::str_to_integral(PQgetvalue(res, 0, 0), table_id);
  PQclear(res);

  return table_id;
}

/**
 * @brief discard the data for testing.
 * @param (foreign_table_id)  [in]   foreign table id.
 */
void ForeignTableHelper::delete_foreign_table(ObjectIdType foreign_table_id) {
  // db connection.
  db_connection();

  // remove dummy data for pg_foreign_table.
  boost::format statement =
      boost::format("DELETE FROM pg_foreign_table where ftrelid = %s") %
      foreign_table_id;
  PGresult* res = PQexec(connection.get(), statement.str().c_str());
  PQclear(res);
}

/**
 * @brief Connect to the database.
 */
void ForeignTableHelper::db_connection() {
  if (!DbcUtils::is_open(connection)) {
    // db connection.
    PGconn* pgconn = PQconnectdb(db::Config::get_connection_string().c_str());
    connection = DbcUtils::make_connection_sptr(pgconn);

    ASSERT_TRUE(DbcUtils::is_open(connection));
  }
}

}  // namespace manager::metadata::testing
