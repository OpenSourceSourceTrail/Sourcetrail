#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Settings.h"

using namespace testing;

namespace {
constexpr std::wstring_view SettingPath = L"data/SettingsTestSuite/settings.xml";
constexpr std::wstring_view WrongSettingPath = L"data/SettingsTestSuite/wrong_settings.xml";

class TestSettings : public Settings {
public:
  bool getBool() const {
    return getValue<bool>("Bool", false);
  }

  bool setBool(bool value) {
    return setValue<bool>("Bool", value);
  }

  int getInt() const {
    return getValue<int>("Int", -1);
  }

  bool setInt(int value) {
    return setValue<int>("Int", value);
  }

  float getFloat() const {
    constexpr auto FloatValue = 0.01F;
    return getValue<float>("Float", FloatValue);
  }

  bool setFloat(float value) {
    return setValue<float>("Float", value);
  }

  std::string getString() const {
    return getValue<std::string>("String", "<empty>");
  }

  bool setString(const std::string& value) {
    return setValue<std::string>("String", value);
  }

  std::wstring getWString() const {
    return getValue<std::wstring>("WString", L"<empty>");
  }

  bool setWString(const std::wstring& value) {
    return setValue<std::wstring>("WString", value);
  }

  bool getNewBool() const {
    return getValue<bool>("NewBool", false);
  }

  bool setNewBool(bool value) {
    return setValue<bool>("NewBool", value);
  }
};
}    // namespace

TEST(Settings, settingsGetLoadedFromFile) {
  TestSettings settings;
  EXPECT_TRUE(settings.load(FilePath(SettingPath.data())));
}

TEST(Settings, settingsGetNotLoadedFromFile) {
  TestSettings settings;
  EXPECT_FALSE(settings.load(FilePath(WrongSettingPath.data())));
}

TEST(Settings, settingsGetLoadedValue) {
  constexpr auto FloatValue = 3.1416F;

  TestSettings settings;
  ASSERT_TRUE(settings.load(FilePath(SettingPath.data())));

  EXPECT_TRUE(settings.getBool());
  EXPECT_EQ(42, settings.getInt());
  EXPECT_FLOAT_EQ(FloatValue, settings.getFloat());
  EXPECT_THAT(settings.getString(), StrEq("Hello World!"));
  EXPECT_THAT(settings.getWString(), StrEq(L"Hello World!"));
}

TEST(Settings, settingsGetDefaultValueWhenNotLoaded) {
  constexpr auto FloatValue = 0.01F;

  const TestSettings settings;
  EXPECT_FALSE(settings.getBool());
  EXPECT_EQ(-1, settings.getInt());
  EXPECT_FLOAT_EQ(FloatValue, settings.getFloat());
  EXPECT_THAT(settings.getString(), StrEq("<empty>"));
  EXPECT_THAT(settings.getWString(), StrEq(L"<empty>"));
}

TEST(Settings, settingsGetDefaultValueWhenWronglyLoaded) {
  constexpr auto FloatValue = 0.01F;

  TestSettings settings;
  ASSERT_FALSE(settings.load(FilePath(WrongSettingPath.data())));

  EXPECT_FALSE(settings.getBool());
  EXPECT_EQ(-1, settings.getInt());
  EXPECT_FLOAT_EQ(FloatValue, settings.getFloat());
  EXPECT_THAT(settings.getString(), StrEq("<empty>"));
  EXPECT_THAT(settings.getWString(), StrEq(L"<empty>"));
}

TEST(Settings, settingsGetDefaultValueAfterClearing) {
  constexpr auto FloatValue = 0.01F;

  TestSettings settings;
  ASSERT_TRUE(settings.load(FilePath(SettingPath.data())));

  settings.clear();
  EXPECT_FALSE(settings.getBool());
  EXPECT_EQ(-1, settings.getInt());
  EXPECT_FLOAT_EQ(FloatValue, settings.getFloat());
  EXPECT_THAT(settings.getString(), StrEq("<empty>"));
  EXPECT_THAT(settings.getWString(), StrEq(L"<empty>"));
}

TEST(Settings, settingsCanBeSetWhenNotLoaded) {
  constexpr auto FloatValue = 2.5F;

  TestSettings settings;

  ASSERT_TRUE(settings.setBool(false));
  EXPECT_FALSE(settings.getBool());

  ASSERT_TRUE(settings.setInt(2));
  EXPECT_EQ(2, settings.getInt());

  ASSERT_TRUE(settings.setFloat(FloatValue));
  EXPECT_FLOAT_EQ(FloatValue, settings.getFloat());

  ASSERT_TRUE(settings.setString("foo"));
  EXPECT_THAT(settings.getString(), StrEq("foo"));

  ASSERT_TRUE(settings.setWString(L"bar"));
  EXPECT_THAT(settings.getWString(), StrEq(L"bar"));
}

TEST(Settings, settingsCanBeReplacedWhenLoaded) {
  constexpr auto FloatValue = 2.5F;

  TestSettings settings;
  ASSERT_TRUE(settings.load(FilePath(SettingPath.data())));

  ASSERT_TRUE(settings.setBool(false));
  EXPECT_FALSE(settings.getBool());

  ASSERT_TRUE(settings.setInt(2));
  EXPECT_EQ(2, settings.getInt());

  ASSERT_TRUE(settings.setFloat(FloatValue));
  EXPECT_FLOAT_EQ(FloatValue, settings.getFloat());

  ASSERT_TRUE(settings.setString("foo"));
  EXPECT_THAT(settings.getString(), StrEq("foo"));

  ASSERT_TRUE(settings.setWString(L"bar"));
  EXPECT_THAT(settings.getWString(), StrEq(L"bar"));
}

TEST(Settings, settingsCanBeAddedWhenLoaded) {
  TestSettings settings;
  EXPECT_TRUE(settings.load(FilePath(SettingPath.data())));

  EXPECT_FALSE(settings.getNewBool());
  EXPECT_TRUE(settings.setNewBool(true));
  EXPECT_TRUE(settings.getNewBool());
}
