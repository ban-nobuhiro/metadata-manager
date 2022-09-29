/*
 * Copyright 2022 tsurugi project.
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
#include "manager/metadata/dao/json/index_dao_json.h"

#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>

#include "manager/metadata/common/config.h"
#include "manager/metadata/common/message.h"
#include "manager/metadata/dao/json/object_id_json.h"
#include "manager/metadata/error_code.h"
#include "manager/metadata/helper/logging_helper.h"
#include "manager/metadata/metadata.h"
#include "manager/metadata/indexes.h"

// =============================================================================

namespace {
using manager::metadata::db::ObjectIdGenerator;
std::unique_ptr<ObjectIdGenerator> oid_generator = nullptr;
}

namespace manager::metadata::db {

using boost::property_tree::ptree;

/**
 * @brief Find metadata object from metadata table.
 * @param objects [in]  container of JSON object.
 * @param key     [in]  key. column name of a table metadata table.
 * e.g. object ID, object name.
 * @param value   [in]  value to be filtered.
 * @param object  [out] metadata-object with the specified name.
 * @return ErrorCode::OK if success, otherwise an error code.
 */
manager::metadata::ErrorCode IndexDaoJson::find_metadata_object(
    const boost::property_tree::ptree& objects, 
    std::string_view key, std::string_view value,
    boost::property_tree::ptree& object) const {

  ErrorCode error = ErrorCode::NOT_FOUND;
  BOOST_FOREACH(const auto& node,
                objects.get_child(IndexDaoJson::INDEXES_ROOT)) {
    const auto& temp_obj = node.second;
    auto temp_value = temp_obj.get_optional<std::string>(key.data());
    if (!temp_value) {
      error = ErrorCode::INTERNAL_ERROR;
      break;
    }
    if (temp_value.get() == value) {
      // find the object.
      object = temp_obj;
      error = ErrorCode::OK;
      break;
    }
  }

  return error;
}

/**
 * @brief Delete a metadata object from a metadata table file.
 * @param (container)    [in/out] metadata container.
 * @param (object_key)   [in]     key. column name of a table metadata table.
 * @param (object_value) [in]     value to be filtered.
 * @param (table_id)     [out]    table id of the row deleted.
 * @retval ErrorCode::OK if success.
 * @retval ErrorCode::ID_NOT_FOUND if the table id does not exist.
 * @retval ErrorCode::NAME_NOT_FOUND if the table name does not exist.
 * @retval otherwise an error code.
 */
ErrorCode IndexDaoJson::delete_metadata_object(
    boost::property_tree::ptree& objects, 
    std::string_view key, std::string_view value, 
    ObjectIdType& object_id) const {
  
  ErrorCode error = ErrorCode::UNKNOWN;

  // Initialize the error code.
  if (key == Object::ID) {
    error = ErrorCode::ID_NOT_FOUND;
  } else if (key == Object::NAME) {
    error = ErrorCode::NAME_NOT_FOUND;
  } else {
    error = ErrorCode::NOT_FOUND;
  }

  ptree& node = objects.get_child(INDEXES_ROOT);
  for (ptree::iterator ite = node.begin(); ite != node.end();) {
    const auto& temp_obj = ite->second;
    auto id = temp_obj.get_optional<std::string>(Object::ID);
    if (!id) {
      error = ErrorCode::INTERNAL_ERROR;
      break;
    }

    if (key == Object::ID) {
      if (id.get() == value) {
        // find the object.
        ite = node.erase(ite);
        object_id = std::stoul(id.get());     
        error = ErrorCode::OK;
        break;
      }
    } else if (key == Object::NAME) {
      auto name = temp_obj.get_optional<std::string>(Object::NAME);
      if (name && (name.get() == value)) {
        // find the object.
        ite = node.erase(ite);
        object_id = std::stoul(id.get());
        error = ErrorCode::OK;
        break;
      }
    } else {
      // Unsupported keys.
      error = ErrorCode::NOT_SUPPORTED;
      break;
    }
    ++ite;
  }

  return error;
}

/**
 * @brief Prepare to access the JSON file of table metadata.
 * @param none.
 * @return ErrorCode::OK if success, otherwise an error code.
 */
ErrorCode IndexDaoJson::prepare() {
  ErrorCode error = ErrorCode::UNKNOWN;

  // Filename of the table metadata.
  boost::format file_path = boost::format("%s/%s.json") %
                            Config::get_storage_dir_path() %
                            std::string(IndexDaoJson::INDEX_METADATA_FILE);

  // Connect to the table metadata file.
  error = session_->connect(file_path.str(), 
                            IndexDaoJson::INDEXES_ROOT);

  // Create Object ID generator.
  oid_generator = std::make_unique<ObjectIdGenerator>();

  return error;
}

/**
 * @brief Check the object which has specified name exists
 * in the metadata table.
 * @param name  [in] object name.
 * @return  true if it exists, otherwise false.
 */
bool IndexDaoJson::exists(std::string_view name) const {

  auto exists = false;

  ErrorCode error = session_->load_contents();
  if (error != ErrorCode::OK) {
    return exists;
  }

  ptree* contents = session_->get_contents();
  BOOST_FOREACH(const auto& node,
                contents->get_child(IndexDaoJson::INDEXES_ROOT)) {
    const auto& obj = node.second;
    auto value = obj.get_optional<std::string>(Object::NAME);
    if (value && value.get() == name) {
      exists = true;
      break;
    }
  }

  return exists;
}

/**
 * @brief Check the object which has specified name exists
 * in the metadata table.
 * @param name  [in] object name.
 * @return  true if it exists, otherwise false.
 */
bool IndexDaoJson::exists(const boost::property_tree::ptree& object) const {

  auto exists = false;

  auto name = object.get_optional<std::string>(Object::NAME);
  if (name) {
    exists = this->exists(name.get());
  }

  return exists;
}

/**
 * @brief Insert a metadata object into the metadata table..
 * @param object    [inout] a metadata object to insert.
 * @param object_id [out] object ID.
 * @return  If success ErrorCode::OK, otherwise error code.
 * @note  If success, metadata object is added management metadata.
 * e.g. format version, generation, etc...
 */
manager::metadata::ErrorCode IndexDaoJson::insert(
    const boost::property_tree::ptree& object,
    ObjectIdType& object_id) const
{
  ErrorCode error = ErrorCode::UNKNOWN;

  // Check if the object is alredy exists.
  if (exists(object)) {
    return ErrorCode::ALREADY_EXISTS;
  }

  error = session_->load_contents();
  if (error != ErrorCode::OK) {
    return error;
  }

  auto opt_name = object.get_optional<std::string>(Index::NAME);
  ptree* contents = session_->get_contents();

  ptree temp_obj = object;
  // Generate management metadata.
  temp_obj.put<int64_t>(Index::FORMAT_VERSION, 1);
  temp_obj.put<int64_t>(Index::GENERATION, 1);
  object_id = oid_generator->generate(OID_KEY_NAME_TABLE);
  temp_obj.put<ObjectIdType>(Index::ID, object_id);

  ptree root = contents->get_child(IndexDaoJson::INDEXES_ROOT);
  root.push_back(std::make_pair("", object));
  contents->put_child(IndexDaoJson::INDEXES_ROOT, root);

  return  ErrorCode::OK;
}

/**
 * @brief Select a metadata object from the metadata table..
 * @param key     [in] key name of the metadata object.
 * @param value   [in] value of key.
 * @param object  [out] a selected metadata object.
 * @return  If success ErrorCode::OK, otherwise error codes.
 */
manager::metadata::ErrorCode IndexDaoJson::select(
    std::string_view key, std::string_view value,
    boost::property_tree::ptree& object) const
{
  ErrorCode error = ErrorCode::UNKNOWN;

  error = session_->load_contents();
  if (error != ErrorCode::OK) {
    return error;
  }

  // Getting a metadata container.
  ptree* container = session_->get_contents();

  // Getting a metadata object.
  error = this->find_metadata_object(*container, key, value, object);
  if (error == ErrorCode::NOT_FOUND) {
    if (key == Object::ID) {
      error = ErrorCode::ID_NOT_FOUND;
    } else if (key == Object::NAME) {
      error = ErrorCode::NAME_NOT_FOUND;
    }
  }
  
  return error;
}

/**
 * @brief Select all metadata objects from the metadata table..
 * @param key     [in] key name of the metadata object.
 * @param value   [in] value of key.
 * @param object  [out] selected metadata objects.
 * @return  If success ErrorCode::OK, otherwise error codes.
 */
manager::metadata::ErrorCode IndexDaoJson::select_all(
    std::vector<boost::property_tree::ptree>& objects) const {
  
  ErrorCode error = ErrorCode::UNKNOWN;

  // Load the meta data from the JSON file.
  error = session_->load_contents();
  if (error != ErrorCode::OK) {
    return error;
  }

  // Getting a metadata container.
  ptree* contents = session_->get_contents();

  // Convert from ptree structure type to vector<ptree>.
  auto node = contents->get_child(IndexDaoJson::INDEXES_ROOT);
  std::transform(node.begin(), node.end(), std::back_inserter(objects),
                 [](ptree::value_type v) { return v.second; });

  return error;
}

manager::metadata::ErrorCode IndexDaoJson::update(
    std::string_view key, std::string_view value,
    const boost::property_tree::ptree& object) const {

  return ErrorCode::UNKNOWN;
}

/**
 * @brief Delete a metadata object from the metadata table.
 * @param key       [in] key name of the metadata object.
 * @param value     [in] value of key.
 * @param object_id [out] removed metadata objects.
 * @return  If success ErrorCode::OK, otherwise error codes.
 */
manager::metadata::ErrorCode IndexDaoJson::remove(
    std::string_view key, std::string_view value,
    ObjectIdType& object_id) const {

  ErrorCode error = ErrorCode::UNKNOWN;

  error = session_->load_contents();
  if (error != ErrorCode::OK) {
    return error;
  }
  ptree* contents = session_->get_contents();

  error = this->delete_metadata_object(*contents, key, value, object_id);

  return error; 
}

} // namespace manager::metadata::db
