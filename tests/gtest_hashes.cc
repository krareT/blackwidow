//  Copyright (c) 2017-present The blackwidow Authors.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.

#include <gtest/gtest.h>
#include <thread>
#include <iostream>

#include "blackwidow/blackwidow.h"

using namespace blackwidow;

class HashesTest : public ::testing::Test {
 public:
  HashesTest() {
    options.create_if_missing = true;
    s = db.Open(options, "./db");
  }
  virtual ~HashesTest() { }

  static void SetUpTestCase() { }
  static void TearDownTestCase() { }

  blackwidow::Options options;
  blackwidow::BlackWidow db;
  blackwidow::Status s;
};

// HSet
TEST_F(HashesTest, HSetTest) {
  int32_t ret = 0;
  std::string value;
  // If field is a new field in the hash and value was set.
  s = db.HSet("HSET_KEY", "HSET_TEST_FIELD", "HSET_TEST_VALUE", &ret);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(ret, 1);
  s = db.HGet("HSET_KEY", "HSET_TEST_FIELD", &value);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(value, "HSET_TEST_VALUE");

  // If field already exists in the hash and the value was updated.
  s = db.HSet("HSET_KEY", "HSET_TEST_FIELD", "HSET_TEST_NEW_VALUE", &ret);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(ret, 0);
  s = db.HGet("HSET_KEY", "HSET_TEST_FIELD", &value);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(value, "HSET_TEST_NEW_VALUE");
}

// HGet
TEST_F(HashesTest, HGetTest) {
  int32_t ret = 0;
  std::string value;
  s = db.HSet("HGET_KEY", "HGET_TEST_FIELD", "HGET_TEST_VALUE", &ret);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(ret, 1);
  s = db.HGet("HGET_KEY", "HGET_TEST_FIELD", &value);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(value, "HGET_TEST_VALUE");

  // If key does not exist.
  s = db.HGet("HGET_NOT_EXIST_KEY", "HGET_TEST_FIELD", &value);
  ASSERT_TRUE(s.IsNotFound());

  // If field is not present in the hash
  s = db.HGet("HGET_KEY", "HGET_NOT_EXIST_FIELD", &value);
  ASSERT_TRUE(s.IsNotFound());
}

// HMSet
TEST_F(HashesTest, HMSetTest) {
  int32_t ret = 0;
  std::vector<BlackWidow::SliceFieldValue> fvs1;
  fvs1.push_back({"TEST_FIELD1", "TEST_VALUE1"});
  fvs1.push_back({"TEST_FIELD2", "TEST_VALUE2"});

  // If field already exists in the hash, it is overwritten
  std::vector<BlackWidow::SliceFieldValue> fvs2;
  fvs2.push_back({"TEST_FIELD2", "TEST_VALUE2"});
  fvs2.push_back({"TEST_FIELD3", "TEST_VALUE3"});
  fvs2.push_back({"TEST_FIELD4", "TEST_VALUE4"});
  fvs2.push_back({"TEST_FIELD3", "TEST_VALUE5"});

  s = db.HMSet("HMSET_KEY", fvs1);
  ASSERT_TRUE(s.ok());
  s = db.HMSet("HMSET_KEY", fvs2);
  ASSERT_TRUE(s.ok());

  s = db.HLen("HMSET_KEY", &ret);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(ret, 4);

  std::vector<std::string> values1;
  std::vector<rocksdb::Slice> fields1 {"TEST_FIELD1",
      "TEST_FIELD2", "TEST_FIELD3", "TEST_FIELD4"};
  s = db.HMGet("HMSET_KEY", fields1, &values1);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(values1.size(),  4);

  ASSERT_EQ(values1[0], "TEST_VALUE1");
  ASSERT_EQ(values1[1], "TEST_VALUE2");
  ASSERT_EQ(values1[2], "TEST_VALUE5");
  ASSERT_EQ(values1[3], "TEST_VALUE4");

  std::map<BlackWidow::DataType, rocksdb::Status> type_status;
  db.Expire("HMSET_KEY", 1, &type_status);
  ASSERT_TRUE(type_status[BlackWidow::DataType::HASHES].ok());

  // The key has timeout
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  std::vector<BlackWidow::SliceFieldValue> fvs3;
  fvs3.push_back({"TEST_FIELD3", "TEST_VALUE3"});
  fvs3.push_back({"TEST_FIELD4", "TEST_VALUE4"});
  fvs3.push_back({"TEST_FIELD5", "TEST_VALUE5"});
  s = db.HMSet("HMSET_KEY", fvs3);
  ASSERT_TRUE(s.ok());

  s = db.HLen("HMSET_KEY", &ret);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(ret, 3);

  std::vector<std::string> values2;
  std::vector<rocksdb::Slice> fields2 {"TEST_FIELD3",
      "TEST_FIELD4", "TEST_FIELD5"};
  s = db.HMGet("HMSET_KEY", fields2, &values2);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(values2.size(),  3);

  ASSERT_EQ(values2[0], "TEST_VALUE3");
  ASSERT_EQ(values2[1], "TEST_VALUE4");
  ASSERT_EQ(values2[2], "TEST_VALUE5");
}

// HMGet
TEST_F(HashesTest, HMGetTest) {
  int32_t ret = 0;
  std::vector<BlackWidow::SliceFieldValue> fvs;
  fvs.push_back({"TEST_FIELD1", "TEST_VALUE1"});
  fvs.push_back({"TEST_FIELD2", "TEST_VALUE2"});
  fvs.push_back({"TEST_FIELD3", "TEST_VALUE3"});
  fvs.push_back({"TEST_FIELD2", "TEST_VALUE4"});
  s = db.HMSet("HMGET_KEY", fvs);
  ASSERT_TRUE(s.ok());

  s = db.HLen("HMGET_KEY", &ret);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(ret, 3);

  std::vector<std::string> values;
  std::vector<rocksdb::Slice> fields {"TEST_FIELD1",
      "TEST_FIELD2", "TEST_FIELD3", "TEST_NOT_EXIST_FIELD"};
  s = db.HMGet("HMGET_KEY", fields, &values);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(values.size(), 4);

  ASSERT_EQ(values[0], "TEST_VALUE1");
  ASSERT_EQ(values[1], "TEST_VALUE4");
  ASSERT_EQ(values[2], "TEST_VALUE3");
  ASSERT_EQ(values[3], "");
}

// HSetnx
TEST_F(HashesTest, HSetnxTest) {
  int32_t ret;
  std::string value;
  // If field is a new field in the hash and value was set.
  s = db.HSetnx("HSETNX_KEY", "HSETNX_TEST_FIELD", "HSETNX_TEST_VALUE", &ret);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(ret, 1);
  s = db.HGet("HSETNX_KEY", "HSETNX_TEST_FIELD", &value);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(value, "HSETNX_TEST_VALUE");

  // If field already exists, this operation has no effect.
  s = db.HSetnx("HSETNX_KEY", "HSETNX_TEST_FIELD",
          "HSETNX_TEST_NEW_VALUE", &ret);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(ret, 0);
  s = db.HGet("HSETNX_KEY", "HSETNX_TEST_FIELD", &value);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(value, "HSETNX_TEST_VALUE");
}

// HLen
TEST_F(HashesTest, HLenTest) {
  int32_t ret = 0;
  std::vector<BlackWidow::SliceFieldValue> fvs;
  fvs.push_back({"TEST_FIELD1", "TEST_VALUE1"});
  fvs.push_back({"TEST_FIELD2", "TEST_VALUE2"});
  fvs.push_back({"TEST_FIELD3", "TEST_VALUE3"});
  s = db.HMSet("HLEN_KEY", fvs);
  ASSERT_TRUE(s.ok());

  s = db.HLen("HLEN_KEY", &ret);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(ret, 3);
}

// HStrlen
TEST_F(HashesTest, HStrlenTest) {
  int32_t ret = 0;
  int32_t len = 0;
  s = db.HSet("HSTRLEN_KEY", "HSTRLEN_TEST_FIELD", "HSTRLEN_TEST_VALUE", &ret);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(ret, 1);

  s = db.HStrlen("HSTRLEN_KEY", "HSTRLEN_TEST_FIELD",  &len);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(len, 18);

  // If the key or the field do not exist, 0 is returned
  s = db.HStrlen("HSTRLEN_KEY", "HSTRLEN_NOT_EXIST_FIELD",  &len);
  ASSERT_TRUE(s.IsNotFound());
  ASSERT_EQ(len, 0);
}


// HExists
TEST_F(HashesTest, HExistsTest) {
  int32_t ret;
  s = db.HSet("HEXIST_KEY", "HEXIST_FIELD", "HEXIST_VALUE", &ret);
  ASSERT_TRUE(s.ok());

  s = db.HExists("HEXIST_KEY", "HEXIST_FIELD");
  ASSERT_TRUE(s.ok());

  // If key does not exist.
  s = db.HExists("HEXIST_NOT_EXIST_KEY", "HEXIST_FIELD");
  ASSERT_TRUE(s.IsNotFound());

  // If field is not present in the hash
  s = db.HExists("HEXIST_KEY", "HEXIST_NOT_EXIST_FIELD");
  ASSERT_TRUE(s.IsNotFound());
}

// HIncrby
TEST_F(HashesTest, HIncrby) {
  int32_t ret;
  int64_t value;
  std::string str_value;

  // If key does not exist the value is set to 0 before the
  // operation is performed
  s = db.HIncrby("HINCRBY_NEW_KEY", "HINCRBY_EXIST_FIELD", 1000, &value);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(value, 1000);
  s = db.HGet("HINCRBY_NEW_KEY", "HINCRBY_EXIST_FIELD", &str_value);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(atoll(str_value.data()), 1000);


  // If the hash field contains a string that can not be
  // represented as integer
  s = db.HSet("HINCRBY_KEY", "HINCRBY_STR_FIELD", "HINCRBY_VALEU", &ret);
  ASSERT_TRUE(s.ok());
  s = db.HIncrby("HINCRBY_KEY", "HINCRBY_STR_FIELD", 100, &value);
  ASSERT_TRUE(s.IsInvalidArgument());

  // If field does not exist the value is set to 0 before the
  // operation is performed
  s = db.HIncrby("HINCRBY_KEY", "HINCRBY_NOT_EXIST_FIELD", 100, &value);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(value, 100);
  s = db.HGet("HINCRBY_KEY", "HINCRBY_NOT_EXIST_FIELD", &str_value);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(atoll(str_value.data()), 100);

  s = db.HSet("HINCRBY_KEY", "HINCRBY_NUM_FIELD", "100", &ret);
  ASSERT_TRUE(s.ok());

  // Positive test
  s = db.HIncrby("HINCRBY_KEY", "HINCRBY_NUM_FIELD", 100, &value);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(value, 200);
  s = db.HGet("HINCRBY_KEY", "HINCRBY_NUM_FIELD", &str_value);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(atoll(str_value.data()), 200);

  // Negative test
  s = db.HIncrby("HINCRBY_KEY", "HINCRBY_NUM_FIELD", -100, &value);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(value, 100);
  s = db.HGet("HINCRBY_KEY", "HINCRBY_NUM_FIELD", &str_value);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(atoll(str_value.data()), 100);

  // Larger than the maximum number 9223372036854775807
  s = db.HSet("HINCRBY_KEY", "HINCRBY_NUM_FIELD", "10", &ret);
  ASSERT_TRUE(s.ok());
  s = db.HIncrby("HINCRBY_KEY", "HINCRBY_NUM_FIELD",
          9223372036854775807, &value);
  ASSERT_TRUE(s.IsInvalidArgument());

  // Less than the minimum number -9223372036854775808
  s = db.HSet("HINCRBY_KEY", "HINCRBY_NUM_FIELD", "-10", &ret);
  ASSERT_TRUE(s.ok());
  s = db.HIncrby("HINCRBY_KEY", "HINCRBY_NUM_FIELD",
          -9223372036854775807, &value);
  ASSERT_TRUE(s.IsInvalidArgument());
}

// HDel
TEST_F(HashesTest, HDel) {
  int32_t ret = 0;
  std::vector<BlackWidow::SliceFieldValue> fvs;
  fvs.push_back({"TEST_FIELD1", "TEST_VALUE1"});
  fvs.push_back({"TEST_FIELD2", "TEST_VALUE2"});
  fvs.push_back({"TEST_FIELD3", "TEST_VALUE3"});
  fvs.push_back({"TEST_FIELD4", "TEST_VALUE4"});

  s = db.HMSet("HDEL_KEY", fvs);
  ASSERT_TRUE(s.ok());

  std::vector<rocksdb::Slice> fields {"TEST_FIELD1", "TEST_FIELD2",
    "TEST_FIELD3", "TEST_FIElD2", "TEST_NOT_EXIST_FIELD"};
  s = db.HDel("HDEL_KEY", fields, &ret);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(ret, 3);

  s = db.HLen("HDEL_KEY", &ret);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(ret, 1);

  // Delete not exist hash table
  s = db.HDel("HDEL_NOT_EXIST_KEY", fields, &ret);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(ret, 0);

  // Delete timeout hash table
  s = db.HMSet("HDEL_TIMEOUT_KEY", fvs);
  ASSERT_TRUE(s.ok());

  std::map<BlackWidow::DataType, rocksdb::Status> type_status;
  db.Expire("HDEL_TIMEOUT_KEY", 1, &type_status);
  ASSERT_TRUE(type_status[BlackWidow::DataType::HASHES].ok());
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  s = db.HDel("HDEL_TIMEOUT_KEY", fields, &ret);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(ret, 0);
}


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
