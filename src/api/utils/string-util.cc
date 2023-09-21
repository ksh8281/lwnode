/*
 * Copyright (c) 2021-present Samsung Electronics Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Copyright 2019 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "string-util.h"

#include <sstream>

// Magic values subtracted from a buffer value during UTF8 conversion.
// This table contains as many values as there might be trailing bytes
// in a UTF-8 sequence.
const uint32_t UTF8Sequence::s_offsetsFromUTF8[6] = {
    0x00000000UL,
    0x00003080UL,
    0x000E2080UL,
    0x03C82080UL,
    static_cast<uint32_t>(0xFA082080UL),
    static_cast<uint32_t>(0x82082080UL)};

static bool isLineTerminator(char32_t ch)
{
  return ch == '\r' || ch == '\n' || ch == 0x2028 || ch == 0x2029;
}

template<typename InputType, typename StringReadCallback, typename ResultCharType,
         typename ResultStringType, const bool willFailureWithNonLatin1Char>
bool stripCommentsFromSourceString(InputType sequence, InputType endSequence,
                                   StringReadCallback readCallback, ResultStringType& result)
{
   while (sequence < endSequence) {
    char32_t character = readCallback(sequence);
    if (character == '\'' || character == '"' || character == '`') {
      auto quote = character;
      result += (ResultCharType)character;
      while (sequence < endSequence) {
        character = readCallback(sequence);
        if (willFailureWithNonLatin1Char && character > 255) {
          return false;
        }
        result += (ResultCharType)character;
        if (character == quote) {
          break;
        } else if (character == '\\') {
          if (sequence + 1 < endSequence) {
            InputType forwardSequence = sequence;
            character = readCallback(forwardSequence);
            if (character == quote) {
              result += (ResultCharType)character;
              sequence = forwardSequence;
            } else if (character == '\\') {
              result += (ResultCharType)character;
              sequence = forwardSequence;
            }
          }
        }
      }
      continue;
    } else if (character == '/' && sequence + 1 < endSequence) {
      character = readCallback(sequence);
      if (character == '/' && (!result.size() || result.back() != '\\')) {
        // skip singleline comment
        do {
          character = readCallback(sequence);
        } while (sequence < endSequence && !isLineTerminator(character));
        if (character == 0x0D && sequence + 1 < endSequence) {
          InputType forwardSequence = sequence;
          character = readCallback(forwardSequence);
          if (character == 0x0A) {
            sequence = forwardSequence;
            character = '\n';
          }
        }
      } else if (character == '*') {
        // skip multiline comment
        while (sequence < endSequence) {
          character = readCallback(sequence);
          if (character == '*' && sequence + 1 < endSequence) {
             character = readCallback(sequence);
             if (character == '/') {
               break;
             }
          } else if (isLineTerminator(character)) {
            if (character == 0x0D && sequence + 1 < endSequence) {
              InputType forwardSequence = sequence;
              character = readCallback(forwardSequence);
              if (character == 0x0A) {
                sequence = forwardSequence;
              }
            }
            result += (ResultCharType)'\n';
          }
        }
        continue;
      } else {
        result += (ResultCharType)'/';
      }
    }

    if (willFailureWithNonLatin1Char && character > 255) {
       return false;
     }
    result += (ResultCharType)character;
  }
  return true;
}

std::basic_string<uint8_t, std::char_traits<uint8_t>>
  stripCommentsFromLatin1SourceString(const uint8_t* start, const uint8_t* end)
{
  std::basic_string<uint8_t, std::char_traits<uint8_t>> result;
  stripCommentsFromSourceString<const unsigned char*,
     char32_t (*)(const uint8_t*&), uint8_t,
     std::basic_string<uint8_t, std::char_traits<uint8_t>>, false>
     (start, end, [](const uint8_t*& sequence) -> char32_t {
           auto ret = *sequence;
           sequence++;
           return ret;
         }, result);
  return result;
}

bool UTF8Sequence::convertUTF8ToLatin1(
    std::basic_string<unsigned char, std::char_traits<unsigned char>>&
        oneByteString,
    const unsigned char* sequence,
    const unsigned char* endSequence,
    bool stripComment) {
  if (stripComment) {
    return stripCommentsFromSourceString<const unsigned char*,
      char32_t (*)(const unsigned char*&), unsigned char,
      std::basic_string<unsigned char, std::char_traits<unsigned char>>, true>
      (sequence, endSequence, [](const unsigned char*& sequence) -> char32_t {
        return UTF8Sequence::read(sequence, UTF8Sequence::getLength(*sequence));
      }, oneByteString);
  } else {
    while (sequence < endSequence) {
      char32_t character =
          UTF8Sequence::read(sequence, UTF8Sequence::getLength(*sequence));

      // if character is out of acsii and latin1
      if (character > 255) {
        return false;
      } else {
        oneByteString += (unsigned char)character;
      }
    }

    return true;
  }
}

std::vector<std::string> strSplit(const std::string& str, char delimiter) {
  std::vector<std::string> tokens;
  std::stringstream ss(str);
  std::string token;

  while (getline(ss, token, delimiter)) {
    tokens.push_back(token);
  }

  return tokens;
}
