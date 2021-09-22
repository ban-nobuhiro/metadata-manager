/*
 * Copyright 2020 tsurugi project.
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
#include "manager/metadata/provider/tables_provider.h"

#include <boost/foreach.hpp>

#include "manager/metadata/datatypes.h"
#include "manager/metadata/provider/datatypes_provider.h"
#include "manager/metadata/tables.h"

// =============================================================================
namespace manager::metadata::db {

using boost::property_tree::ptree;
using manager::metadata::ErrorCode;

/**
 * @brief Initialize and prepare to access the metadata repository.
 * @return ErrorCode::OK if success, otherwise an error code.
 */
ErrorCode TablesProvider::init() {
  ErrorCode error = ErrorCode::UNKNOWN;
  std::shared_ptr<GenericDAO> gdao = nullptr;

  if (tables_dao_ != nullptr) {
    // Instance of the TablesDAO class has already been obtained.
    error = ErrorCode::OK;
  } else {
    // Get an instance of the TablesDAO class.
    error = session_manager_->get_dao(GenericDAO::TableName::TABLES, gdao);
    if (error != ErrorCode::OK) {
      return error;
    }
    // Set TablesDAO instance.
    tables_dao_ = std::static_pointer_cast<TablesDAO>(gdao);
  }

  if (columns_dao_ != nullptr) {
    // Instance of the ColumnsDAO class has already been obtained.
    error = ErrorCode::OK;
  } else {
    // Get an instance of the ColumnsDAO class.
    error = session_manager_->get_dao(GenericDAO::TableName::COLUMNS, gdao);
    if (error != ErrorCode::OK) {
      return error;
    }
    // Set ColumnsDAO instance.
    columns_dao_ = std::static_pointer_cast<ColumnsDAO>(gdao);
  }

  return error;
}

/**
 * @brief Add table metadata to table metadata repository.
 * @param (object)     [in]  table metadata to add.
 * @param (table_id)   [out] ID of the added table metadata.
 * @return ErrorCode::OK if success, otherwise an error code.
 */
ErrorCode TablesProvider::add_table_metadata(ptree& object,
                                             ObjectIdType& table_id) {
  ErrorCode error = ErrorCode::UNKNOWN;

  // Initialization
  error = init();
  if (error != ErrorCode::OK) {
    return error;
  }

  // Parameter value check
  error = fill_parameters(object);
  if (error != ErrorCode::OK) {
    return error;
  }

  error = session_manager_->start_transaction();
  if (error != ErrorCode::OK) {
    return error;
  }

  // Add table metadata object to table metadata table.
  error = tables_dao_->insert_table_metadata(object, table_id);
  if (error != ErrorCode::OK) {
    ErrorCode rollback_result = session_manager_->rollback();
    if (rollback_result != ErrorCode::OK) {
      return rollback_result;
    }

    // Check if it already exists.
    ptree res_ptree;
    boost::optional<std::string> name =
        object.get_optional<std::string>(Tables::NAME);
    ErrorCode rselect_result =
        tables_dao_->select_table_metadata(Tables::NAME, name.get(), res_ptree);
    if (rselect_result == ErrorCode::OK) {
      boost::optional<std::int64_t> id =
          res_ptree.get_optional<std::int64_t>(Tables::ID);
      if (id) {
        error = ErrorCode::TABLE_NAME_ALREADY_EXISTS;
      }
    }

    return error;
  }

  // Add column metadata object to column metadata table.
  BOOST_FOREACH (const ptree::value_type& node,
                 object.get_child(Tables::COLUMNS_NODE)) {
    ptree column = node.second;
    error = columns_dao_->insert_one_column_metadata(table_id, column);
    if (error != ErrorCode::OK) {
      ErrorCode rollback_result = session_manager_->rollback();
      if (rollback_result != ErrorCode::OK) {
        error = rollback_result;
      }
      return error;
    }
  }

  error = session_manager_->commit();

  return error;
}

/**
 * @brief Gets one table metadata object from the table metadata repository,
 *   where key = value.
 * @param (key)     [in]  key of table metadata object.
 * @param (value)   [in]  value of table metadata object.
 * @param (object)  [out] one table metadata object to get,
 *   where key = value.
 * @return ErrorCode::OK if success, otherwise an error code.
 */
ErrorCode TablesProvider::get_table_metadata(std::string_view key,
                                             std::string_view value,
                                             ptree& object) {
  ErrorCode error = ErrorCode::UNKNOWN;

  // Initialization
  error = init();
  if (error != ErrorCode::OK) {
    return error;
  }

  error = tables_dao_->select_table_metadata(key, value, object);
  if (error != ErrorCode::OK) {
    return error;
  }

  std::string object_id = "";
  if (key == Tables::ID) {
    error = get_column_metadata(value, object);
  } else if (key == Tables::NAME) {
    error = get_all_column_metadata(object);
  } else {
    error = ErrorCode::INVALID_PARAMETER;
  }
  return error;
}

/**
 * @brief Gets all table metadata object from the table metadata repository.
 * @param (container)   [out] table metadata object to get.
 * @return ErrorCode::OK if success, otherwise an error code.
 */
ErrorCode TablesProvider::get_table_metadata(std::vector<ptree>& container) {
  ErrorCode error = ErrorCode::UNKNOWN;

  // Initialization
  error = init();
  if (error != ErrorCode::OK) {
    return error;
  }

  error = tables_dao_->select_table_metadata(container);
  if (error != ErrorCode::OK) {
    return error;
  }

  std::string object_id = "";
  BOOST_FOREACH (auto& table_object, container) {
    error = get_all_column_metadata(table_object);
    if (error != ErrorCode::OK) {
      break;
    }
  }
  return error;
}

/**
 * @brief Gets one table statistic from the table metadata repository,
 *   where key = value.
 * @param (key)      [in]  key of table metadata object.
 * @param (value)    [in]  value of table metadata object.
 * @param (object)   [out] one table metadata object to get,
 *   where key = value.
 * @return ErrorCode::OK if success, otherwise an error code.
 */
ErrorCode TablesProvider::get_table_statistic(std::string_view key,
                                              std::string_view value,
                                              ptree& object) {
  ErrorCode error = ErrorCode::UNKNOWN;

  // Initialization
  error = init();
  if (error != ErrorCode::OK) {
    return error;
  }

  error = tables_dao_->select_table_metadata(key, value, object);
  if (error != ErrorCode::OK) {
    return error;
  }

  return error;
}

/**
 * @brief Updates the table metadata table with the specified table statistics.
 * @param (object)    [in]  Table statistic object.
 * @param (table_id)  [out] ID of the added table metadata.
 * @return ErrorCode::OK if success, otherwise an error code.
 */
ErrorCode TablesProvider::set_table_statistic(ptree& object,
                                              ObjectIdType& table_id) {
  ErrorCode error = ErrorCode::UNKNOWN;

  // Initialization
  error = init();
  if (error != ErrorCode::OK) {
    return error;
  }

  // id
  boost::optional<std::string> optional_id =
      object.get_optional<std::string>(Tables::ID);
  // name
  boost::optional<std::string> optional_name =
      object.get_optional<std::string>(Tables::NAME);
  // tuples
  boost::optional<float> optional_tuples =
      object.get_optional<float>(Tables::TUPLES);

  // Parameter value check
  if ((!optional_id && !optional_name) || (!optional_tuples)) {
    error = ErrorCode::INVALID_PARAMETER;
    return error;
  }

  // Set the key items and values to be updated.
  std::string key;
  std::string value;
  if (optional_id) {
    key = Tables::ID;
    value = optional_id.get();
  } else {
    key = Tables::NAME;
    value = optional_name.get();
  }

  // Set the update value.
  float tuples = optional_tuples.get();

  error = session_manager_->start_transaction();
  if (error != ErrorCode::OK) {
    return error;
  }

  // Update table statistics to table metadata table.
  error = tables_dao_->update_reltuples(tuples, key, value, table_id);

  if (error == ErrorCode::OK) {
    error = session_manager_->commit();
  } else {
    ErrorCode rollback_result = session_manager_->rollback();
    if (rollback_result != ErrorCode::OK) {
      error = rollback_result;
    }
  }

  return error;
}

/**
 * @brief Remove all metadata-object based on the given table name
 *   (table metadata, column metadata and column statistics)
 *   from metadata-repositorys
 *   (the table metadata repository, the column metadata repository and the
 *   column statistics repository).
 * @param (key)       [in]  key of table metadata object.
 * @param (value)     [in]  value of table metadata object.
 * @param (table_id)  [out] ID of the removed table metadata.
 * @return ErrorCode::OK if success, otherwise an error code.
 */
ErrorCode TablesProvider::remove_table_metadata(std::string_view key,
                                                std::string_view value,
                                                ObjectIdType& table_id) {
  ErrorCode error = ErrorCode::UNKNOWN;

  // Initialization
  error = init();
  if (error != ErrorCode::OK) {
    return error;
  }

  error = session_manager_->start_transaction();
  if (error != ErrorCode::OK) {
    return error;
  }

  error = tables_dao_->delete_table_metadata(key, value, table_id);
  if (error != ErrorCode::OK) {
    ErrorCode rollback_result = session_manager_->rollback();
    if (rollback_result != ErrorCode::OK) {
      return rollback_result;
    }
    return error;
  }

  error = columns_dao_->delete_column_metadata(Tables::Column::TABLE_ID,
                                               std::to_string(table_id));
  if (error == ErrorCode::OK) {
    error = session_manager_->commit();
  } else {
    ErrorCode rollback_result = session_manager_->rollback();
    if (rollback_result != ErrorCode::OK) {
      error = rollback_result;
    }
  }

  return error;
}

//_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
// Private method area

/**
 * @brief Get column metadata-object based on the given table id.
 * @param (tables)  [out] table metadata-object.
 * @return ErrorCode::OK if success, otherwise an error code.
 */
ErrorCode TablesProvider::get_all_column_metadata(ptree& tables) const {
  ErrorCode error = ErrorCode::UNKNOWN;

  error = ErrorCode::OK;
  BOOST_FOREACH (ptree::value_type& node, tables) {
    ptree& table = node.second;

    if (table.empty()) {
      boost::optional<std::string> o_table_id =
          tables.get_optional<std::string>(Tables::ID);
      if (!o_table_id) {
        error = ErrorCode::INTERNAL_ERROR;
        return error;
      }

      error = get_column_metadata(o_table_id.get(), tables);
      break;
    } else {
      boost::optional<std::string> o_table_id =
          table.get_optional<std::string>(Tables::ID);
      if (!o_table_id) {
        error = ErrorCode::INTERNAL_ERROR;
        break;
      }

      error = get_column_metadata(o_table_id.get(), table);
      if (error != ErrorCode::OK) {
        break;
      }
    }
  }

  return error;
}

/**
 * @brief Get column metadata-object based on the given table id.
 * @param (table_id) [in]  table id.
 * @param (tables)   [out] table metadata-object
 *   with the specified table id.
 * @return ErrorCode::OK if success, otherwise an error code.
 */
ErrorCode TablesProvider::get_column_metadata(std::string_view table_id,
                                              ptree& tables) const {
  ErrorCode error = ErrorCode::UNKNOWN;
  ptree columns;

  error = columns_dao_->select_column_metadata(Tables::Column::TABLE_ID,
                                               table_id, columns);

  if ((error == ErrorCode::OK) || (error == ErrorCode::INVALID_PARAMETER)) {
    tables.add_child(Tables::COLUMNS_NODE, columns);
    error = ErrorCode::OK;
  }

  return error;
}

/**
 * @brief Checks if the parameters are correct.
 * @param (table)  [in]  metadata-object
 * @return ErrorCode::OK if success, otherwise an error code.
 */
ErrorCode TablesProvider::fill_parameters(ptree& table) const {
  ErrorCode error = ErrorCode::UNKNOWN;

  boost::optional<std::string> name =
      table.get_optional<std::string>(Metadata::NAME);
  if (!name || name.get().empty()) {
    error = ErrorCode::INVALID_PARAMETER;
    return error;
  }

  // DataTypes check provider.
  db::DataTypesProvider data_type_provider;
  data_type_provider.init();

  //
  // column metadata
  //
  error = ErrorCode::OK;
  BOOST_FOREACH (ptree::value_type& node,
                 table.get_child(Tables::COLUMNS_NODE)) {
    ptree& column = node.second;

    // name
    boost::optional<std::string> name =
        column.get_optional<std::string>(Tables::Column::NAME);
    if (!name || (name.get().empty())) {
      error = ErrorCode::INVALID_PARAMETER;
      break;
    }

    // ordinal position
    boost::optional<std::int64_t> ordinal_position =
        column.get_optional<std::int64_t>(Tables::Column::ORDINAL_POSITION);
    if (!ordinal_position || (ordinal_position.get() <= 0)) {
      error = ErrorCode::INVALID_PARAMETER;
      break;
    }

    // datatype id
    boost::optional<ObjectIdType> datatype_id =
        column.get_optional<ObjectIdType>(Tables::Column::DATA_TYPE_ID);
    if (!datatype_id || (datatype_id.get() < 0)) {
      error = ErrorCode::INVALID_PARAMETER;
      break;
    }
    ptree object;
    error = data_type_provider.get_datatype_metadata(
        DataTypes::ID, std::to_string(datatype_id.get()), object);
    if (error != ErrorCode::OK) {
      error = ErrorCode::INVALID_PARAMETER;
      break;
    }

    // nullable
    boost::optional<std::string> nullable =
        column.get_optional<std::string>(Tables::Column::NULLABLE);
    if (!nullable || (nullable.get().empty())) {
      error = ErrorCode::INVALID_PARAMETER;
      break;
    }
  }

  return error;
}

}  // namespace manager::metadata::db
