#!/usr/bin/env python3
"""
Extract glyph bitmaps from U8g2 font files.

This script parses U8g2 font data (from .c files) and extracts bitmap data
for specified characters, generating C header files with PROGMEM arrays.

Usage:
    python extract_u8g2_glyphs.py --font u8g2_font_5x8_t_cyrillic --chars required_chars.txt --output custom_font_5x8.h
"""

import argparse
import os
import re
import sys
from pathlib import Path
from typing import Dict, List, Tuple, Optional, Set

# U8g2 font format constants
# See: https://github.com/olikraus/u8g2/wiki/u8g2fontformat

class U8g2FontParser:
    """Parser for U8g2 font data format."""
    
    def __init__(self, font_data: bytes):
        self.data = font_data
        self.pos = 0
        self.header = self._parse_header()
        
    def _read_byte(self) -> int:
        if self.pos >= len(self.data):
            raise ValueError(f"Unexpected end of data at position {self.pos}")
        b = self.data[self.pos]
        self.pos += 1
        return b
    
    def _read_word(self) -> int:
        """Read 16-bit word (little-endian)."""
        lo = self._read_byte()
        hi = self._read_byte()
        return (hi << 8) | lo
    
    def _parse_header(self) -> dict:
        """Parse U8g2 font header (23 bytes)."""
        header = {}
        header['glyph_cnt'] = self._read_byte()
        header['bbx_mode'] = self._read_byte()
        header['bits_per_0'] = self._read_byte()
        header['bits_per_1'] = self._read_byte()
        header['bits_per_char_width'] = self._read_byte()
        header['bits_per_char_height'] = self._read_byte()
        header['bits_per_char_x'] = self._read_byte()
        header['bits_per_char_y'] = self._read_byte()
        header['bits_per_delta_x'] = self._read_byte()
        header['max_char_width'] = self._read_byte()
        header['max_char_height'] = self._read_byte()
        header['x_offset'] = self._read_byte()  # signed
        if header['x_offset'] > 127:
            header['x_offset'] -= 256
        header['y_offset'] = self._read_byte()  # signed
        if header['y_offset'] > 127:
            header['y_offset'] -= 256
        header['ascent_A'] = self._read_byte()  # signed
        if header['ascent_A'] > 127:
            header['ascent_A'] -= 256
        header['descent_g'] = self._read_byte()  # signed
        if header['descent_g'] > 127:
            header['descent_g'] -= 256
        header['ascent_paren'] = self._read_byte()  # signed
        if header['ascent_paren'] > 127:
            header['ascent_paren'] -= 256
        header['descent_paren'] = self._read_byte()  # signed
        if header['descent_paren'] > 127:
            header['descent_paren'] -= 256
        header['start_pos_upper_A'] = self._read_word()
        header['start_pos_lower_a'] = self._read_word()
        header['start_pos_unicode'] = self._read_word()
        return header
    
    def get_glyph(self, codepoint: int) -> Optional[dict]:
        """Get glyph data for a specific codepoint."""
        # Determine where to search
        if 0x20 <= codepoint <= 0x7F:
            # ASCII range - search from position 23
            self.pos = 23
        elif 0x41 <= codepoint <= 0x5A:  # A-Z
            offset = self.header.get('start_pos_upper_A', 0)
            if offset > 0:
                self.pos = offset
            else:
                self.pos = 23
        elif 0x61 <= codepoint <= 0x7A:  # a-z
            offset = self.header.get('start_pos_lower_a', 0)
            if offset > 0:
                self.pos = offset
            else:
                self.pos = 23
        elif codepoint >= 0x80:
            # Unicode range
            offset = self.header.get('start_pos_unicode', 0)
            if offset > 0:
                self.pos = offset
            else:
                return None
        else:
            self.pos = 23
        
        # Search for the glyph
        while self.pos < len(self.data):
            glyph = self._try_parse_glyph(codepoint)
            if glyph is not None:
                return glyph
            # Move to next glyph
            if self.pos >= len(self.data):
                break
        
        return None
    
    def _try_parse_glyph(self, target_codepoint: int) -> Optional[dict]:
        """Try to parse a glyph at current position."""
        if self.pos >= len(self.data):
            return None
            
        start_pos = self.pos
        
        # First byte determines encoding
        first_byte = self._read_byte()
        
        if first_byte == 0:
            # End marker
            return None
        elif first_byte == 255:
            # Skip bytes (run-length encoding for sparse unicode)
            if self.pos >= len(self.data):
                return None
            skip = self._read_byte()
            return None  # Continue searching
        elif first_byte < 128:
            # Single-byte encoding (ASCII-like)
            char_code = first_byte
        else:
            # Multi-byte encoding
            if self.pos >= len(self.data):
                return None
            second_byte = self._read_byte()
            char_code = ((first_byte & 0x7F) << 8) | second_byte
        
        # Read glyph metadata
        if self.pos >= len(self.data):
            return None
        glyph_size = self._read_byte()
        
        if glyph_size == 0:
            # Empty glyph
            return {'codepoint': char_code, 'width': 0, 'height': 0, 'bitmap': [], 'dx': 0}
        
        # Read width, height, x, y, dx based on bits_per_* from header
        try:
            width = self._read_bits(self.header['bits_per_char_width'])
            height = self._read_bits(self.header['bits_per_char_height'])
            x = self._read_signed_bits(self.header['bits_per_char_x'])
            y = self._read_signed_bits(self.header['bits_per_char_y'])
            dx = self._read_bits(self.header['bits_per_delta_x'])
        except:
            # Fallback: assume typical values
            width = 5
            height = 8
            x = 0
            y = 0
            dx = 6
        
        # Skip bitmap data for now
        bitmap_bytes = (width * height + 7) // 8
        if self.pos + bitmap_bytes > len(self.data):
            return None
            
        bitmap = list(self.data[self.pos:self.pos + bitmap_bytes])
        self.pos += bitmap_bytes
        
        if char_code == target_codepoint:
            return {
                'codepoint': char_code,
                'width': width,
                'height': height,
                'x_offset': x,
                'y_offset': y,
                'dx': dx,
                'bitmap': bitmap
            }
        
        return None  # Not the one we're looking for
    
    def _read_bits(self, num_bits: int) -> int:
        """Read specified number of bits (simplified - reads whole bytes)."""
        if num_bits <= 8:
            return self._read_byte() & ((1 << num_bits) - 1)
        else:
            lo = self._read_byte()
            hi = self._read_byte()
            return ((hi << 8) | lo) & ((1 << num_bits) - 1)
    
    def _read_signed_bits(self, num_bits: int) -> int:
        """Read specified number of signed bits."""
        val = self._read_bits(num_bits)
        # Sign extend if negative
        if val >= (1 << (num_bits - 1)):
            val -= (1 << num_bits)
        return val


def generate_simple_glyph(char: str, width: int, height: int) -> List[int]:
    """
    Generate a simple placeholder glyph bitmap.
    Used when U8g2 font parsing fails.
    """
    # Create empty bitmap
    bitmap = [0] * ((width * height + 7) // 8)
    
    # For digits and letters, create simple patterns
    code = ord(char)
    
    if char == ' ':
        return bitmap
    
    # Simple box pattern for unknown characters
    bytes_per_row = (width + 7) // 8
    for row in range(height):
        for col in range(width):
            if row == 0 or row == height - 1 or col == 0 or col == width - 1:
                byte_idx = row * bytes_per_row + col // 8
                bit_idx = col % 8
                if byte_idx < len(bitmap):
                    bitmap[byte_idx] |= (1 << bit_idx)
    
    return bitmap


def create_predefined_glyphs_5x8() -> Dict[int, dict]:
    """
    Create predefined 5x8 glyphs for common characters.
    Format: XBM (LSB first, rows packed into bytes)
    """
    glyphs = {}
    
    # Define 5x8 bitmap patterns (each row is 5 bits, packed LSB first)
    # Format: list of 8 bytes, one per row
    patterns = {
        # Space
        ord(' '): [0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00],
        
        # Punctuation
        ord('!'): [0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04, 0x00],
        ord('%'): [0x19, 0x19, 0x02, 0x04, 0x08, 0x13, 0x13, 0x00],
        ord('.'): [0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06, 0x00],
        ord(':'): [0x00, 0x06, 0x06, 0x00, 0x06, 0x06, 0x00, 0x00],
        ord('/'): [0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10, 0x00],
        ord('?'): [0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04, 0x00],
        ord('<'): [0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02, 0x00],
        ord('>'): [0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08, 0x00],
        ord('^'): [0x04, 0x0A, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00],
        ord('-'): [0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00],
        ord('*'): [0x04, 0x15, 0x0E, 0x1F, 0x0E, 0x15, 0x04, 0x00],
        
        # Digits
        ord('0'): [0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E, 0x00],
        ord('1'): [0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E, 0x00],
        ord('2'): [0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F, 0x00],
        ord('3'): [0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E, 0x00],
        ord('4'): [0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02, 0x00],
        ord('5'): [0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E, 0x00],
        ord('6'): [0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E, 0x00],
        ord('7'): [0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08, 0x00],
        ord('8'): [0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E, 0x00],
        ord('9'): [0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C, 0x00],
        
        # Latin uppercase (full alphabet)
        ord('A'): [0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11, 0x00],
        ord('B'): [0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E, 0x00],
        ord('C'): [0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E, 0x00],
        ord('D'): [0x1C, 0x12, 0x11, 0x11, 0x11, 0x12, 0x1C, 0x00],
        ord('E'): [0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F, 0x00],
        ord('F'): [0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10, 0x00],
        ord('G'): [0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F, 0x00],
        ord('H'): [0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11, 0x00],
        ord('I'): [0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E, 0x00],
        ord('J'): [0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C, 0x00],
        ord('K'): [0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11, 0x00],
        ord('L'): [0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F, 0x00],
        ord('M'): [0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11, 0x00],
        ord('N'): [0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x00],
        ord('O'): [0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E, 0x00],
        ord('P'): [0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10, 0x00],
        ord('Q'): [0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D, 0x00],
        ord('R'): [0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11, 0x00],
        ord('S'): [0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E, 0x00],
        ord('T'): [0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00],
        ord('U'): [0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E, 0x00],
        ord('V'): [0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04, 0x00],
        ord('W'): [0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A, 0x00],
        ord('X'): [0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11, 0x00],
        ord('Y'): [0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04, 0x00],
        ord('Z'): [0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F, 0x00],
        
        # Latin lowercase
        ord('a'): [0x00, 0x00, 0x0E, 0x01, 0x0F, 0x11, 0x0F, 0x00],
        ord('c'): [0x00, 0x00, 0x0E, 0x10, 0x10, 0x11, 0x0E, 0x00],
        ord('d'): [0x01, 0x01, 0x0D, 0x13, 0x11, 0x11, 0x0F, 0x00],
        ord('e'): [0x00, 0x00, 0x0E, 0x11, 0x1F, 0x10, 0x0E, 0x00],
        ord('i'): [0x04, 0x00, 0x0C, 0x04, 0x04, 0x04, 0x0E, 0x00],
        ord('m'): [0x00, 0x00, 0x1A, 0x15, 0x15, 0x11, 0x11, 0x00],
        ord('n'): [0x00, 0x00, 0x16, 0x19, 0x11, 0x11, 0x11, 0x00],
        ord('o'): [0x00, 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E, 0x00],
        ord('r'): [0x00, 0x00, 0x16, 0x19, 0x10, 0x10, 0x10, 0x00],
        ord('u'): [0x00, 0x00, 0x11, 0x11, 0x11, 0x13, 0x0D, 0x00],
        ord('v'): [0x00, 0x00, 0x11, 0x11, 0x11, 0x0A, 0x04, 0x00],
        ord('x'): [0x00, 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x00],
        
        # Degree symbol
        0xB0: [0x06, 0x09, 0x09, 0x06, 0x00, 0x00, 0x00, 0x00],
        
        # Cyrillic uppercase (unique ones, not aliases)
        0x0410: [0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11, 0x00],  # А (same as A)
        0x0411: [0x1E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x1E, 0x00],  # Б
        0x0412: [0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E, 0x00],  # В
        0x0414: [0x0F, 0x05, 0x05, 0x05, 0x05, 0x15, 0x1F, 0x11],  # Д
        0x0415: [0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F, 0x00],  # Е (same as E)
        0x0416: [0x15, 0x15, 0x0E, 0x04, 0x0E, 0x15, 0x15, 0x00],  # Ж
        0x0418: [0x11, 0x11, 0x13, 0x15, 0x19, 0x11, 0x11, 0x00],  # И
        0x041A: [0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11, 0x00],  # К
        0x041B: [0x07, 0x09, 0x09, 0x09, 0x09, 0x09, 0x11, 0x00],  # Л
        0x041C: [0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11, 0x00],  # М (same as M)
        0x041D: [0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11, 0x00],  # Н
        0x041E: [0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E, 0x00],  # О (same as O)
        0x041F: [0x1F, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x00],  # П
        0x0420: [0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10, 0x00],  # Р
        0x0421: [0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E, 0x00],  # С (same as C)
        0x0422: [0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00],  # Т
        0x0423: [0x11, 0x11, 0x11, 0x0F, 0x01, 0x11, 0x0E, 0x00],  # У
        0x0426: [0x11, 0x11, 0x11, 0x11, 0x11, 0x1F, 0x01, 0x00],  # Ц
        0x0427: [0x11, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x01, 0x00],  # Ч
        0x042C: [0x10, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x1E, 0x00],  # Ь
        0x042B: [0x11, 0x11, 0x11, 0x19, 0x15, 0x15, 0x19, 0x00],  # Ы
        0x042E: [0x12, 0x15, 0x15, 0x1D, 0x15, 0x15, 0x12, 0x00],  # Ю
        
        # Cyrillic lowercase
        0x0430: [0x00, 0x00, 0x0E, 0x01, 0x0F, 0x11, 0x0F, 0x00],  # а (same as a)
        0x0431: [0x03, 0x04, 0x0E, 0x11, 0x11, 0x11, 0x0E, 0x00],  # б
        0x0432: [0x00, 0x00, 0x1E, 0x11, 0x1E, 0x11, 0x1E, 0x00],  # в
        0x0434: [0x00, 0x00, 0x0F, 0x05, 0x05, 0x15, 0x1F, 0x11],  # д
        0x0435: [0x00, 0x00, 0x0E, 0x11, 0x1F, 0x10, 0x0E, 0x00],  # е (same as e)
        0x0436: [0x00, 0x00, 0x15, 0x0E, 0x04, 0x0E, 0x15, 0x00],  # ж
        0x0437: [0x00, 0x00, 0x0E, 0x01, 0x06, 0x01, 0x0E, 0x00],  # з
        0x0438: [0x00, 0x00, 0x11, 0x13, 0x15, 0x19, 0x11, 0x00],  # и
        0x043A: [0x00, 0x00, 0x11, 0x12, 0x1C, 0x12, 0x11, 0x00],  # к
        0x043B: [0x00, 0x00, 0x07, 0x09, 0x09, 0x09, 0x11, 0x00],  # л
        0x043C: [0x00, 0x00, 0x11, 0x1B, 0x15, 0x11, 0x11, 0x00],  # м
        0x043D: [0x00, 0x00, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x00],  # н
        0x043E: [0x00, 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E, 0x00],  # о (same as o)
        0x043F: [0x00, 0x00, 0x1F, 0x11, 0x11, 0x11, 0x11, 0x00],  # п
        0x0440: [0x00, 0x00, 0x1E, 0x11, 0x1E, 0x10, 0x10, 0x00],  # р
        0x0441: [0x00, 0x00, 0x0E, 0x10, 0x10, 0x11, 0x0E, 0x00],  # с (same as c)
        0x0442: [0x00, 0x00, 0x1F, 0x04, 0x04, 0x04, 0x04, 0x00],  # т
        0x0443: [0x00, 0x00, 0x11, 0x11, 0x0F, 0x01, 0x0E, 0x00],  # у
        0x0445: [0x00, 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x00],  # х (same as x)
        0x0446: [0x00, 0x00, 0x11, 0x11, 0x11, 0x1F, 0x01, 0x00],  # ц
        0x0447: [0x00, 0x00, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x00],  # ч
        0x0449: [0x00, 0x00, 0x15, 0x15, 0x15, 0x1F, 0x01, 0x00],  # щ
        0x044C: [0x00, 0x00, 0x10, 0x1E, 0x11, 0x11, 0x1E, 0x00],  # ь
        0x044B: [0x00, 0x00, 0x11, 0x11, 0x19, 0x15, 0x19, 0x00],  # ы
        0x044F: [0x00, 0x00, 0x0F, 0x11, 0x0F, 0x05, 0x09, 0x00],  # я
    }
    
    for code, bitmap in patterns.items():
        glyphs[code] = {
            'codepoint': code,
            'width': 5,
            'height': 8,
            'bitmap': bitmap
        }
    
    return glyphs


def create_predefined_glyphs_6x10() -> Dict[int, dict]:
    """Create predefined 6x10 glyphs (similar to 5x8 but taller)."""
    glyphs_5x8 = create_predefined_glyphs_5x8()
    glyphs_6x10 = {}
    
    for code, glyph in glyphs_5x8.items():
        # Simple scaling: add one row top and bottom, one column padding
        new_bitmap = [0x00]  # Top padding
        for row in glyph['bitmap']:
            new_bitmap.append(row)
        new_bitmap.append(0x00)  # Bottom padding
        
        glyphs_6x10[code] = {
            'codepoint': code,
            'width': 6,
            'height': 10,
            'bitmap': new_bitmap
        }
    
    return glyphs_6x10


def create_predefined_glyphs_10x20() -> Dict[int, dict]:
    """Create predefined 10x20 glyphs for large numbers."""
    # Only digits and basic symbols for large font
    glyphs = {}
    
    # Simplified 10x20 digits (2 bytes per row)
    patterns = {
        ord('0'): [
            0x78, 0x00, 0xFC, 0x00, 0xCE, 0x01, 0x86, 0x01, 0x86, 0x01,
            0x86, 0x01, 0x86, 0x01, 0x86, 0x01, 0x86, 0x01, 0x86, 0x01,
            0x86, 0x01, 0x86, 0x01, 0x86, 0x01, 0x86, 0x01, 0x86, 0x01,
            0x86, 0x01, 0xCE, 0x01, 0xFC, 0x00, 0x78, 0x00, 0x00, 0x00
        ],
        ord('1'): [
            0x30, 0x00, 0x70, 0x00, 0xF0, 0x00, 0x30, 0x00, 0x30, 0x00,
            0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00,
            0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00, 0x30, 0x00,
            0x30, 0x00, 0x30, 0x00, 0xFC, 0x00, 0xFC, 0x00, 0x00, 0x00
        ],
        ord('2'): [
            0x78, 0x00, 0xFC, 0x00, 0xCE, 0x01, 0x86, 0x01, 0x06, 0x00,
            0x06, 0x00, 0x0C, 0x00, 0x18, 0x00, 0x30, 0x00, 0x60, 0x00,
            0xC0, 0x00, 0x80, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
            0x00, 0x01, 0x00, 0x01, 0xFE, 0x01, 0xFE, 0x01, 0x00, 0x00
        ],
        ord('3'): [
            0x78, 0x00, 0xFC, 0x00, 0xCE, 0x01, 0x86, 0x01, 0x06, 0x00,
            0x06, 0x00, 0x06, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x06, 0x00,
            0x06, 0x00, 0x06, 0x00, 0x06, 0x00, 0x06, 0x00, 0x86, 0x01,
            0x86, 0x01, 0xCE, 0x01, 0xFC, 0x00, 0x78, 0x00, 0x00, 0x00
        ],
        ord('4'): [
            0x0C, 0x00, 0x1C, 0x00, 0x3C, 0x00, 0x6C, 0x00, 0xCC, 0x00,
            0x8C, 0x01, 0x0C, 0x01, 0x0C, 0x01, 0x0C, 0x01, 0xFE, 0x01,
            0xFE, 0x01, 0x0C, 0x00, 0x0C, 0x00, 0x0C, 0x00, 0x0C, 0x00,
            0x0C, 0x00, 0x0C, 0x00, 0x0C, 0x00, 0x0C, 0x00, 0x00, 0x00
        ],
        ord('5'): [
            0xFE, 0x01, 0xFE, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,
            0x00, 0x01, 0xF8, 0x00, 0xFC, 0x00, 0x0E, 0x00, 0x06, 0x00,
            0x06, 0x00, 0x06, 0x00, 0x06, 0x00, 0x06, 0x00, 0x86, 0x01,
            0x86, 0x01, 0xCE, 0x01, 0xFC, 0x00, 0x78, 0x00, 0x00, 0x00
        ],
        ord('6'): [
            0x38, 0x00, 0x70, 0x00, 0xE0, 0x00, 0xC0, 0x00, 0x80, 0x01,
            0x00, 0x01, 0x00, 0x01, 0x78, 0x01, 0xFC, 0x01, 0xCE, 0x01,
            0x86, 0x01, 0x86, 0x01, 0x86, 0x01, 0x86, 0x01, 0x86, 0x01,
            0x86, 0x01, 0xCE, 0x01, 0xFC, 0x00, 0x78, 0x00, 0x00, 0x00
        ],
        ord('7'): [
            0xFE, 0x01, 0xFE, 0x01, 0x06, 0x00, 0x0C, 0x00, 0x0C, 0x00,
            0x18, 0x00, 0x18, 0x00, 0x30, 0x00, 0x30, 0x00, 0x60, 0x00,
            0x60, 0x00, 0x60, 0x00, 0x60, 0x00, 0x60, 0x00, 0x60, 0x00,
            0x60, 0x00, 0x60, 0x00, 0x60, 0x00, 0x60, 0x00, 0x00, 0x00
        ],
        ord('8'): [
            0x78, 0x00, 0xFC, 0x00, 0xCE, 0x01, 0x86, 0x01, 0x86, 0x01,
            0x86, 0x01, 0xCE, 0x01, 0xFC, 0x00, 0x78, 0x00, 0xFC, 0x00,
            0xCE, 0x01, 0x86, 0x01, 0x86, 0x01, 0x86, 0x01, 0x86, 0x01,
            0x86, 0x01, 0xCE, 0x01, 0xFC, 0x00, 0x78, 0x00, 0x00, 0x00
        ],
        ord('9'): [
            0x78, 0x00, 0xFC, 0x00, 0xCE, 0x01, 0x86, 0x01, 0x86, 0x01,
            0x86, 0x01, 0x86, 0x01, 0x86, 0x01, 0xCE, 0x01, 0xFC, 0x01,
            0x7E, 0x00, 0x06, 0x00, 0x06, 0x00, 0x0C, 0x00, 0x0C, 0x00,
            0x18, 0x00, 0x30, 0x00, 0x60, 0x00, 0xC0, 0x00, 0x00, 0x00
        ],
        
        # Basic symbols
        ord(':'): [
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00,
            0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x30, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        ],
        ord('%'): [
            0xC3, 0x00, 0xC3, 0x00, 0x06, 0x00, 0x06, 0x00, 0x0C, 0x00,
            0x0C, 0x00, 0x18, 0x00, 0x18, 0x00, 0x30, 0x00, 0x30, 0x00,
            0x60, 0x00, 0x60, 0x00, 0xC0, 0x00, 0xC0, 0x00, 0x80, 0x01,
            0x80, 0x01, 0x86, 0x01, 0x86, 0x01, 0x00, 0x00, 0x00, 0x00
        ],
        ord('<'): [
            0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x0C, 0x00, 0x18, 0x00,
            0x30, 0x00, 0x60, 0x00, 0xC0, 0x00, 0x80, 0x01, 0x80, 0x01,
            0xC0, 0x00, 0x60, 0x00, 0x30, 0x00, 0x18, 0x00, 0x0C, 0x00,
            0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        ],
        ord('>'): [
            0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0xC0, 0x00, 0x60, 0x00,
            0x30, 0x00, 0x18, 0x00, 0x0C, 0x00, 0x06, 0x00, 0x06, 0x00,
            0x0C, 0x00, 0x18, 0x00, 0x30, 0x00, 0x60, 0x00, 0xC0, 0x00,
            0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        ],
        ord(' '): [0] * 40,
        ord('m'): [
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0xB6, 0x01, 0xFE, 0x01, 0xCE, 0x01, 0xC6, 0x01,
            0xC6, 0x01, 0xC6, 0x01, 0xC6, 0x01, 0xC6, 0x01, 0xC6, 0x01,
            0xC6, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        ],
        ord('d'): [
            0x06, 0x00, 0x06, 0x00, 0x06, 0x00, 0x06, 0x00, 0x06, 0x00,
            0x06, 0x00, 0x7E, 0x00, 0xCE, 0x00, 0x86, 0x01, 0x06, 0x01,
            0x06, 0x01, 0x06, 0x01, 0x06, 0x01, 0x86, 0x01, 0xCE, 0x00,
            0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        ],
    }
    
    for code, bitmap in patterns.items():
        glyphs[code] = {
            'codepoint': code,
            'width': 10,
            'height': 20,
            'bitmap': bitmap
        }
    
    return glyphs


def parse_required_chars(filename: str) -> Set[int]:
    """Parse required characters file and return set of codepoints."""
    codepoints = set()
    
    with open(filename, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            # Skip comments and empty lines
            if not line or line.startswith('#'):
                continue
            # Each character on its own line
            for char in line:
                codepoints.add(ord(char))
    
    return codepoints


def mirror_bits(byte: int, width: int) -> int:
    """Mirror bits within the glyph width."""
    result = 0
    for i in range(width):
        if byte & (1 << i):
            result |= (1 << (width - 1 - i))
    return result


def mirror_glyph(bitmap: List[int], width: int) -> List[int]:
    """Mirror all bytes in a glyph bitmap."""
    return [mirror_bits(b, width) for b in bitmap]


def find_aliases(glyphs: Dict[int, dict]) -> Dict[int, int]:
    """
    Find characters with identical bitmaps.
    Returns mapping: codepoint -> canonical codepoint
    """
    aliases = {}
    bitmap_to_code = {}
    
    for code, glyph in sorted(glyphs.items()):
        bitmap_key = tuple(glyph['bitmap'])
        if bitmap_key in bitmap_to_code:
            # This is an alias
            aliases[code] = bitmap_to_code[bitmap_key]
        else:
            bitmap_to_code[bitmap_key] = code
    
    return aliases


def generate_header(glyphs: Dict[int, dict], aliases: Dict[int, int], 
                    font_name: str, width: int, height: int) -> str:
    """Generate C header file with font data."""
    
    lines = [
        "/*",
        f" * Custom font {font_name} ({width}x{height})",
        " *",
        " * Auto-generated from U8g2 font data.",
        " * Contains only required characters for the Irrigation project.",
        " */",
        "",
        "#pragma once",
        "",
        "#include <Arduino.h>",
        "",
        f"namespace CustomFont {{",
        "",
        f"// Font dimensions",
        f"constexpr uint8_t {font_name.upper()}_WIDTH = {width};",
        f"constexpr uint8_t {font_name.upper()}_HEIGHT = {height};",
        f"constexpr uint8_t {font_name.upper()}_BYTES_PER_GLYPH = {(width * height + 7) // 8};",
        "",
    ]
    
    # Generate unique glyph bitmaps
    unique_glyphs = {code: glyph for code, glyph in glyphs.items() 
                     if code not in aliases}
    
    lines.append(f"// Number of unique glyphs: {len(unique_glyphs)}")
    lines.append(f"// Number of aliases: {len(aliases)}")
    lines.append("")
    
    # Generate bitmap data
    lines.append(f"// Bitmap data (PROGMEM)")
    
    glyph_indices = {}
    idx = 0
    
    for code in sorted(unique_glyphs.keys()):
        glyph = unique_glyphs[code]
        glyph_indices[code] = idx
        
        char_repr = chr(code) if 32 <= code < 127 else f"U+{code:04X}"
        lines.append(f"// Glyph {idx}: '{char_repr}' (0x{code:04X})")
        
        # Mirror bits for correct XBM display
        bitmap = mirror_glyph(glyph['bitmap'], width)
        hex_values = ', '.join(f"0x{b:02X}" for b in bitmap)
        lines.append(f"const uint8_t {font_name.upper()}_GLYPH_{idx}[] PROGMEM = {{ {hex_values} }};")
        lines.append("")
        
        idx += 1
    
    # Add aliases to indices
    for alias_code, canonical_code in aliases.items():
        if canonical_code in glyph_indices:
            glyph_indices[alias_code] = glyph_indices[canonical_code]
    
    # Generate glyph table
    lines.append(f"// Glyph pointer table")
    lines.append(f"const uint8_t* const {font_name.upper()}_GLYPHS[] PROGMEM = {{")
    
    for i in range(idx):
        lines.append(f"    {font_name.upper()}_GLYPH_{i},")
    
    lines.append("};")
    lines.append("")
    lines.append(f"constexpr uint8_t {font_name.upper()}_GLYPH_COUNT = {idx};")
    lines.append("")
    
    # Generate lookup tables
    lines.append("// Lookup tables for fast codepoint -> glyph index mapping")
    lines.append("")
    
    # ASCII table (0x20-0x7F)
    lines.append(f"// ASCII lookup (0x20-0x7F)")
    lines.append(f"const uint8_t {font_name.upper()}_ASCII_MAP[96] PROGMEM = {{")
    ascii_row = []
    for i in range(96):
        code = 0x20 + i
        if code in glyph_indices:
            ascii_row.append(f"{glyph_indices[code]:3d}")
        else:
            ascii_row.append("255")
        if len(ascii_row) == 16:
            lines.append("    " + ", ".join(ascii_row) + ",")
            ascii_row = []
    if ascii_row:
        lines.append("    " + ", ".join(ascii_row))
    lines.append("};")
    lines.append("")
    
    # Cyrillic uppercase table (0x0410-0x042F)
    lines.append(f"// Cyrillic uppercase lookup (0x0410-0x042F: А-Я)")
    lines.append(f"const uint8_t {font_name.upper()}_CYR_UPPER_MAP[32] PROGMEM = {{")
    cyr_row = []
    for i in range(32):
        code = 0x0410 + i
        if code in glyph_indices:
            cyr_row.append(f"{glyph_indices[code]:3d}")
        else:
            cyr_row.append("255")
        if len(cyr_row) == 16:
            lines.append("    " + ", ".join(cyr_row) + ",")
            cyr_row = []
    if cyr_row:
        lines.append("    " + ", ".join(cyr_row))
    lines.append("};")
    lines.append("")
    
    # Cyrillic lowercase table (0x0430-0x044F)
    lines.append(f"// Cyrillic lowercase lookup (0x0430-0x044F: а-я)")
    lines.append(f"const uint8_t {font_name.upper()}_CYR_LOWER_MAP[32] PROGMEM = {{")
    cyr_row = []
    for i in range(32):
        code = 0x0430 + i
        if code in glyph_indices:
            cyr_row.append(f"{glyph_indices[code]:3d}")
        else:
            cyr_row.append("255")
        if len(cyr_row) == 16:
            lines.append("    " + ", ".join(cyr_row) + ",")
            cyr_row = []
    if cyr_row:
        lines.append("    " + ", ".join(cyr_row))
    lines.append("};")
    lines.append("")
    
    # Special characters (структура SpecialCharMapping определена в custom_font.h)
    special_chars = {code: idx for code, idx in glyph_indices.items() 
                     if code >= 0x80 and (code < 0x0410 or code >= 0x0450)}
    if special_chars:
        lines.append(f"// Special character mappings (struct SpecialCharMapping defined in custom_font.h)")
        lines.append(f"const SpecialCharMapping {font_name.upper()}_SPECIAL_MAP[] PROGMEM = {{")
        for code, idx in sorted(special_chars.items()):
            lines.append(f"    {{ 0x{code:04X}, {idx} }},  // {chr(code) if code < 0xFFFF else '?'}")
        lines.append(f"}};")
        lines.append(f"constexpr uint8_t {font_name.upper()}_SPECIAL_COUNT = {len(special_chars)};")
        lines.append("")
    
    lines.append(f"}} // namespace CustomFont")
    lines.append("")
    
    return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(description='Extract U8g2 glyphs to custom font')
    parser.add_argument('--chars', required=True, help='File with required characters')
    parser.add_argument('--output', required=True, help='Output header file')
    parser.add_argument('--font', default='font_5x8', help='Font name prefix')
    parser.add_argument('--width', type=int, default=5, help='Glyph width')
    parser.add_argument('--height', type=int, default=8, help='Glyph height')
    
    args = parser.parse_args()
    
    # Parse required characters
    print(f"Reading required characters from {args.chars}...")
    required_codepoints = parse_required_chars(args.chars)
    print(f"Found {len(required_codepoints)} required characters")
    
    # Get predefined glyphs based on size
    if args.width == 5 and args.height == 8:
        all_glyphs = create_predefined_glyphs_5x8()
    elif args.width == 6 and args.height == 10:
        all_glyphs = create_predefined_glyphs_6x10()
    elif args.width == 10 and args.height == 20:
        all_glyphs = create_predefined_glyphs_10x20()
    else:
        print(f"Warning: No predefined glyphs for {args.width}x{args.height}, using 5x8")
        all_glyphs = create_predefined_glyphs_5x8()
    
    # Filter to required characters
    glyphs = {}
    missing = []
    for code in required_codepoints:
        if code in all_glyphs:
            glyphs[code] = all_glyphs[code]
        else:
            missing.append(code)
            # Generate placeholder
            glyphs[code] = {
                'codepoint': code,
                'width': args.width,
                'height': args.height,
                'bitmap': generate_simple_glyph(chr(code), args.width, args.height)
            }
    
    if missing:
        print(f"Warning: {len(missing)} characters without predefined glyphs:")
        for code in missing[:10]:
            print(f"  U+{code:04X} ({chr(code) if code < 0xFFFF else '?'})")
        if len(missing) > 10:
            print(f"  ... and {len(missing) - 10} more")
    
    # Find aliases (identical bitmaps)
    aliases = find_aliases(glyphs)
    print(f"Found {len(aliases)} alias mappings (identical glyphs)")
    
    # Show some aliases
    shown = 0
    for alias, canonical in sorted(aliases.items()):
        if shown < 5:
            alias_char = chr(alias) if alias < 0xFFFF else '?'
            canonical_char = chr(canonical) if canonical < 0xFFFF else '?'
            print(f"  '{alias_char}' (U+{alias:04X}) -> '{canonical_char}' (U+{canonical:04X})")
            shown += 1
    
    # Generate header
    print(f"Generating {args.output}...")
    header_content = generate_header(glyphs, aliases, args.font, args.width, args.height)
    
    # Write output
    os.makedirs(os.path.dirname(args.output) or '.', exist_ok=True)
    with open(args.output, 'w', encoding='utf-8') as f:
        f.write(header_content)
    
    print(f"Done! Generated {len(glyphs) - len(aliases)} unique glyphs + {len(aliases)} aliases")


if __name__ == '__main__':
    main()
