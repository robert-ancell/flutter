// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/linux/fl_atspi_text.h"

#include <pango/pango.h>

#include <cstring>

static gint compare_ints(gconstpointer a, gconstpointer b) {
  return *static_cast<const gint*>(a) - *static_cast<const gint*>(b);
}

// Sorts @values and removes duplicates.
static void sort_unique(GArray* values) {
  g_array_sort(values, compare_ints);
  guint n_unique = 0;
  for (guint i = 0; i < values->len; i++) {
    gint value = g_array_index(values, gint, i);
    if (n_unique == 0 || g_array_index(values, gint, n_unique - 1) != value) {
      g_array_index(values, gint, n_unique++) = value;
    }
  }
  g_array_set_size(values, n_unique);
}

// Gets the sorted character offsets of @boundary boundaries in @text, always
// including the start and end of the text.
static GArray* get_boundaries(const gchar* text, FlAtspiTextBoundary boundary) {
  gint length = g_utf8_strlen(text, -1);
  GArray* boundaries = g_array_new(FALSE, FALSE, sizeof(gint));
  switch (boundary) {
    case FL_ATSPI_TEXT_BOUNDARY_CHAR:
      for (gint i = 0; i < length; i++) {
        g_array_append_val(boundaries, i);
      }
      break;
    case FL_ATSPI_TEXT_BOUNDARY_WORD_START:
    case FL_ATSPI_TEXT_BOUNDARY_WORD_END:
    case FL_ATSPI_TEXT_BOUNDARY_SENTENCE_START:
    case FL_ATSPI_TEXT_BOUNDARY_SENTENCE_END: {
      g_autofree PangoLogAttr* attrs = g_new0(PangoLogAttr, length + 1);
      pango_get_log_attrs(text, strlen(text), -1, pango_language_get_default(),
                          attrs, length + 1);
      for (gint i = 0; i <= length; i++) {
        gboolean is_boundary =
            boundary == FL_ATSPI_TEXT_BOUNDARY_WORD_START
                ? attrs[i].is_word_start
            : boundary == FL_ATSPI_TEXT_BOUNDARY_WORD_END ? attrs[i].is_word_end
            : boundary == FL_ATSPI_TEXT_BOUNDARY_SENTENCE_START
                ? attrs[i].is_sentence_start
                : attrs[i].is_sentence_end;
        if (is_boundary) {
          g_array_append_val(boundaries, i);
        }
      }
      break;
    }
    case FL_ATSPI_TEXT_BOUNDARY_LINE_START:
    case FL_ATSPI_TEXT_BOUNDARY_LINE_END: {
      gint i = 0;
      for (const gchar* c = text; *c != '\0'; c = g_utf8_next_char(c), i++) {
        if (*c == '\n') {
          gint offset =
              boundary == FL_ATSPI_TEXT_BOUNDARY_LINE_START ? i + 1 : i;
          g_array_append_val(boundaries, offset);
        }
      }
      break;
    }
  }
  gint start = 0;
  g_array_append_val(boundaries, start);
  g_array_append_val(boundaries, length);

  sort_unique(boundaries);

  return boundaries;
}

gint fl_atspi_text_utf16_to_offset(const gchar* text, gint utf16_offset) {
  gint offset = 0;
  gint units = 0;
  for (const gchar* c = text; *c != '\0' && units < utf16_offset;
       c = g_utf8_next_char(c)) {
    units += g_utf8_get_char(c) >= 0x10000 ? 2 : 1;
    offset++;
  }
  return offset;
}

gint fl_atspi_text_offset_to_utf16(const gchar* text, gint offset) {
  gint units = 0;
  gint i = 0;
  for (const gchar* c = text; *c != '\0' && i < offset;
       c = g_utf8_next_char(c), i++) {
    units += g_utf8_get_char(c) >= 0x10000 ? 2 : 1;
  }
  return units;
}

gchar* fl_atspi_text_substring(const gchar* text, gint start, gint end) {
  gint count = g_utf8_strlen(text, -1);
  if (end < 0 || end > count) {
    end = count;
  }
  start = CLAMP(start, 0, end);
  return g_utf8_substring(text, start, end);
}

void fl_atspi_text_get_text_at_offset(const gchar* text,
                                      gint offset,
                                      FlAtspiTextBoundary boundary,
                                      FlAtspiTextPosition position,
                                      gint* start,
                                      gint* end) {
  g_autoptr(GArray) boundaries = get_boundaries(text, boundary);
  gint* values = reinterpret_cast<gint*>(boundaries->data);
  gint count = values[boundaries->len - 1];
  *start = *end = 0;
  if (boundaries->len < 2) {
    return;
  }
  offset = CLAMP(offset, 0, count);

  if (boundary == FL_ATSPI_TEXT_BOUNDARY_CHAR && offset == count) {
    *start = position == FL_ATSPI_TEXT_POSITION_BEFORE && count > 0 ? count - 1
                                                                    : count;
    *end = count;
    return;
  }

  // Find the range containing @offset. At the end of the text this is the
  // last range.
  guint end_index = 1;
  while (end_index < boundaries->len - 1 && values[end_index] <= offset) {
    end_index++;
  }

  switch (position) {
    case FL_ATSPI_TEXT_POSITION_AT:
      break;
    case FL_ATSPI_TEXT_POSITION_BEFORE:
      if (end_index < 2) {
        return;
      }
      end_index--;
      break;
    case FL_ATSPI_TEXT_POSITION_AFTER:
      if (end_index + 1 >= boundaries->len) {
        *start = *end = count;
        return;
      }
      end_index++;
      break;
  }
  *start = values[end_index - 1];
  *end = values[end_index];
}

void fl_atspi_text_get_string_at_offset(const gchar* text,
                                        gint offset,
                                        FlAtspiTextGranularity granularity,
                                        gint* start,
                                        gint* end) {
  gint count = g_utf8_strlen(text, -1);
  offset = CLAMP(offset, 0, count);
  switch (granularity) {
    case FL_ATSPI_TEXT_GRANULARITY_CHAR:
      *start = offset;
      *end = offset < count ? offset + 1 : count;
      return;
    case FL_ATSPI_TEXT_GRANULARITY_WORD: {
      // The word containing @offset, or the space between words.
      g_autoptr(GArray) boundaries =
          get_boundaries(text, FL_ATSPI_TEXT_BOUNDARY_WORD_START);
      g_autoptr(GArray) ends =
          get_boundaries(text, FL_ATSPI_TEXT_BOUNDARY_WORD_END);
      g_array_append_vals(boundaries, ends->data, ends->len);
      sort_unique(boundaries);
      gint* values = reinterpret_cast<gint*>(boundaries->data);
      guint i = 0;
      while (i < boundaries->len - 1 && values[i] <= offset) {
        i++;
      }
      if (i == 0) {
        *start = *end = 0;
        return;
      }
      *start = values[i - 1];
      *end = values[i];
      return;
    }
    case FL_ATSPI_TEXT_GRANULARITY_SENTENCE:
      fl_atspi_text_get_text_at_offset(text, offset,
                                       FL_ATSPI_TEXT_BOUNDARY_SENTENCE_START,
                                       FL_ATSPI_TEXT_POSITION_AT, start, end);
      return;
    case FL_ATSPI_TEXT_GRANULARITY_LINE:
    case FL_ATSPI_TEXT_GRANULARITY_PARAGRAPH:
      // The line excluding its newline.
      fl_atspi_text_get_text_at_offset(text, offset,
                                       FL_ATSPI_TEXT_BOUNDARY_LINE_START,
                                       FL_ATSPI_TEXT_POSITION_AT, start, end);
      if (*end > *start && *g_utf8_offset_to_pointer(text, *end - 1) == '\n') {
        (*end)--;
      }
      return;
  }
  *start = *end = offset;
}
