#!/usr/bin/env python3
"""
Конвертирует иконки из JSON формата в побитовый C++ формат для PROGMEM.

Формат: каждый байт содержит 8 пикселей (MSB = левый пиксель).
"""

import json
import sys
from pathlib import Path


def convert_string_to_bits(row: str) -> list[int]:
    """Конвертирует строку '0' и '1' в список битов."""
    return [1 if c == '1' else 0 for c in row]


def pack_bits_to_bytes(bits: list[int], width: int) -> list[int]:
    """
    Упаковывает биты в байты.
    Каждый байт содержит 8 пикселей (LSB = левый пиксель для U8g2 drawXBMP).
    U8g2 drawXBMP ожидает формат XBM с порядком битов LSB-first.
    """
    bytes_list = []
    for i in range(0, len(bits), 8):
        byte_val = 0
        for j in range(8):
            if i + j < len(bits):
                if bits[i + j]:
                    byte_val |= (1 << j)  # LSB first (для U8g2 drawXBMP)
        bytes_list.append(byte_val)
    return bytes_list


def calculate_bytes_count(width: int, height: int) -> int:
    """Вычисляет количество байт для иконки заданного размера."""
    bits_count = width * height
    return (bits_count + 7) // 8  # Округление вверх


def convert_icon(name: str, rows: list[str]) -> tuple[int, int, list[int]]:
    """
    Конвертирует иконку из строкового формата в побитовый.
    
    XBM формат: каждая строка упаковывается отдельно в байты.
    Если ширина не кратна 8, остаток строки дополняется нулями.
    """
    if not rows:
        return 0, 0, []
    
    height = len(rows)
    width = len(rows[0]) if rows else 0
    
    # Проверка, что все строки одинаковой длины
    for row in rows:
        if len(row) != width:
            raise ValueError(f"Icon '{name}': inconsistent row length")
    
    # Упаковываем каждую строку отдельно (XBM формат)
    bytes_data = []
    for row in rows:
        bits = convert_string_to_bits(row)
        # Упаковываем биты строки в байты
        row_bytes = pack_bits_to_bytes(bits, width)
        bytes_data.extend(row_bytes)
    
    return width, height, bytes_data


def generate_cpp_header(icons_data: dict[str, tuple[int, int, list[int]]]) -> str:
    """Генерирует C++ заголовочный файл с иконками в PROGMEM."""
    
    lines = [
        "/*",
        " * Icons for main screen display",
        " *",
        " * Generated from tools/display/main_screen_icons.json",
        " * Format: 1 bit per pixel, packed into bytes (LSB = left pixel for U8g2 drawXBMP)",
        " * All icons stored in PROGMEM to save SRAM",
        " */",
        "",
        "#pragma once",
        "",
        "#include <Arduino.h>",
        "",
        "namespace Display::Icons {",
        "",
        "struct IconData {",
        "    const uint8_t* data;",
        "    uint8_t width;",
        "    uint8_t height;",
        "};",
        "",
    ]
    
    # Генерируем массивы данных для каждой иконки
    for name, (width, height, bytes_data) in icons_data.items():
        var_name = f"ICON_{name.upper().replace('-', '_')}"
        lines.append(f"// Icon: {name} ({width}x{height})")
        lines.append(f"static const uint8_t {var_name}_DATA[] PROGMEM = {{")
        
        # Форматируем байты по 16 в строке
        for i in range(0, len(bytes_data), 16):
            chunk = bytes_data[i:i+16]
            hex_values = ", ".join(f"0x{b:02X}" for b in chunk)
            lines.append(f"    {hex_values},")
        
        lines.append("};")
        lines.append(f"static const IconData {var_name} = {{")
        lines.append(f"    {var_name}_DATA,")
        lines.append(f"    {width},")
        lines.append(f"    {height}")
        lines.append("};")
        lines.append("")
    
    # Генерируем функцию getIcon
    lines.append("/**")
    lines.append(" * Получает данные иконки по имени.")
    lines.append(" *")
    lines.append(" * @param name имя иконки")
    lines.append(" * @return указатель на IconData или nullptr, если иконка не найдена")
    lines.append(" */")
    lines.append("inline const IconData* getIcon(const char* name) {")
    
    # Генерируем switch statement
    lines.append("    if (!name) return nullptr;")
    lines.append("")
    
    for name in icons_data.keys():
        var_name = f"ICON_{name.upper().replace('-', '_')}"
        lines.append(f'    if (strcmp_P(name, PSTR("{name}")) == 0) {{')
        lines.append(f"        return &{var_name};")
        lines.append("    }")
    
    lines.append("    return nullptr;")
    lines.append("}")
    lines.append("")
    lines.append("}  // namespace Display::Icons")
    lines.append("")
    
    return "\n".join(lines)


def main():
    """Основная функция."""
    script_dir = Path(__file__).parent
    json_path = script_dir / "main_screen_icons.json"
    output_path = script_dir.parent.parent / "src" / "display" / "main_screen_icons.h"
    
    # Читаем JSON
    with open(json_path, "r", encoding="utf-8") as f:
        icons_json = json.load(f)
    
    # Конвертируем все иконки
    icons_data = {}
    for name, rows in icons_json.items():
        width, height, bytes_data = convert_icon(name, rows)
        icons_data[name] = (width, height, bytes_data)
        print(f"Converted '{name}': {width}x{height}, {len(bytes_data)} bytes")
    
    # Генерируем C++ код
    cpp_code = generate_cpp_header(icons_data)
    
    # Записываем в файл
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(cpp_code)
    
    print(f"\nGenerated: {output_path}")
    print(f"Total icons: {len(icons_data)}")
    
    # Подсчитываем общий размер
    total_bytes = sum(len(bytes_data) for _, _, bytes_data in icons_data.values())
    print(f"Total size: {total_bytes} bytes ({total_bytes / 1024:.2f} KB)")


if __name__ == "__main__":
    main()

