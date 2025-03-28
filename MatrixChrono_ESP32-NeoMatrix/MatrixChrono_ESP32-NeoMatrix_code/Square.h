/*
 * MatrixChrono - IoT Smart Home Clock System
 * Copyright (c) 2025 João Fernandes
 * 
 * This work is licensed under the Creative Commons Attribution-NonCommercial 
 * 4.0 International License. To view a copy of this license, visit:
 * http://creativecommons.org/licenses/by-nc/4.0/
 */

//modified last on v5.10
const uint8_t square_big_font_Bitmaps[] PROGMEM = {
  0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0x80, 0x80, 
  0xFC, 0x63, 0x18, 0xC7, 0xE0, 0x21, 0x08, 0x42, 0x10, 0x80, 0xF8, 0x43, 
  0xF8, 0x43, 0xE0, 0xF8, 0x42, 0xF0, 0x87, 0xE0, 0x8C, 0x63, 0xF0, 0x84, 
  0x20, 0xFC, 0x21, 0xF0, 0x87, 0xE0, 0xFC, 0x21, 0xF8, 0xC7, 0xE0, 0xF8, 
  0x44, 0x42, 0x10, 0x80, 0xFC, 0x63, 0xF8, 0xC7, 0xE0, 0xFC, 0x63, 0xF0, 
  0x87, 0xE0, 0x44, 0x00, 0xFC, 0x21, 0x08, 0x43, 0xE0
};

const GFXglyph square_big_font_Glyphs[] PROGMEM = {
  {     0,   2,   8,   2,    0,   -7 },   // 0x20 ' '
  {     0,   0,   0,   0,    0,    0 },   // 0x21 '!'
  {     0,   0,   0,   0,    0,    0 },   // 0x22 '"'
  {     3,   6,   8,   6,    0,   -7 },   // 0x23 '#'
  {     9,   3,   3,   4,    0,   -6 },   // 0x24 '$'
  {     0,   0,   0,   0,    0,    0 },   // 0x25 '%'
  {     0,   0,   0,   0,    0,    0 },   // 0x26 '&'
  {     0,   0,   0,   0,    0,    0 },   // 0x27 '''
  {     0,   0,   0,   0,    0,    0 },   // 0x28 '('
  {     0,   0,   0,   0,    0,    0 },   // 0x29 ')'
  {     0,   0,   0,   0,    0,    0 },   // 0x2A '*'
  {     0,   0,   0,   0,    0,    0 },   // 0x2B '+'
  {     0,   0,   0,   0,    0,    0 },   // 0x2C ','
  {     0,   0,   0,   0,    0,    0 },   // 0x2D '-'
  {    11,   1,   1,   2,    0,    0 },   // 0x2E '.'
  {     0,   0,   0,   0,    0,    0 },   // 0x2F '/'
  {    12,   5,   7,   6,    0,   -6 },   // 0x30 '0'
  {    17,   5,   7,   6,    0,   -6 },   // 0x31 '1'
  {    22,   5,   7,   6,    0,   -6 },   // 0x32 '2'
  {    27,   5,   7,   6,    0,   -6 },   // 0x33 '3'
  {    32,   5,   7,   6,    0,   -6 },   // 0x34 '4'
  {    37,   5,   7,   6,    0,   -6 },   // 0x35 '5'
  {    42,   5,   7,   6,    0,   -6 },   // 0x36 '6'
  {    47,   5,   7,   6,    0,   -6 },   // 0x37 '7'
  {    52,   5,   7,   6,    0,   -6 },   // 0x38 '8'
  {    57,   5,   7,   6,    0,   -6 },   // 0x39 '9'
  {    62,   1,   7,   2,    0,   -6 },   // 0x3A ':'
  {     0,   0,   0,   0,    0,    0 },   // 0x3B ';'
  {     0,   0,   0,   0,    0,    0 },   // 0x3C '<'
  {     0,   0,   0,   0,    0,    0 },   // 0x3D '='
  {     0,   0,   0,   0,    0,    0 },   // 0x3E '>'
  {     0,   0,   0,   0,    0,    0 },   // 0x3F '?'
  {     0,   0,   0,   0,    0,    0 },   // 0x40 '@'
  {     0,   0,   0,   0,    0,    0 },   // 0x41 'A'
  {     0,   0,   0,   0,    0,    0 },   // 0x42 'B'
  {    64,   5,   7,   6,    0,   -6 },   // 0x43 'C'
  {     0,   0,   0,   0,    0,    0 },   // 0x44 'D'
  {     0,   0,   0,   0,    0,    0 },   // 0x45 'E'
  {     0,   0,   0,   0,    0,    0 },   // 0x46 'F'
  {     0,   0,   0,   0,    0,    0 },   // 0x47 'G'
  {     0,   0,   0,   0,    0,    0 },   // 0x48 'H'
  {     0,   0,   0,   0,    0,    0 },   // 0x49 'I'
  {     0,   0,   0,   0,    0,    0 },   // 0x4A 'J'
  {     0,   0,   0,   0,    0,    0 },   // 0x4B 'K'
  {     0,   0,   0,   0,    0,    0 },   // 0x4C 'L'
  {     0,   0,   0,   0,    0,    0 },   // 0x4D 'M'
  {     0,   0,   0,   0,    0,    0 },   // 0x4E 'N'
  {     0,   0,   0,   0,    0,    0 },   // 0x4F 'O'
  {     0,   0,   0,   0,    0,    0 },   // 0x50 'P'
  {     0,   0,   0,   0,    0,    0 },   // 0x51 'Q'
  {     0,   0,   0,   0,    0,    0 },   // 0x52 'R'
  {     0,   0,   0,   0,    0,    0 },   // 0x53 'S'
  {     0,   0,   0,   0,    0,    0 },   // 0x54 'T'
  {     0,   0,   0,   0,    0,    0 },   // 0x55 'U'
  {     0,   0,   0,   0,    0,    0 },   // 0x56 'V'
  {     0,   0,   0,   0,    0,    0 },   // 0x57 'W'
  {     0,   0,   0,   0,    0,    0 },   // 0x58 'X'
  {     0,   0,   0,   0,    0,    0 },   // 0x59 'Y'
  {     0,   0,   0,   0,    0,    0 },   // 0x5A 'Z'
  {     0,   0,   0,   0,    0,    0 },   // 0x5B '['
  {     0,   0,   0,   0,    0,    0 },   // 0x5C '\'
  {     0,   0,   0,   0,    0,    0 },   // 0x5D ']'
  {     0,   0,   0,   0,    0,    0 },   // 0x5E '^'
  {     0,   0,   0,   0,    0,    0 },   // 0x5F '_'
  {     0,   0,   0,   0,    0,    0 },   // 0x60 '`'
  {     0,   0,   0,   0,    0,    0 },   // 0x61 'a'
  {     0,   0,   0,   0,    0,    0 },   // 0x62 'b'
  {     0,   0,   0,   0,    0,    0 },   // 0x63 'c'
  {     0,   0,   0,   0,    0,    0 },   // 0x64 'd'
  {     0,   0,   0,   0,    0,    0 },   // 0x65 'e'
  {     0,   0,   0,   0,    0,    0 },   // 0x66 'f'
  {     0,   0,   0,   0,    0,    0 },   // 0x67 'g'
  {     0,   0,   0,   0,    0,    0 },   // 0x68 'h'
  {     0,   0,   0,   0,    0,    0 },   // 0x69 'i'
  {     0,   0,   0,   0,    0,    0 },   // 0x6A 'j'
  {     0,   0,   0,   0,    0,    0 },   // 0x6B 'k'
  {     0,   0,   0,   0,    0,    0 },   // 0x6C 'l'
  {     0,   0,   0,   0,    0,    0 },   // 0x6D 'm'
  {     0,   0,   0,   0,    0,    0 },   // 0x6E 'n'
  {     0,   0,   0,   0,    0,    0 },   // 0x6F 'o'
  {     0,   0,   0,   0,    0,    0 },   // 0x70 'p'
  {     0,   0,   0,   0,    0,    0 },   // 0x71 'q'
  {     0,   0,   0,   0,    0,    0 },   // 0x72 'r'
  {     0,   0,   0,   0,    0,    0 },   // 0x73 's'
  {     0,   0,   0,   0,    0,    0 },   // 0x74 't'
  {     0,   0,   0,   0,    0,    0 },   // 0x75 'u'
  {     0,   0,   0,   0,    0,    0 },   // 0x76 'v'
  {     0,   0,   0,   0,    0,    0 },   // 0x77 'w'
  {     0,   0,   0,   0,    0,    0 },   // 0x78 'x'
  {     0,   0,   0,   0,    0,    0 },   // 0x79 'y'
  {     0,   0,   0,   0,    0,    0 }    // 0x7A 'z'
};

const GFXfont square_big_font PROGMEM = {(uint8_t *) square_big_font_Bitmaps,        (GFXglyph *)square_big_font_Glyphs, 0x20, 0x7A,        8};
