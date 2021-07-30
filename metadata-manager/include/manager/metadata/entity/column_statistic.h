/*
 * Copyright 2020 tsurugi project.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
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
#ifndef MANAGER_METADATA_ENTITY_COLUMN_STATISTIC_H_
#define MANAGER_METADATA_ENTITY_COLUMN_STATISTIC_H_

#include <boost/property_tree/ptree.hpp>

#include "manager/metadata/metadata.h"

namespace manager::metadata {

class ColumnStatistic {
 public:
  ObjectIdType table_id;
  ObjectIdType ordinal_position;
  boost::property_tree::ptree column_statistic;
};  // class ColumnStatistic

}  // namespace manager::metadata

#endif  // MANAGER_METADATA_ENTITY_COLUMN_STATISTIC_H_
