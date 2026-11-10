// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/linux/fl_atspi_text.h"

#include "gtest/gtest.h"

// Checks the range of @boundary at @offset in @text.
static void expect_text_at_offset(const gchar* text,
                                  gint offset,
                                  FlAtspiTextBoundary boundary,
                                  FlAtspiTextPosition position,
                                  gint expected_start,
                                  gint expected_end) {
  gint start, end;
  fl_atspi_text_get_text_at_offset(text, offset, boundary, position, &start,
                                   &end);
  EXPECT_EQ(start, expected_start);
  EXPECT_EQ(end, expected_end);
}

// Checks the range of @granularity at @offset in @text.
static void expect_string_at_offset(const gchar* text,
                                    gint offset,
                                    FlAtspiTextGranularity granularity,
                                    gint expected_start,
                                    gint expected_end) {
  gint start, end;
  fl_atspi_text_get_string_at_offset(text, offset, granularity, &start, &end);
  EXPECT_EQ(start, expected_start);
  EXPECT_EQ(end, expected_end);
}

TEST(FlAtspiTextTest, Offsets) {
  // "a😀b": the emoji is two UTF-16 code units.
  const gchar* text =
      "a\xF0\x9F\x98\x80"
      "b";
  EXPECT_EQ(fl_atspi_text_offset_to_utf16(text, 0), 0);
  EXPECT_EQ(fl_atspi_text_offset_to_utf16(text, 2), 3);
  EXPECT_EQ(fl_atspi_text_offset_to_utf16(text, 3), 4);
  EXPECT_EQ(fl_atspi_text_offset_to_utf16(text, 100), 4);
  EXPECT_EQ(fl_atspi_text_utf16_to_offset(text, 3), 2);
  EXPECT_EQ(fl_atspi_text_utf16_to_offset(text, 100), 3);

  g_autofree gchar* emoji = fl_atspi_text_substring(text, 1, 2);
  EXPECT_STREQ(emoji, "\xF0\x9F\x98\x80");
  g_autofree gchar* end = fl_atspi_text_substring(text, 1, -1);
  EXPECT_STREQ(end,
               "\xF0\x9F\x98\x80"
               "b");
  g_autofree gchar* empty = fl_atspi_text_substring(text, 5, 1);
  EXPECT_STREQ(empty, "");
}

TEST(FlAtspiTextTest, Characters) {
  expect_text_at_offset("ab", 0, FL_ATSPI_TEXT_BOUNDARY_CHAR,
                        FL_ATSPI_TEXT_POSITION_AT, 0, 1);
  expect_text_at_offset("ab", 1, FL_ATSPI_TEXT_BOUNDARY_CHAR,
                        FL_ATSPI_TEXT_POSITION_BEFORE, 0, 1);
  expect_text_at_offset("ab", 0, FL_ATSPI_TEXT_BOUNDARY_CHAR,
                        FL_ATSPI_TEXT_POSITION_AFTER, 1, 2);
  expect_text_at_offset("ab", 2, FL_ATSPI_TEXT_BOUNDARY_CHAR,
                        FL_ATSPI_TEXT_POSITION_AT, 2, 2);
  expect_string_at_offset("ab", 1, FL_ATSPI_TEXT_GRANULARITY_CHAR, 1, 2);
  expect_string_at_offset("ab", 2, FL_ATSPI_TEXT_GRANULARITY_CHAR, 2, 2);
}

TEST(FlAtspiTextTest, Words) {
  const gchar* text = "one two  three";
  expect_text_at_offset(text, 5, FL_ATSPI_TEXT_BOUNDARY_WORD_START,
                        FL_ATSPI_TEXT_POSITION_AT, 4, 9);
  expect_text_at_offset(text, 5, FL_ATSPI_TEXT_BOUNDARY_WORD_START,
                        FL_ATSPI_TEXT_POSITION_BEFORE, 0, 4);
  expect_text_at_offset(text, 5, FL_ATSPI_TEXT_BOUNDARY_WORD_START,
                        FL_ATSPI_TEXT_POSITION_AFTER, 9, 14);
  expect_text_at_offset(text, 5, FL_ATSPI_TEXT_BOUNDARY_WORD_END,
                        FL_ATSPI_TEXT_POSITION_AT, 3, 7);
  expect_string_at_offset(text, 5, FL_ATSPI_TEXT_GRANULARITY_WORD, 4, 7);
  expect_string_at_offset(text, 7, FL_ATSPI_TEXT_GRANULARITY_WORD, 7, 9);
}

TEST(FlAtspiTextTest, Lines) {
  const gchar* text = "first\nsecond\nthird";
  expect_text_at_offset(text, 8, FL_ATSPI_TEXT_BOUNDARY_LINE_START,
                        FL_ATSPI_TEXT_POSITION_AT, 6, 13);
  expect_text_at_offset(text, 8, FL_ATSPI_TEXT_BOUNDARY_LINE_START,
                        FL_ATSPI_TEXT_POSITION_BEFORE, 0, 6);
  expect_text_at_offset(text, 8, FL_ATSPI_TEXT_BOUNDARY_LINE_START,
                        FL_ATSPI_TEXT_POSITION_AFTER, 13, 18);
  expect_text_at_offset(text, 8, FL_ATSPI_TEXT_BOUNDARY_LINE_END,
                        FL_ATSPI_TEXT_POSITION_AT, 5, 12);
  expect_string_at_offset(text, 8, FL_ATSPI_TEXT_GRANULARITY_LINE, 6, 12);
  expect_string_at_offset(text, 18, FL_ATSPI_TEXT_GRANULARITY_LINE, 13, 18);
}

TEST(FlAtspiTextTest, Sentences) {
  const gchar* text = "Hello there. How are you?";
  expect_string_at_offset(text, 15, FL_ATSPI_TEXT_GRANULARITY_SENTENCE, 13, 25);
  expect_text_at_offset(text, 3, FL_ATSPI_TEXT_BOUNDARY_SENTENCE_END,
                        FL_ATSPI_TEXT_POSITION_AT, 0, 12);
}

TEST(FlAtspiTextTest, Empty) {
  expect_text_at_offset("", 0, FL_ATSPI_TEXT_BOUNDARY_WORD_START,
                        FL_ATSPI_TEXT_POSITION_AT, 0, 0);
  expect_string_at_offset("", 0, FL_ATSPI_TEXT_GRANULARITY_LINE, 0, 0);
  expect_string_at_offset("", 0, FL_ATSPI_TEXT_GRANULARITY_WORD, 0, 0);
}
