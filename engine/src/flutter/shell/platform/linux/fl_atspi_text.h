// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_LINUX_FL_ATSPI_TEXT_H_
#define FLUTTER_SHELL_PLATFORM_LINUX_FL_ATSPI_TEXT_H_

#include <glib.h>

G_BEGIN_DECLS

// Helpers for the AT-SPI Text interface. Text is UTF-8 and offsets are in
// Unicode characters, as used by AT-SPI.

/**
 * FlAtspiTextGranularity:
 *
 * AT-SPI text granularities (AtspiTextGranularity).
 */
typedef enum {
  FL_ATSPI_TEXT_GRANULARITY_CHAR = 0,
  FL_ATSPI_TEXT_GRANULARITY_WORD = 1,
  FL_ATSPI_TEXT_GRANULARITY_SENTENCE = 2,
  FL_ATSPI_TEXT_GRANULARITY_LINE = 3,
  FL_ATSPI_TEXT_GRANULARITY_PARAGRAPH = 4,
} FlAtspiTextGranularity;

/**
 * FlAtspiTextBoundary:
 *
 * AT-SPI text boundaries (AtspiTextBoundaryType).
 */
typedef enum {
  FL_ATSPI_TEXT_BOUNDARY_CHAR = 0,
  FL_ATSPI_TEXT_BOUNDARY_WORD_START = 1,
  FL_ATSPI_TEXT_BOUNDARY_WORD_END = 2,
  FL_ATSPI_TEXT_BOUNDARY_SENTENCE_START = 3,
  FL_ATSPI_TEXT_BOUNDARY_SENTENCE_END = 4,
  FL_ATSPI_TEXT_BOUNDARY_LINE_START = 5,
  FL_ATSPI_TEXT_BOUNDARY_LINE_END = 6,
} FlAtspiTextBoundary;

/**
 * FlAtspiTextPosition:
 *
 * The range to get relative to an offset.
 */
typedef enum {
  FL_ATSPI_TEXT_POSITION_BEFORE,
  FL_ATSPI_TEXT_POSITION_AT,
  FL_ATSPI_TEXT_POSITION_AFTER,
} FlAtspiTextPosition;

/**
 * fl_atspi_text_utf16_to_offset:
 * @text: UTF-8 text.
 * @utf16_offset: an offset in UTF-16 code units.
 *
 * Converts a UTF-16 offset, as used by Flutter, to a character offset.
 *
 * Returns: a character offset, clamped to the text.
 */
gint fl_atspi_text_utf16_to_offset(const gchar* text, gint utf16_offset);

/**
 * fl_atspi_text_offset_to_utf16:
 * @text: UTF-8 text.
 * @offset: a character offset.
 *
 * Converts a character offset to a UTF-16 offset, as used by Flutter.
 *
 * Returns: a UTF-16 offset, clamped to the text.
 */
gint fl_atspi_text_offset_to_utf16(const gchar* text, gint offset);

/**
 * fl_atspi_text_substring:
 * @text: UTF-8 text.
 * @start: the character offset to start at.
 * @end: the character offset to end at, or -1 for the end of the text.
 *
 * Gets part of @text, clamping the offsets to the text.
 *
 * Returns: a new UTF-8 string.
 */
gchar* fl_atspi_text_substring(const gchar* text, gint start, gint end);

/**
 * fl_atspi_text_get_text_at_offset:
 * @text: UTF-8 text.
 * @offset: a character offset.
 * @boundary: the boundary type.
 * @position: the range to get relative to @offset.
 * @start: (out): location to write the start of the range.
 * @end: (out): location to write the end of the range.
 *
 * Gets the range between @boundary boundaries before, at or after @offset,
 * as used by GetTextBeforeOffset, GetTextAtOffset and GetTextAfterOffset.
 */
void fl_atspi_text_get_text_at_offset(const gchar* text,
                                      gint offset,
                                      FlAtspiTextBoundary boundary,
                                      FlAtspiTextPosition position,
                                      gint* start,
                                      gint* end);

/**
 * fl_atspi_text_get_string_at_offset:
 * @text: UTF-8 text.
 * @offset: a character offset.
 * @granularity: the unit to get.
 * @start: (out): location to write the start of the range.
 * @end: (out): location to write the end of the range.
 *
 * Gets the unit of @granularity containing @offset, as used by
 * GetStringAtOffset.
 */
void fl_atspi_text_get_string_at_offset(const gchar* text,
                                        gint offset,
                                        FlAtspiTextGranularity granularity,
                                        gint* start,
                                        gint* end);

G_END_DECLS

#endif  // FLUTTER_SHELL_PLATFORM_LINUX_FL_ATSPI_TEXT_H_
