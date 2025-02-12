/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2001-2002 by NaN Holding BV.
 * All rights reserved.
 */

/** \file
 * \ingroup bli
 */

#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MEM_guardedalloc.h"

#include "BLI_dynstr.h"
#include "BLI_string.h"

#include "BLI_utildefines.h"

#ifdef __GNUC__
#  pragma GCC diagnostic error "-Wsign-conversion"
#endif

// #define DEBUG_STRSIZE

/**
 * Duplicates the first \a len bytes of cstring \a str
 * into a newly mallocN'd string and returns it. \a str
 * is assumed to be at least len bytes long.
 *
 * \param str: The string to be duplicated
 * \param len: The number of bytes to duplicate
 * \retval Returns the duplicated string
 */
char *BLI_strdupn(const char *str, const size_t len)
{
  char *n = MEM_mallocN(len + 1, "strdup");
  memcpy(n, str, len);
  n[len] = '\0';

  return n;
}

/**
 * Duplicates the cstring \a str into a newly mallocN'd
 * string and returns it.
 *
 * \param str: The string to be duplicated
 * \retval Returns the duplicated string
 */
char *BLI_strdup(const char *str)
{
  return BLI_strdupn(str, strlen(str));
}

/**
 * Appends the two strings, and returns new mallocN'ed string
 * \param str1: first string for copy
 * \param str2: second string for append
 * \retval Returns dst
 */
char *BLI_strdupcat(const char *__restrict str1, const char *__restrict str2)
{
  /* include the NULL terminator of str2 only */
  const size_t str1_len = strlen(str1);
  const size_t str2_len = strlen(str2) + 1;
  char *str, *s;

  str = MEM_mallocN(str1_len + str2_len, "strdupcat");
  s = str;

  memcpy(s, str1, str1_len); /* NOLINT: bugprone-not-null-terminated-result */
  s += str1_len;
  memcpy(s, str2, str2_len);

  return str;
}

/**
 * Like strncpy but ensures dst is always
 * '\0' terminated.
 *
 * \param dst: Destination for copy
 * \param src: Source string to copy
 * \param maxncpy: Maximum number of characters to copy (generally
 * the size of dst)
 * \retval Returns dst
 */
char *BLI_strncpy(char *__restrict dst, const char *__restrict src, const size_t maxncpy)
{
  size_t srclen = BLI_strnlen(src, maxncpy - 1);
  BLI_assert(maxncpy != 0);

#ifdef DEBUG_STRSIZE
  memset(dst, 0xff, sizeof(*dst) * maxncpy);
#endif

  memcpy(dst, src, srclen);
  dst[srclen] = '\0';
  return dst;
}

/**
 * Like BLI_strncpy but ensures dst is always padded by given char,
 * on both sides (unless src is empty).
 *
 * \param dst: Destination for copy
 * \param src: Source string to copy
 * \param pad: the char to use for padding
 * \param maxncpy: Maximum number of characters to copy (generally the size of dst)
 * \retval Returns dst
 */
char *BLI_strncpy_ensure_pad(char *__restrict dst,
                             const char *__restrict src,
                             const char pad,
                             size_t maxncpy)
{
  BLI_assert(maxncpy != 0);

#ifdef DEBUG_STRSIZE
  memset(dst, 0xff, sizeof(*dst) * maxncpy);
#endif

  if (src[0] == '\0') {
    dst[0] = '\0';
  }
  else {
    /* Add heading/trailing wildcards if needed. */
    size_t idx = 0;
    size_t srclen;

    if (src[idx] != pad) {
      dst[idx++] = pad;
      maxncpy--;
    }
    maxncpy--; /* trailing '\0' */

    srclen = BLI_strnlen(src, maxncpy);
    if ((src[srclen - 1] != pad) && (srclen == maxncpy)) {
      srclen--;
    }

    memcpy(&dst[idx], src, srclen);
    idx += srclen;

    if (dst[idx - 1] != pad) {
      dst[idx++] = pad;
    }
    dst[idx] = '\0';
  }

  return dst;
}

/**
 * Like strncpy but ensures dst is always
 * '\0' terminated.
 *
 * \note This is a duplicate of #BLI_strncpy that returns bytes copied.
 * And is a drop in replacement for 'snprintf(str, sizeof(str), "%s", arg);'
 *
 * \param dst: Destination for copy
 * \param src: Source string to copy
 * \param maxncpy: Maximum number of characters to copy (generally
 * the size of dst)
 * \retval The number of bytes copied (The only difference from BLI_strncpy).
 */
size_t BLI_strncpy_rlen(char *__restrict dst, const char *__restrict src, const size_t maxncpy)
{
  size_t srclen = BLI_strnlen(src, maxncpy - 1);
  BLI_assert(maxncpy != 0);

#ifdef DEBUG_STRSIZE
  memset(dst, 0xff, sizeof(*dst) * maxncpy);
#endif

  memcpy(dst, src, srclen);
  dst[srclen] = '\0';
  return srclen;
}

size_t BLI_strcpy_rlen(char *__restrict dst, const char *__restrict src)
{
  size_t srclen = strlen(src);
  memcpy(dst, src, srclen + 1);
  return srclen;
}

/**
 * Portable replacement for `vsnprintf`.
 */
size_t BLI_vsnprintf(char *__restrict buffer,
                     size_t maxncpy,
                     const char *__restrict format,
                     va_list arg)
{
  size_t n;

  BLI_assert(buffer != NULL);
  BLI_assert(maxncpy > 0);
  BLI_assert(format != NULL);

  n = (size_t)vsnprintf(buffer, maxncpy, format, arg);

  if (n != -1 && n < maxncpy) {
    buffer[n] = '\0';
  }
  else {
    buffer[maxncpy - 1] = '\0';
  }

  return n;
}

/**
 * A version of #BLI_vsnprintf that returns ``strlen(buffer)``
 */
size_t BLI_vsnprintf_rlen(char *__restrict buffer,
                          size_t maxncpy,
                          const char *__restrict format,
                          va_list arg)
{
  size_t n;

  BLI_assert(buffer != NULL);
  BLI_assert(maxncpy > 0);
  BLI_assert(format != NULL);

  n = (size_t)vsnprintf(buffer, maxncpy, format, arg);

  if (n != -1 && n < maxncpy) {
    /* pass */
  }
  else {
    n = maxncpy - 1;
  }
  buffer[n] = '\0';

  return n;
}

/**
 * Portable replacement for #snprintf
 */
size_t BLI_snprintf(char *__restrict dst, size_t maxncpy, const char *__restrict format, ...)
{
  size_t n;
  va_list arg;

#ifdef DEBUG_STRSIZE
  memset(dst, 0xff, sizeof(*dst) * maxncpy);
#endif

  va_start(arg, format);
  n = BLI_vsnprintf(dst, maxncpy, format, arg);
  va_end(arg);

  return n;
}

/**
 * A version of #BLI_snprintf that returns ``strlen(dst)``
 */
size_t BLI_snprintf_rlen(char *__restrict dst, size_t maxncpy, const char *__restrict format, ...)
{
  size_t n;
  va_list arg;

#ifdef DEBUG_STRSIZE
  memset(dst, 0xff, sizeof(*dst) * maxncpy);
#endif

  va_start(arg, format);
  n = BLI_vsnprintf_rlen(dst, maxncpy, format, arg);
  va_end(arg);

  return n;
}

/**
 * Print formatted string into a newly #MEM_mallocN'd string
 * and return it.
 */
char *BLI_sprintfN(const char *__restrict format, ...)
{
  DynStr *ds;
  va_list arg;
  char *n;

  va_start(arg, format);

  ds = BLI_dynstr_new();
  BLI_dynstr_vappendf(ds, format, arg);
  n = BLI_dynstr_get_cstring(ds);
  BLI_dynstr_free(ds);

  va_end(arg);

  return n;
}

/**
 * This roughly matches C and Python's string escaping with double quotes - `"`.
 *
 * Since every character may need escaping,
 * it's common to create a buffer twice as large as the input.
 *
 * \param dst: The destination string, at least \a dst_maxncpy, typically `(strlen(src) * 2) + 1`.
 * \param src: The un-escaped source string.
 * \param dst_maxncpy: The maximum number of bytes allowable to copy.
 *
 * \note This is used for creating animation paths in blend files.
 */
size_t BLI_str_escape(char *__restrict dst, const char *__restrict src, const size_t dst_maxncpy)
{

  BLI_assert(dst_maxncpy != 0);

  size_t len = 0;
  for (; (len < dst_maxncpy) && (*src != '\0'); dst++, src++, len++) {
    char c = *src;
    if (ELEM(c, '\\', '"') ||                       /* Use as-is. */
        ((c == '\t') && ((void)(c = 't'), true)) || /* Tab. */
        ((c == '\n') && ((void)(c = 'n'), true)) || /* Newline. */
        ((c == '\r') && ((void)(c = 'r'), true)) || /* Carriage return. */
        ((c == '\a') && ((void)(c = 'a'), true)) || /* Bell. */
        ((c == '\b') && ((void)(c = 'b'), true)) || /* Backspace. */
        ((c == '\f') && ((void)(c = 'f'), true)))   /* Form-feed. */
    {
      if (UNLIKELY(len + 1 >= dst_maxncpy)) {
        /* Not enough space to escape. */
        break;
      }
      *dst++ = '\\';
      len++;
    }
    *dst = c;
  }
  *dst = '\0';

  return len;
}

/**
 * This roughly matches C and Python's string escaping with double quotes - `"`.
 *
 * The destination will never be larger than the source, it will either be the same
 * or up to half when all characters are escaped.
 *
 * \param dst: The destination string, at least the size of `strlen(src) + 1`.
 * \param src: The escaped source string.
 * \param dst_maxncpy: The maximum number of bytes allowable to copy.
 *
 * \note This is used for for parsing animation paths in blend files.
 */
size_t BLI_str_unescape(char *__restrict dst, const char *__restrict src, const size_t src_maxncpy)
{
  size_t len = 0;
  for (size_t i = 0; i < src_maxncpy && (*src != '\0'); i++, src++) {
    char c = *src;
    if (c == '\\') {
      char c_next = *(src + 1);
      if (((c_next == '"') && ((void)(c = '"'), true)) ||   /* Quote. */
          ((c_next == '\\') && ((void)(c = '\\'), true)) || /* Backslash. */
          ((c_next == 't') && ((void)(c = '\t'), true)) ||  /* Tab. */
          ((c_next == 'n') && ((void)(c = '\n'), true)) ||  /* Newline. */
          ((c_next == 'r') && ((void)(c = '\r'), true)) ||  /* Carriage return. */
          ((c_next == 'a') && ((void)(c = '\a'), true)) ||  /* Bell. */
          ((c_next == 'b') && ((void)(c = '\b'), true)) ||  /* Backspace. */
          ((c_next == 'f') && ((void)(c = '\f'), true)))    /* Form-feed. */
      {
        i++;
        src++;
      }
    }

    dst[len++] = c;
  }
  dst[len] = 0;
  return len;
}

/**
 * Find the first un-escaped quote in the string (to find the end of the string).
 *
 * \param str: Typically this is the first character in a quoted string.
 * Where the character before `*str` would be `"`.

 * \return The pointer to the first un-escaped quote.
 */
const char *BLI_str_escape_find_quote(const char *str)
{
  bool escape = false;
  while (*str && (*str != '"' || escape)) {
    /* A pair of back-slashes represents a single back-slash,
     * only use a single back-slash for escaping. */
    escape = (escape == false) && (*str == '\\');
    str++;
  }
  return (*str == '"') ? str : NULL;
}

/**
 * Makes a copy of the text within the "" that appear after some text `blahblah`.
 * i.e. for string `pose["apples"]` with prefix `pose[`, it will return `apples`.
 *
 * \param str: is the entire string to chop.
 * \param prefix: is the part of the string to step over.
 *
 * Assume that the strings returned must be freed afterwards,
 * and that the inputs will contain data we want.
 */
char *BLI_str_quoted_substrN(const char *__restrict str, const char *__restrict prefix)
{
  const char *start_match, *end_match;

  /* get the starting point (i.e. where prefix starts, and add prefix_len+1
   * to it to get be after the first " */
  start_match = strstr(str, prefix);
  if (start_match) {
    const size_t prefix_len = strlen(prefix);
    start_match += prefix_len + 1;
    /* get the end point (i.e. where the next occurrence of " is after the starting point) */
    end_match = BLI_str_escape_find_quote(start_match);
    if (end_match) {
      const size_t escaped_len = (size_t)(end_match - start_match);
      char *result = MEM_mallocN(sizeof(char) * (escaped_len + 1), __func__);
      const size_t unescaped_len = BLI_str_unescape(result, start_match, escaped_len);
      if (unescaped_len != escaped_len) {
        result = MEM_reallocN(result, sizeof(char) * (unescaped_len + 1));
      }
      return result;
    }
  }
  return NULL;
}

/**
 * string with all instances of substr_old replaced with substr_new,
 * Returns a copy of the c-string \a str into a newly #MEM_mallocN'd
 * and returns it.
 *
 * \note A rather wasteful string-replacement utility, though this shall do for now...
 * Feel free to replace this with an even safe + nicer alternative
 *
 * \param str: The string to replace occurrences of substr_old in
 * \param substr_old: The text in the string to find and replace
 * \param substr_new: The text in the string to find and replace
 * \retval Returns the duplicated string
 */
char *BLI_str_replaceN(const char *__restrict str,
                       const char *__restrict substr_old,
                       const char *__restrict substr_new)
{
  DynStr *ds = NULL;
  size_t len_old = strlen(substr_old);
  const char *match;

  BLI_assert(substr_old[0] != '\0');

  /* While we can still find a match for the old sub-string that we're searching for,
   * keep dicing and replacing. */
  while ((match = strstr(str, substr_old))) {
    /* the assembly buffer only gets created when we actually need to rebuild the string */
    if (ds == NULL) {
      ds = BLI_dynstr_new();
    }

    /* If the match position does not match the current position in the string,
     * copy the text up to this position and advance the current position in the string. */
    if (str != match) {
      /* Add the segment of the string from `str` to match to the buffer,
       * then restore the value at match. */
      BLI_dynstr_nappend(ds, str, (match - str));

      /* now our current position should be set on the start of the match */
      str = match;
    }

    /* Add the replacement text to the accumulation buffer. */
    BLI_dynstr_append(ds, substr_new);

    /* Advance the current position of the string up to the end of the replaced segment. */
    str += len_old;
  }

  /* Finish off and return a new string that has had all occurrences of. */
  if (ds) {
    char *str_new;

    /* Add what's left of the string to the assembly buffer
     * - we've been adjusting `str` to point at the end of the replaced segments. */
    BLI_dynstr_append(ds, str);

    /* Convert to new c-string (MEM_malloc'd), and free the buffer. */
    str_new = BLI_dynstr_get_cstring(ds);
    BLI_dynstr_free(ds);

    return str_new;
  }
  /* Just create a new copy of the entire string - we avoid going through the assembly buffer
   * for what should be a bit more efficiency. */
  return BLI_strdup(str);
}

/**
 * In-place replace every \a src to \a dst in \a str.
 *
 * \param str: The string to operate on.
 * \param src: The character to replace.
 * \param dst: The character to replace with.
 */
void BLI_str_replace_char(char *str, char src, char dst)
{
  while (*str) {
    if (*str == src) {
      *str = dst;
    }
    str++;
  }
}

/**
 * Compare two strings without regard to case.
 *
 * \retval True if the strings are equal, false otherwise.
 */
int BLI_strcaseeq(const char *a, const char *b)
{
  return (BLI_strcasecmp(a, b) == 0);
}

/**
 * Portable replacement for `strcasestr` (not available in MSVC)
 */
char *BLI_strcasestr(const char *s, const char *find)
{
  char c, sc;
  size_t len;

  if ((c = *find++) != 0) {
    c = tolower(c);
    len = strlen(find);
    do {
      do {
        if ((sc = *s++) == 0) {
          return NULL;
        }
        sc = tolower(sc);
      } while (sc != c);
    } while (BLI_strncasecmp(s, find, len) != 0);
    s--;
  }
  return ((char *)s);
}

int BLI_string_max_possible_word_count(const int str_len)
{
  return (str_len / 2) + 1;
}

bool BLI_string_has_word_prefix(const char *haystack, const char *needle, size_t needle_len)
{
  const char *match = BLI_strncasestr(haystack, needle, needle_len);
  if (match) {
    if ((match == haystack) || (*(match - 1) == ' ') || ispunct(*(match - 1))) {
      return true;
    }
    return BLI_string_has_word_prefix(match + 1, needle, needle_len);
  }
  return false;
}

bool BLI_string_all_words_matched(const char *name,
                                  const char *str,
                                  int (*words)[2],
                                  const int words_len)
{
  int index;
  for (index = 0; index < words_len; index++) {
    if (!BLI_string_has_word_prefix(name, str + words[index][0], (size_t)words[index][1])) {
      break;
    }
  }
  const bool all_words_matched = (index == words_len);

  return all_words_matched;
}

/**
 * Variation of #BLI_strcasestr with string length limited to \a len
 */
char *BLI_strncasestr(const char *s, const char *find, size_t len)
{
  char c, sc;

  if ((c = *find++) != 0) {
    c = tolower(c);
    if (len > 1) {
      do {
        do {
          if ((sc = *s++) == 0) {
            return NULL;
          }
          sc = tolower(sc);
        } while (sc != c);
      } while (BLI_strncasecmp(s, find, len - 1) != 0);
    }
    else {
      {
        do {
          if ((sc = *s++) == 0) {
            return NULL;
          }
          sc = tolower(sc);
        } while (sc != c);
      }
    }
    s--;
  }
  return ((char *)s);
}

int BLI_strcasecmp(const char *s1, const char *s2)
{
  int i;
  char c1, c2;

  for (i = 0;; i++) {
    c1 = tolower(s1[i]);
    c2 = tolower(s2[i]);

    if (c1 < c2) {
      return -1;
    }
    if (c1 > c2) {
      return 1;
    }
    if (c1 == 0) {
      break;
    }
  }

  return 0;
}

int BLI_strncasecmp(const char *s1, const char *s2, size_t len)
{
  size_t i;
  char c1, c2;

  for (i = 0; i < len; i++) {
    c1 = tolower(s1[i]);
    c2 = tolower(s2[i]);

    if (c1 < c2) {
      return -1;
    }
    if (c1 > c2) {
      return 1;
    }
    if (c1 == 0) {
      break;
    }
  }

  return 0;
}

/* compare number on the left size of the string */
static int left_number_strcmp(const char *s1, const char *s2, int *tiebreaker)
{
  const char *p1 = s1, *p2 = s2;
  int numdigit, numzero1, numzero2;

  /* count and skip leading zeros */
  for (numzero1 = 0; *p1 == '0'; numzero1++) {
    p1++;
  }
  for (numzero2 = 0; *p2 == '0'; numzero2++) {
    p2++;
  }

  /* find number of consecutive digits */
  for (numdigit = 0;; numdigit++) {
    if (isdigit(*(p1 + numdigit)) && isdigit(*(p2 + numdigit))) {
      continue;
    }
    if (isdigit(*(p1 + numdigit))) {
      return 1; /* s2 is bigger */
    }
    if (isdigit(*(p2 + numdigit))) {
      return -1; /* s1 is bigger */
    }
    break;
  }

  /* same number of digits, compare size of number */
  if (numdigit > 0) {
    int compare = (int)strncmp(p1, p2, (size_t)numdigit);

    if (compare != 0) {
      return compare;
    }
  }

  /* use number of leading zeros as tie breaker if still equal */
  if (*tiebreaker == 0) {
    if (numzero1 > numzero2) {
      *tiebreaker = 1;
    }
    else if (numzero1 < numzero2) {
      *tiebreaker = -1;
    }
  }

  return 0;
}

/**
 * Case insensitive, *natural* string comparison,
 * keeping numbers in order.
 */
int BLI_strcasecmp_natural(const char *s1, const char *s2)
{
  int d1 = 0, d2 = 0;
  char c1, c2;
  int tiebreaker = 0;

  /* if both chars are numeric, to a left_number_strcmp().
   * then increase string deltas as long they are
   * numeric, else do a tolower and char compare */

  while (1) {
    if (isdigit(s1[d1]) && isdigit(s2[d2])) {
      int numcompare = left_number_strcmp(s1 + d1, s2 + d2, &tiebreaker);

      if (numcompare != 0) {
        return numcompare;
      }

      /* Some wasted work here, left_number_strcmp already consumes at least some digits. */
      d1++;
      while (isdigit(s1[d1])) {
        d1++;
      }
      d2++;
      while (isdigit(s2[d2])) {
        d2++;
      }
    }

    /* Test for end of strings first so that shorter strings are ordered in front. */
    if (ELEM(0, s1[d1], s2[d2])) {
      break;
    }

    c1 = tolower(s1[d1]);
    c2 = tolower(s2[d2]);

    if (c1 == c2) {
      /* Continue iteration */
    }
    /* Check for '.' so "foo.bar" comes before "foo 1.bar". */
    else if (c1 == '.') {
      return -1;
    }
    else if (c2 == '.') {
      return 1;
    }
    else if (c1 < c2) {
      return -1;
    }
    else if (c1 > c2) {
      return 1;
    }

    d1++;
    d2++;
  }

  if (tiebreaker) {
    return tiebreaker;
  }

  /* we might still have a different string because of lower/upper case, in
   * that case fall back to regular string comparison */
  return strcmp(s1, s2);
}

/**
 * Like strcmp, but will ignore any heading/trailing pad char for comparison.
 * So e.g. if pad is '*', '*world' and 'world*' will compare equal.
 */
int BLI_strcmp_ignore_pad(const char *str1, const char *str2, const char pad)
{
  size_t str1_len, str2_len;

  while (*str1 == pad) {
    str1++;
  }
  while (*str2 == pad) {
    str2++;
  }

  str1_len = strlen(str1);
  str2_len = strlen(str2);

  while (str1_len && (str1[str1_len - 1] == pad)) {
    str1_len--;
  }
  while (str2_len && (str2[str2_len - 1] == pad)) {
    str2_len--;
  }

  if (str1_len == str2_len) {
    return strncmp(str1, str2, str2_len);
  }
  if (str1_len > str2_len) {
    int ret = strncmp(str1, str2, str2_len);
    if (ret == 0) {
      ret = 1;
    }
    return ret;
  }
  {
    int ret = strncmp(str1, str2, str1_len);
    if (ret == 0) {
      ret = -1;
    }
    return ret;
  }
}

/* determine the length of a fixed-size string */
size_t BLI_strnlen(const char *s, const size_t maxlen)
{
  size_t len;

  for (len = 0; len < maxlen; len++, s++) {
    if (!*s) {
      break;
    }
  }
  return len;
}

void BLI_str_tolower_ascii(char *str, const size_t len)
{
  size_t i;

  for (i = 0; (i < len) && str[i]; i++) {
    if (str[i] >= 'A' && str[i] <= 'Z') {
      str[i] += 'a' - 'A';
    }
  }
}

void BLI_str_toupper_ascii(char *str, const size_t len)
{
  size_t i;

  for (i = 0; (i < len) && str[i]; i++) {
    if (str[i] >= 'a' && str[i] <= 'z') {
      str[i] -= 'a' - 'A';
    }
  }
}

/**
 * Strip whitespace from end of the string.
 */
void BLI_str_rstrip(char *str)
{
  for (int i = (int)strlen(str) - 1; i >= 0; i--) {
    if (isspace(str[i])) {
      str[i] = '\0';
    }
    else {
      break;
    }
  }
}

/**
 * Strip trailing zeros from a float, eg:
 *   0.0000 -> 0.0
 *   2.0010 -> 2.001
 *
 * \param str:
 * \param pad:
 * \return The number of zeros stripped.
 */
int BLI_str_rstrip_float_zero(char *str, const char pad)
{
  char *p = strchr(str, '.');
  int totstrip = 0;
  if (p) {
    char *end_p;
    p++;                         /* position at first decimal place */
    end_p = p + (strlen(p) - 1); /* position at last character */
    if (end_p > p) {
      while (end_p != p && *end_p == '0') {
        *end_p = pad;
        end_p--;
        totstrip++;
      }
    }
  }

  return totstrip;
}

/**
 * Return index of a string in a string array.
 *
 * \param str: The string to find.
 * \param str_array: Array of strings.
 * \param str_array_len: The length of the array, or -1 for a NULL-terminated array.
 * \return The index of str in str_array or -1.
 */
int BLI_str_index_in_array_n(const char *__restrict str,
                             const char **__restrict str_array,
                             const int str_array_len)
{
  int index;
  const char **str_iter = str_array;

  for (index = 0; index < str_array_len; str_iter++, index++) {
    if (STREQ(str, *str_iter)) {
      return index;
    }
  }
  return -1;
}

/**
 * Return index of a string in a string array.
 *
 * \param str: The string to find.
 * \param str_array: Array of strings, (must be NULL-terminated).
 * \return The index of str in str_array or -1.
 */
int BLI_str_index_in_array(const char *__restrict str, const char **__restrict str_array)
{
  int index;
  const char **str_iter = str_array;

  for (index = 0; *str_iter; str_iter++, index++) {
    if (STREQ(str, *str_iter)) {
      return index;
    }
  }
  return -1;
}

/**
 * Find if a string starts with another string.
 *
 * \param str: The string to search within.
 * \param start: The string we look for at the start.
 * \return If str starts with start.
 */
bool BLI_str_startswith(const char *__restrict str, const char *__restrict start)
{
  for (; *str && *start; str++, start++) {
    if (*str != *start) {
      return false;
    }
  }

  return (*start == '\0');
}

bool BLI_strn_endswith(const char *__restrict str, const char *__restrict end, size_t slength)
{
  size_t elength = strlen(end);

  if (elength < slength) {
    const char *iter = &str[slength - elength];
    while (*iter) {
      if (*iter++ != *end++) {
        return false;
      }
    }
    return true;
  }
  return false;
}

/**
 * Find if a string ends with another string.
 *
 * \param str: The string to search within.
 * \param end: The string we look for at the end.
 * \return If str ends with end.
 */
bool BLI_str_endswith(const char *__restrict str, const char *__restrict end)
{
  const size_t slength = strlen(str);
  return BLI_strn_endswith(str, end, slength);
}

/**
 * Find the first char matching one of the chars in \a delim, from left.
 *
 * \param str: The string to search within.
 * \param delim: The set of delimiters to search for, as unicode values.
 * \param sep: Return value, set to the first delimiter found (or NULL if none found).
 * \param suf: Return value, set to next char after the first delimiter found
 * (or NULL if none found).
 * \return The length of the prefix (i.e. *sep - str).
 */
size_t BLI_str_partition(const char *str, const char delim[], const char **sep, const char **suf)
{
  return BLI_str_partition_ex(str, NULL, delim, sep, suf, false);
}

/**
 * Find the first char matching one of the chars in \a delim, from right.
 *
 * \param str: The string to search within.
 * \param delim: The set of delimiters to search for, as unicode values.
 * \param sep: Return value, set to the first delimiter found (or NULL if none found).
 * \param suf: Return value, set to next char after the first delimiter found
 * (or NULL if none found).
 * \return The length of the prefix (i.e. *sep - str).
 */
size_t BLI_str_rpartition(const char *str, const char delim[], const char **sep, const char **suf)
{
  return BLI_str_partition_ex(str, NULL, delim, sep, suf, true);
}

/**
 * Find the first char matching one of the chars in \a delim, either from left or right.
 *
 * \param str: The string to search within.
 * \param end: If non-NULL, the right delimiter of the string.
 * \param delim: The set of delimiters to search for, as unicode values.
 * \param sep: Return value, set to the first delimiter found (or NULL if none found).
 * \param suf: Return value, set to next char after the first delimiter found
 * (or NULL if none found).
 * \param from_right: If %true, search from the right of \a str, else, search from its left.
 * \return The length of the prefix (i.e. *sep - str).
 */
size_t BLI_str_partition_ex(const char *str,
                            const char *end,
                            const char delim[],
                            const char **sep,
                            const char **suf,
                            const bool from_right)
{
  const char *d;
  char *(*func)(const char *str, int c) = from_right ? strrchr : strchr;

  BLI_assert(end == NULL || end > str);

  *sep = *suf = NULL;

  for (d = delim; *d != '\0'; d++) {
    const char *tmp;

    if (end) {
      if (from_right) {
        for (tmp = end - 1; (tmp >= str) && (*tmp != *d); tmp--) {
          /* pass */
        }
        if (tmp < str) {
          tmp = NULL;
        }
      }
      else {
        tmp = func(str, *d);
        if (tmp >= end) {
          tmp = NULL;
        }
      }
    }
    else {
      tmp = func(str, *d);
    }

    if (tmp && (from_right ? (*sep < tmp) : (!*sep || *sep > tmp))) {
      *sep = tmp;
    }
  }

  if (*sep) {
    *suf = *sep + 1;
    return (size_t)(*sep - str);
  }

  return end ? (size_t)(end - str) : strlen(str);
}

static size_t BLI_str_format_int_grouped_ex(char src[16], char dst[16], int num_len)
{
  char *p_src = src;
  char *p_dst = dst;

  const char separator = ',';
  int commas;

  if (*p_src == '-') {
    *p_dst++ = *p_src++;
    num_len--;
  }

  for (commas = 2 - num_len % 3; *p_src; commas = (commas + 1) % 3) {
    *p_dst++ = *p_src++;
    if (commas == 1) {
      *p_dst++ = separator;
    }
  }
  *--p_dst = '\0';

  return (size_t)(p_dst - dst);
}

/**
 * Format ints with decimal grouping.
 * 1000 -> 1,000
 *
 * \param dst: The resulting string
 * \param num: Number to format
 * \return The length of \a dst
 */
size_t BLI_str_format_int_grouped(char dst[16], int num)
{
  char src[16];
  int num_len = sprintf(src, "%d", num);

  return BLI_str_format_int_grouped_ex(src, dst, num_len);
}

/**
 * Format uint64_t with decimal grouping.
 * 1000 -> 1,000
 *
 * \param dst: The resulting string
 * \param num: Number to format
 * \return The length of \a dst
 */
size_t BLI_str_format_uint64_grouped(char dst[16], uint64_t num)
{
  /* NOTE: Buffer to hold maximum unsigned int64, which is 1.8e+19. but
   * we also need space for commas and null-terminator. */
  char src[27];
  int num_len = sprintf(src, "%" PRIu64 "", num);

  return BLI_str_format_int_grouped_ex(src, dst, num_len);
}

/**
 * Format a size in bytes using binary units.
 * 1000 -> 1 KB
 * Number of decimal places grows with the used unit (e.g. 1.5 MB, 1.55 GB, 1.545 TB).
 *
 * \param dst: The resulting string.
 * Dimension of 14 to support largest possible value for \a bytes (#LLONG_MAX).
 * \param bytes: Number to format.
 * \param base_10: Calculate using base 10 (GB, MB, ...) or 2 (GiB, MiB, ...).
 */
void BLI_str_format_byte_unit(char dst[15], long long int bytes, const bool base_10)
{
  double bytes_converted = bytes;
  int order = 0;
  int decimals;
  const int base = base_10 ? 1000 : 1024;
  const char *units_base_10[] = {"B", "KB", "MB", "GB", "TB", "PB"};
  const char *units_base_2[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
  const int tot_units = ARRAY_SIZE(units_base_2);

  BLI_STATIC_ASSERT(ARRAY_SIZE(units_base_2) == ARRAY_SIZE(units_base_10), "array size mismatch");

  while ((fabs(bytes_converted) >= base) && ((order + 1) < tot_units)) {
    bytes_converted /= base;
    order++;
  }
  decimals = MAX2(order - 1, 0);

  /* Format value first, stripping away floating zeroes. */
  const size_t dst_len = 15;
  size_t len = BLI_snprintf_rlen(dst, dst_len, "%.*f", decimals, bytes_converted);
  len -= (size_t)BLI_str_rstrip_float_zero(dst, '\0');
  dst[len++] = ' ';
  BLI_strncpy(dst + len, base_10 ? units_base_10[order] : units_base_2[order], dst_len - len);
}

/**
 * Find the ranges needed to split \a str into its individual words.
 *
 * \param str: The string to search for words.
 * \param len: Size of the string to search.
 * \param delim: Character to use as a delimiter.
 * \param r_words: Info about the words found. Set to [index, len] pairs.
 * \param words_max: Max number of words to find
 * \return The number of words found in \a str
 */
int BLI_string_find_split_words(
    const char *str, const size_t len, const char delim, int r_words[][2], int words_max)
{
  int n = 0, i;
  bool charsearch = true;

  /* Skip leading spaces */
  for (i = 0; (i < len) && (str[i] != '\0'); i++) {
    if (str[i] != delim) {
      break;
    }
  }

  for (; (i < len) && (str[i] != '\0') && (n < words_max); i++) {
    if ((str[i] != delim) && (charsearch == true)) {
      r_words[n][0] = i;
      charsearch = false;
    }
    else {
      if ((str[i] == delim) && (charsearch == false)) {
        r_words[n][1] = i - r_words[n][0];
        n++;
        charsearch = true;
      }
    }
  }

  if (charsearch == false) {
    r_words[n][1] = i - r_words[n][0];
    n++;
  }

  return n;
}
