
# クライアントにおけるメタデータ処理

2020.03.02 NEC

## テーブルメタデータを統合メタデータ管理基盤に新規作成する

1. tableメタデータオブジェクトを作成する
1. rootオブジェクトにtableメタデータオブジェクトを追加する
1. TableMetadata::save()で統合メタデータ管理基盤にテーブルメタデータを保存する

boost::property_tree::ptreeオブジェクトにテーブルメタデータを入力して、tableメタデータオブジェクトを作成する
```c++
    boost::property_tree::ptree new_table;  // tableメタデータオブジェクト
    new_table.put(TableMetadata::NAME, "sample table");
```

boost::property_tree::ptreeオブジェクトにテーブルメタデータを入力する（配列・基本データ型）
```c++
boost::property_tree::ptree primary_keys;
{
    // 子ノードの作成
    boost::property_tree::ptree primary_key;
    // 配列要素１
    primary_key.put<uint64_t>("", 1);
    primary_keys.push_back(std::make_pair("", primary_key));    // 配列に追加
    // 配列要素２
    primary_key.put<uint64_t>("", 2);
    primary_keys.push_back(std::make_pair("", primary_key));    // 配列に追加
}
// 親ノードに子ノードを追加
new_table.add_child(TableMetadata::PRIMARY_KEY_NODE, primary_keys);
```

boost::property_tree::ptreeオブジェクトにテーブルメタデータを入力する（配列・オブジェクト）
```c++
    ptree columns;
    {
        // 子ノードの作成
        ptree column;
        // 配列要素１
        column.put(TableMetadata::Column::NAME, "column_1");
        column.put<uint64_t>(TableMetadata::Column::ORDINAL_POSITION, 1);
        columns.push_back(std::make_pair("", column));  // 配列に追加
        // 配列要素２
        column.put(TableMetadata::Column::NAME, "column_2");
        column.put<uint64_t>(TableMetadata::Column::ORDINAL_POSITION, 2);
        columns.push_back(std::make_pair("", column));  // 配列に追加
    }
    // 親ノードに子ノードを追加
     new_table.add_child(TableMetadata::COLUMNS_NODE, columns);
```

dataTypeIdの取得方法
```c++
    // データ型メタデータオブジェクトの生成
    std::unique_ptr<Metadata> datatypes(new DataTypeMetadata("データベース名"));
    if (datatypes->load() != ErrorCode::OK) {
        エラー処理
    }
    // データ型名を指定してDataTypeメタデータオブジェクトを取得する
    boost::property_tree::ptree datatype;   // DataTypeメタデータオブジェクト
    if (datatypes->get("INT64", datatype) != ErrorCode::OK) {
        エラー処理
    }
    // データ型IDの取得
    boost::optional<ObjectIdType> data_type_id = 
        datatype.get_optional<ObjectIdType>(DataTypeMetadata::ID);
    if (!data_type_id) {
        エラー処理
    }
```

rootオブジェクトにtableメタデータオブジェクトを追加する
```c++
    boost::property_tree::ptree root;
    root.add_child(TableMetadata::TABLES_NODE, new_table);
```

TableMetadata::save()で統合メタデータ管理基盤にテーブルメタデータを保存する
```c++
    if (TableMetadata::save("データベース名", root) != manager::metadata_manager::OK) {
        エラー処理
    }
```
---
## テーブルメタデータを統合メタデータ管理基盤に追加する

1. TableMetadataオブジェクトを作成する
1. TableMetadataオブジェクトに統合メタデータ管理基盤上のメタデータを読み込む
1. boost::property_tree::ptreeオブジェクトにテーブルメタデータを入力する
1. TableMetadata::add()でテーブルメタデータをNEDO DBに追加する（永続化もされる）
 
TableMetadataオブジェクトを作成する
```c++
    // Template-Methodパターンを使用しているため親クラス(Metadataクラス)のポインタを使用する
    // V1では、データベース名は任意の文字列で可
    std::unique_ptr<Metadata> tables(new TableMetadata("データベース名"));
```

TableMetadataオブジェクトに統合メタデータ管理基盤上のメタデータを読み込む
```c++
    if ( tables->load() != manager::metadata_manager::ErrorCode::OK) {
        エラー処理
    }
```

boost::property_tree::ptreeオブジェクトにテーブルメタデータを入力する
```c++
    boost::property_tree::ptree new_table;
    new_table.put(TableMetadata::NAME, "sample table");
```

boost::property_tree::ptreeオブジェクトにテーブルメタデータを入力する（配列・基本データ型）
```c++
    boost::property_tree::ptree primary_keys;
    {
        // 子ノードの作成
        boost::property_tree::ptree primary_key;
        // 配列要素１
        primary_key.put<uint64_t>("", 1);
        primary_keys.push_back(std::make_pair("", primary_key));    // 配列に追加
        // 配列要素２
        primary_key.put<uint64_t>("", 2);
        primary_keys.push_back(std::make_pair("", primary_key));    // 配列に追加
    }
    // 親ノードに子ノードを追加
    new_table.add_child(TableMetadata::PRIMARY_KEY_NODE, primary_keys);
```

boost::property_tree::ptreeオブジェクトにテーブルメタデータを入力する（配列・オブジェクト）
```c++
    ptree columns;
    {
        // 子ノードの作成
        ptree column;
        // 配列要素１
        column.put(TableMetadata::Column::NAME, "column_1");
        column.put<uint64_t>(TableMetadata::Column::ORDINAL_POSITION, 1);
        columns.push_back(std::make_pair("", column));  // 配列に追加
        // 配列要素２
        column.put(TableMetadata::Column::NAME, "column_2");
        column.put<uint64_t>(TableMetadata::Column::ORDINAL_POSITION, 2);
        columns.push_back(std::make_pair("", column));  // 配列に追加
    }
    // 親ノードに子ノードを追加
     new_table.add_child(TableMetadata::COLUMNS_NODE, columns);
```

TableMetadata::add()でテーブルメタデータをNEDO DBに追加する（永続化もされる）
```c++
    if (tables.add(new_table) != manager::metadata_manager::ErrorCode::OK) {
        エラー処理
    }

```

以上