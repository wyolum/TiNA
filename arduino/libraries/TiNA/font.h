// Copyright 2013 WyoLum, LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at:
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
// express or implied.  See the License for the specific language
// governing permissions and limitations under the License.

#ifndef FONT_H
#define FONT_H
#include "inttypes.h"

const uint8_t FONT_RECLEN = 6;
uint8_t font_read_char(File font_file, char c, uint8_t *dest);
bool char_is_blank(char c, File font_file);
#endif
