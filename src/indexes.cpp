/*
 * Copyright 2020-2021 tsurugi project.
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
#include "manager/metadata/indexes.h"

#include <memory>
#include <jwt-cpp/jwt.h>

#include "manager/metadata/common/config.h"
#include "manager/metadata/common/jwt_claims.h"
#include "manager/metadata/common/message.h"
#include "manager/metadata/helper/logging_helper.h"
#include "manager/metadata/helper/table_metadata_helper.h"
#include "manager/metadata/provider/datatypes_provider.h"
#include "manager/metadata/provider/indexes_provider.h"
#include "manager/metadata/helper/ptree_helper.h"

namespace manager::metadata {

using boost::property_tree::ptree;

// ==========================================================================
// Index struct methods.
/**
 * @brief 
 */
boost::property_tree::ptree Index::convert_to_ptree() const
{
  auto pt = Object::convert_to_ptree();
  pt.put(OWNER_ID, this->owner_id);
  pt.put(ACCESS_METHOD, this->access_method);
  pt.put(NUMBER_OF_COLUMNS, this->number_of_columns);
  pt.put(NUMBER_OF_KEY_COLUMNS, this->number_of_key_columns);
  
  ptree keys = ptree_helper::make_array_ptree(this->keys);
  pt.push_back(std::make_pair(KEYS, keys));
  
  ptree keys_id = ptree_helper::make_array_ptree(this->keys_id);
  pt.push_back(std::make_pair(KEYS_ID, keys_id));
  
  ptree options = ptree_helper::make_array_ptree(this->options);
  pt.push_back(std::make_pair(OPTIONS, options));

  return pt;
}

/**
 * @brief 
 */
void Index::convert_from_ptree(const boost::property_tree::ptree& pt)
{
  Object::convert_from_ptree(pt);
  auto opt_int = pt.get_optional<int64_t>(OWNER_ID);
  this->owner_id = opt_int ? opt_int.get() : INVALID_OBJECT_ID;

  opt_int = pt.get_optional<int64_t>(ACCESS_METHOD);
  this->access_method = opt_int ? opt_int.get() : INVALID_VALUE;

  opt_int = pt.get_optional<int64_t>(NUMBER_OF_COLUMNS);
  this->number_of_columns = opt_int ? opt_int.get() : INVALID_VALUE;

  opt_int = pt.get_optional<int64_t>(NUMBER_OF_KEY_COLUMNS);
  this->number_of_key_columns = opt_int ? opt_int.get() : INVALID_VALUE;

  this->keys = ptree_helper::make_vector(pt, KEYS);
  this->keys_id = ptree_helper::make_vector(pt, KEYS_ID);
  this->options = ptree_helper::make_vector(pt, OPTIONS);
}

// ==========================================================================
// Indexes class methods.

ErrorCode Indexes::init() const {
  return ErrorCode::OK;
}

ErrorCode Indexes::add(const boost::property_tree::ptree& object) const {
  return ErrorCode::OK;
}
ErrorCode Indexes::add(const boost::property_tree::ptree& object,
              ObjectId* object_id) const {
  return ErrorCode::OK;
}

ErrorCode Indexes::get(const ObjectId object_id,
              boost::property_tree::ptree& object) const {
  return ErrorCode::OK;
}

ErrorCode Indexes::get(std::string_view object_name,
              boost::property_tree::ptree& object) const {
  return ErrorCode::OK;
}

ErrorCode Indexes::get_all(
    std::vector<boost::property_tree::ptree>& container) const {
  return ErrorCode::OK;
}

/**
 * @brief Update metadata-table with metadata-object.
 * @param object_id [in]  ID of the metadata-table to update.
 * @param object    [in]  metadata-object to update.
 * @return ErrorCode::OK if success, otherwise an error code.
 */
ErrorCode Indexes::update(const ObjectIdType object_id,
                          const boost::property_tree::ptree& object) const {
                          
  return ErrorCode::OK;
}

ErrorCode Indexes::remove(const ObjectId object_id) const {
  return ErrorCode::OK;
}
ErrorCode Indexes::remove(std::string_view object_name,
                  ObjectId* object_id) const {
  return ErrorCode::OK;
              
}

ErrorCode Indexes::add(const manager::metadata::Index& index) const {
  return ErrorCode::OK;
}
ErrorCode Indexes::add(const manager::metadata::Index& index,
              ObjectId* object_id) const {
  return ErrorCode::OK;

}

ErrorCode Indexes::get(const ObjectId object_id,
              manager::metadata::Index& index) const {
  return ErrorCode::OK;
}

ErrorCode Indexes::get_all(
    std::vector<manager::metadata::Index>& container) const {
  return ErrorCode::OK;
      
}

} // namespace manager::metadata
