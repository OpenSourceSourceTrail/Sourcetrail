#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "TextAccess.h"

namespace {

std::string getTestText() {
  std::string text =
      "\"But the plans were on display . . .\"\n"
      "\"On display? I eventually had to go down to the cellar to find them.\"\n"
      "\"That's the display department.\"\n"
      "\"With a torch.\"\n"
      "\"Ah, well the lights had probably gone.\"\n"
      "\"So had the stairs.\"\n"
      "\"But look, you found the notice, didn't you?\"\n"
      "\"Yes,\" said Arthur, \"yes I did. It was on display in the bottom of a locked"
      " filing cabinet stuck in a disused lavatory with a sign on the door saying"
      " Beware of the Leopard.\"\n";

  return text;
}

}    // namespace

TEST(TextAccess, textAccessStringConstructor) {
  const auto text = getTestText();
  const auto textAccess = TextAccess::createFromString(text);

  EXPECT_TRUE(textAccess.get() != nullptr);
}

TEST(TextAccess, textAccessStringLinesCount) {
  const auto text = getTestText();
  constexpr uint32_t lineCount = 8;

  const auto textAccess = TextAccess::createFromString(text);

  EXPECT_EQ(textAccess->getLineCount(), lineCount);
}

TEST(TextAccess, textAccessStringLinesContent) {
  const auto text = getTestText();

  const auto textAccess = TextAccess::createFromString(text);
  const auto lines = textAccess->getLines(1, 4);

  ASSERT_EQ(4, lines.size());
  EXPECT_THAT(lines[0], testing::StrEq("\"But the plans were on display . . .\"\n"));
  EXPECT_THAT(lines[1], testing::StrEq("\"On display? I eventually had to go down to the cellar to find them.\"\n"));
  EXPECT_THAT(lines[2], testing::StrEq("\"That's the display department.\"\n"));
  EXPECT_THAT(lines[3], testing::StrEq("\"With a torch.\"\n"));
}

TEST(TextAccess, textAccessStringLinesContentErrorHandling) {
  const auto text = getTestText();

  const auto textAccess = TextAccess::createFromString(text);
  auto lines = textAccess->getLines(3, 2);

  EXPECT_TRUE(lines.empty());

  lines = textAccess->getLines(10, 3);

  EXPECT_TRUE(lines.empty());

  lines = textAccess->getLines(1, 10);

  EXPECT_TRUE(lines.empty());

  const auto line = textAccess->getLine(0);

  EXPECT_TRUE(line.empty());

  lines = textAccess->getLines(0, 2);

  EXPECT_TRUE(lines.empty());
}

TEST(TextAccess, TextAccessStringSingleLineContent) {
  const auto text = getTestText();

  const auto textAccess = TextAccess::createFromString(text);
  const auto line = textAccess->getLine(6);

  EXPECT_THAT(line, testing::StrEq("\"So had the stairs.\"\n"));
}

TEST(TextAccess, textAccessStringAllLines) {
  const auto text = getTestText();
  constexpr uint32_t lineCount = 8;

  const auto textAccess = TextAccess::createFromString(text);
  const auto lines = textAccess->getAllLines();

  EXPECT_EQ(lineCount, lines.size());
}

class TextAccessFix : public testing::Test {
public:
  void SetUp() override {
    ASSERT_TRUE(filePath.exists());
    mTextAccess = TextAccess::createFromFile(filePath);
    ASSERT_TRUE(mTextAccess.get() != nullptr);
  }

  void TearDown() override {}

  FilePath filePath = FilePath{L"data/TextAccessTestSuite/text.txt"};
  std::shared_ptr<TextAccess> mTextAccess;
};

TEST_F(TextAccessFix, textAccessFileLinesCount) {
  constexpr uint32_t lineCount = 7;
  EXPECT_EQ(lineCount, mTextAccess->getLineCount());

  const auto lines = mTextAccess->getLines(1, 4);

  ASSERT_EQ(4, lines.size());
  EXPECT_THAT(lines[0],
              testing::StrEq("\"If you're a researcher on this book thing and you were on Earth, you must have been "
                             "gathering material on it.\"\n"));
  EXPECT_THAT(lines[1], testing::StrEq("\"Well, I was able to extend the original entry a bit, yes.\"\n"));
  EXPECT_THAT(lines[2], testing::StrEq("\"Let me see what it says in this edition, then. I've got to see it.\"\n"));
  EXPECT_THAT(lines[3],
              testing::StrEq("... \"What? Harmless! Is that all it's got to say? Harmless! One word! ... Well, for "
                             "God's sake I hope you managed to rectify that a bit.\"\n"));

  EXPECT_EQ(mTextAccess->getFilePath(), filePath);
}