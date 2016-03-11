#include "gtest/gtest.h"
#include "test/mock-firebase.h"
#include "modem/test/mock-output-stream.h"
#include "modem/test/mock-input-stream.h"
#include "Firebase.h"
#include "modem/commands.h"

namespace firebase {
namespace modem {

using ::testing::Return;
using ::testing::ByMove;
using ::testing::ReturnRef;
using ::testing::_;

class SetCommandTest : public ::testing::Test {
 protected:
  void SetUp() override {
    set_.reset(new MockFirebaseSet());
  }

  void FeedCommand(const String& path, const String& data) {
    const String data_fragment(String(" ") + data + "\r\n");
    EXPECT_CALL(in_, readStringUntil(' '))
        .WillOnce(Return(path));
    EXPECT_CALL(in_, readLine())
        .WillOnce(Return(data_fragment));
  }

  void ExpectOutput(const String& output) {
    EXPECT_CALL(out_, println(output))
        .WillOnce(Return(3));
  }
  
  void ExpectErrorOutput(const String& error_message) {
    EXPECT_CALL(out_, print(String("-FAIL ")))
        .WillOnce(Return(5));
    EXPECT_CALL(out_, println(error_message))
        .WillOnce(Return(error_message.length()));
  }

  void RunExpectingData(const String& data, const FirebaseError& error) {
    EXPECT_CALL(*set_, error())
      .WillRepeatedly(ReturnRef(error));

    EXPECT_CALL(fbase_, setPtr(_, data))
        .WillOnce(Return(ByMove(std::move(set_))));

    SetCommand setCmd(&fbase_);
    if (error) {
      ASSERT_FALSE(setCmd.execute("SET", &in_, &out_));
    } else {
      ASSERT_TRUE(setCmd.execute("SET", &in_, &out_));
    }
  }

  MockInputStream in_;
  MockOutputStream out_;
  MockFirebase fbase_;
  std::unique_ptr<MockFirebaseSet> set_;
};

TEST_F(SetCommandTest, sendsData) {
  const String path("/test/path");
  const String data("This is a test payload.");

  FeedCommand(path, data);
  ExpectOutput("+OK");

  RunExpectingData(data, FirebaseError());
}

TEST_F(SetCommandTest, HandlesError) {
  const String path("/test/path");
  const String data("This is a test payload.");
  FirebaseError error(-200, "Test error.");

  FeedCommand(path, data);
  ExpectErrorOutput(error.message());

  RunExpectingData(data, error);
}
}  // modem
}  // firebase
