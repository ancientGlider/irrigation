import json
import socketserver
import time
from http.server import SimpleHTTPRequestHandler
from urllib.parse import urlparse


WIDTH = 128
HEIGHT = 64
SCALE = 4
ICONS_PATH = "tools/display/main_screen_icons.json"


class FrameBuffer:
    def __init__(self, width: int, height: int) -> None:
        self.width = width
        self.height = height
        self.buffer = [[0 for _ in range(width)] for _ in range(height)]

    def set_pixel(self, x: int, y: int, value: int = 1) -> None:
        if 0 <= x < self.width and 0 <= y < self.height:
            self.buffer[y][x] = 1 if value else 0

    def draw_hline(self, x: int, y: int, length: int, value: int = 1) -> None:
        for dx in range(length):
            self.set_pixel(x + dx, y, value)

    def fill_rect(self, x: int, y: int, w: int, h: int, value: int = 1) -> None:
        for dy in range(h):
            for dx in range(w):
                self.set_pixel(x + dx, y + dy, value)

    def to_list(self) -> list[int]:
        flat = []
        for row in self.buffer:
            flat.extend(row)
        return flat


def load_font() -> dict[str, list[str]]:
    return {
        " ": ["00000"] * 8,
        "!": ["00100", "00100", "00100", "00100", "00100", "00000", "00100", "00000"],
        ":": ["00000", "00100", "00000", "00000", "00000", "00100", "00000", "00000"],
        ".": ["00000", "00000", "00000", "00000", "00000", "00000", "00100", "00000"],
        "/": ["00001", "00010", "00100", "01000", "10000", "00000", "00000", "00000"],
        "0": ["01110", "10001", "10011", "10101", "11001", "10001", "01110", "00000"],
        "1": ["00100", "01100", "00100", "00100", "00100", "00100", "01110", "00000"],
        "2": ["01110", "10001", "00001", "00110", "01000", "10000", "11111", "00000"],
        "3": ["01110", "10001", "00001", "00110", "00001", "10001", "01110", "00000"],
        "4": ["00010", "00110", "01010", "10010", "11111", "00010", "00010", "00000"],
        "5": ["11111", "10000", "11110", "00001", "00001", "10001", "01110", "00000"],
        "6": ["00110", "01000", "10000", "11110", "10001", "10001", "01110", "00000"],
        "7": ["11111", "00001", "00010", "00100", "01000", "01000", "01000", "00000"],
        "8": ["01110", "10001", "10001", "01110", "10001", "10001", "01110", "00000"],
        "9": ["01110", "10001", "10001", "01111", "00001", "00010", "01100", "00000"],
        "A": ["01110", "10001", "10001", "11111", "10001", "10001", "10001", "00000"],
        "C": ["01110", "10001", "10000", "10000", "10000", "10001", "01110", "00000"],
        "D": ["11110", "10001", "10001", "10001", "10001", "10001", "11110", "00000"],
        "E": ["11111", "10000", "10000", "11110", "10000", "10000", "11111", "00000"],
        "F": ["11111", "10000", "10000", "11110", "10000", "10000", "10000", "00000"],
        "G": ["01110", "10001", "10000", "10111", "10001", "10001", "01110", "00000"],
        "H": ["10001", "10001", "10001", "11111", "10001", "10001", "10001", "00000"],
        "I": ["01110", "00100", "00100", "00100", "00100", "00100", "01110", "00000"],
        "K": ["10001", "10010", "10100", "11000", "10100", "10010", "10001", "00000"],
        "L": ["10000", "10000", "10000", "10000", "10000", "10000", "11111", "00000"],
        "M": ["10001", "11011", "10101", "10101", "10001", "10001", "10001", "00000"],
        "N": ["10001", "11001", "10101", "10011", "10001", "10001", "10001", "00000"],
        "O": ["01110", "10001", "10001", "10001", "10001", "10001", "01110", "00000"],
        "P": ["11110", "10001", "10001", "11110", "10000", "10000", "10000", "00000"],
        "R": ["11110", "10001", "10001", "11110", "10100", "10010", "10001", "00000"],
        "S": ["01111", "10000", "10000", "01110", "00001", "00001", "11110", "00000"],
        "T": ["11111", "00100", "00100", "00100", "00100", "00100", "00100", "00000"],
        "U": ["10001", "10001", "10001", "10001", "10001", "10001", "01110", "00000"],
        "V": ["10001", "10001", "10001", "10001", "10001", "01010", "00100", "00000"],
        "W": ["10001", "10001", "10001", "10101", "10101", "10101", "01010", "00000"],
        "X": ["10001", "10001", "01010", "00100", "01010", "10001", "10001", "00000"],
        "Y": ["10001", "10001", "01010", "00100", "00100", "00100", "00100", "00000"],
        "Р": ["11110", "10001", "10001", "11110", "10000", "10000", "10000", "00000"],
        "О": ["01110", "10001", "10001", "10001", "10001", "10001", "01110", "00000"],
        "С": ["01110", "10001", "10000", "10000", "10000", "10001", "01110", "00000"],
        "Т": ["11111", "00100", "00100", "00100", "00100", "00100", "00100", "00000"],
        "В": ["11110", "10001", "10001", "11110", "10001", "10001", "11110", "00000"],
        "Е": ["11111", "10000", "10000", "11110", "10000", "10000", "11111", "00000"],
        "Л": ["00111", "01001", "01001", "01001", "01001", "01001", "11001", "00000"],
        "Д": ["01111", "01001", "01001", "01001", "01001", "01001", "11111", "00000"],
        "Н": ["10001", "10001", "10001", "11111", "10001", "10001", "10001", "00000"],
        "Ь": ["10000", "10000", "10000", "11110", "10001", "10001", "11110", "00000"],
        "А": ["01110", "10001", "10001", "11111", "10001", "10001", "10001", "00000"],
        "П": ["11111", "10001", "10001", "10001", "10001", "10001", "10001", "00000"],
        "И": ["10001", "10001", "10011", "10101", "11001", "10001", "10001", "00000"],
        "°": ["00100", "01010", "01010", "00100", "00000", "00000", "00000", "00000"],
        "%": ["11001", "11010", "00100", "00100", "01011", "10011", "00000", "00000"],
    }


FONT_5X8 = load_font()


def load_icons(path: str) -> dict[str, list[str]]:
    with open(path, "r", encoding="utf-8") as fh:
        raw = json.load(fh)
    icons: dict[str, list[str]] = {}
    for name, rows in raw.items():
        pattern: list[str] = []
        for row in rows:
            normalized = "".join("#" if c in ("1", "#", "X") else "." for c in row)
            pattern.append(normalized)
        icons[name] = pattern
    return icons


ICONS = load_icons(ICONS_PATH)


def draw_text(fb: FrameBuffer, x: int, y: int, text: str, invert: bool = False) -> None:
    cursor_x = x
    glyph_height = len(next(iter(FONT_5X8.values())))
    glyph_width = len(next(iter(FONT_5X8.values()))[0])
    for ch in text:
        glyph = FONT_5X8.get(ch, FONT_5X8[" "])
        for row in range(glyph_height):
            for col in range(glyph_width):
                if glyph[row][col] == "1":
                    fb.set_pixel(cursor_x + col, y + row, 0 if invert else 1)
        cursor_x += glyph_width + 1


def draw_icon(
    fb: FrameBuffer,
    x: int,
    y: int,
    name: str,
    *,
    invert: bool = False,
    max_width: int | None = None,
    max_height: int | None = None,
) -> None:
    pattern = ICONS.get(name)
    if not pattern:
        return
    height = len(pattern) if max_height is None else min(len(pattern), max_height)
    width = len(pattern[0]) if pattern else 0
    if max_width is not None:
        width = min(width, max_width)
    if invert:
        fb.fill_rect(x, y, width, height, 1)
    for row in range(height):
        line = pattern[row]
        for col in range(width):
            pixel = line[col]
            if pixel == "#":
                fb.set_pixel(x + col, y + row, 0 if invert else 1)


def render_screen() -> FrameBuffer:
    fb = FrameBuffer(WIDTH, HEIGHT)

    now = time.localtime()
    blink_state = (int(time.time()) % 2) == 0
    colon_on = True  # временно отображаем двоеточие постоянно
    info_blink_state = blink_state

    mode = "РОСТ"  # возможные значения: РОСТ/ВЕСНА/ЛЕТО/ОСЕНЬ
    light_on_state = True
    pump_alert = True
    attention_alert = True

    # === Первый блок ===
    # Поле режима (0..29)
    mode_field_width = 30
    fb.fill_rect(0, 0, mode_field_width, 8, 1)
    draw_text(fb, 2, 0, mode[:4], invert=True)

    # Поле информации (30..115)
    info_start = mode_field_width
    info_end = 116
    info_width = info_end - info_start

    light_icon_name = "light_on" if light_on_state else "light_off"
    light_width = len(ICONS[light_icon_name][0]) if light_icon_name in ICONS else 0

    hours = f"{now.tm_hour:02d}"
    minutes = f"{now.tm_min:02d}"
    glyph_width = len(next(iter(FONT_5X8.values()))[0])
    hours_width = len(hours) * (glyph_width + 1) - 1
    colon_width = glyph_width
    minutes_width = len(minutes) * (glyph_width + 1) - 1
    time_width = hours_width + 1 + colon_width + 1 + minutes_width

    space_count = 3
    available = info_width - light_width - time_width
    gap = max(0, available // space_count)

    light_x = info_start + gap
    draw_icon(fb, light_x, 0, light_icon_name)

    time_x = light_x + light_width + gap
    light_x = max(info_start, light_x - 1)
    time_x = max(info_start, time_x - 1)

    draw_text(fb, time_x, 0, hours)
    colon_x = time_x + hours_width + 1
    if colon_on:
        draw_text(fb, colon_x, 0, ":")
    minutes_x = colon_x + colon_width + 1
    draw_text(fb, minutes_x, 0, minutes)

    # Поля оповещений
    pump_icon_width = len(ICONS["pump_on"][0]) if "pump_on" in ICONS and ICONS["pump_on"] else 0
    field_width = max(pump_icon_width, 6)
    alert_pump_x = 113
    alert_attention_x = 122
    field_height = 8

    if pump_alert:
        draw_icon(fb, alert_pump_x, 0, "pump_on", max_width=field_width)

    if attention_alert:
        fb.fill_rect(alert_attention_x, 0, field_width, field_height, 1)
        draw_text(fb, alert_attention_x, 0, "!", invert=True)

    fb.draw_hline(0, 9, WIDTH)

    # === Второй блок ===
    left_width = 64
    right_start = left_width

    watering_state = 1  # 1..6
    state_map = {
        1: ["АВТО", "ПОЛИВ"],
        2: ["ОЖИДАНИЕ"],
        3: ["ТРЕНИРОВКА"],
        4: ["ОЖИДАНИЕ", "ТРЕНИРОВКИ"],
        5: ["СТОП"],
        6: ["НЕТ", "ВОДЫ"],
    }
    state_lines = state_map.get(watering_state, ["АВТО", "ПОЛИВ"])

    y_lines = [11, 20, 29]
    for idx, line in enumerate(state_lines[:2]):
        draw_text(fb, 2, y_lines[idx], line[:12])

    sensors_seconds = 255
    sensors_mm = sensors_seconds // 60
    sensors_ss = sensors_seconds % 60
    sensors_time = f"{sensors_mm:02d}:{sensors_ss:02d}"
    draw_icon(fb, 2, y_lines[2], "magnifier")
    draw_text(fb, 18, y_lines[2], sensors_time)

    temp_value = 23.4
    humidity_air = 52
    humidity_soil = 68

    draw_icon(fb, right_start + 2, y_lines[0], "thermometer")
    draw_icon(fb, right_start + 12, y_lines[0], "air")
    temp_text = f"{temp_value:.1f}"
    temp_text_x = right_start + 24
    draw_text(fb, temp_text_x, y_lines[0], temp_text)
    temp_text_width = len(temp_text) * (glyph_width + 1)
    draw_text(fb, temp_text_x + temp_text_width, y_lines[0], "°")
    draw_text(fb, temp_text_x + temp_text_width + (glyph_width + 1), y_lines[0], "C")

    draw_icon(fb, right_start + 2, y_lines[1], "drop")
    draw_icon(fb, right_start + 12, y_lines[1], "air")
    draw_text(fb, right_start + 24, y_lines[1], f"{humidity_air}")
    draw_text(fb, right_start + 24 + len(str(humidity_air)) * (glyph_width + 1), y_lines[1], "%")

    draw_icon(fb, right_start + 2, y_lines[2], "drop")
    draw_icon(fb, right_start + 12, y_lines[2], "earth")
    draw_text(fb, right_start + 24, y_lines[2], f"{humidity_soil}")
    draw_text(fb, right_start + 24 + len(str(humidity_soil)) * (glyph_width + 1), y_lines[2], "%")

    fb.draw_hline(0, 38, WIDTH)

    # === Третий блок ===
    season_name = "ВЕСНА"  # варианты: ВЕСНА/ЛЕТО/ОСЕНЬ
    current_day = 24
    total_days = 120

    line_chars = [" "] * 21

    season_padded = season_name.ljust(5)
    for idx, ch in enumerate(season_padded[:5]):
        line_chars[idx] = ch

    label = "ДЕНЬ: "
    for idx, ch in enumerate(label):
        line_chars[8 + idx] = ch

    day_field = f"{current_day:>3}"
    for idx, ch in enumerate(day_field):
        line_chars[14 + idx] = ch

    line_chars[17] = "/"

    total_field = f"{total_days:>3}"
    for idx, ch in enumerate(total_field):
        line_chars[18 + idx] = ch

    line_text = "".join(line_chars)
    draw_text(fb, 0, 40, line_text)

    fb.draw_hline(0, 49, WIDTH)

    # === Блок иконок ===
    button_y = 51
    button_icons = ["settings", "watering", "check", "on_off"]
    icon_widths = [
        len(ICONS[name][0]) if name in ICONS and ICONS[name] else 0
        for name in button_icons
    ]
    total_icons_width = sum(icon_widths)
    gaps = len(button_icons) - 1
    gap_space = WIDTH - total_icons_width
    if gaps > 0:
        base_gap = max(0, gap_space // gaps)
        extra_gap = gap_space - base_gap * gaps
    else:
        base_gap = 0
        extra_gap = 0

    x = 0
    for idx, name in enumerate(button_icons):
        draw_icon(fb, x, button_y, name)
        if idx < len(button_icons) - 1:
            gap = base_gap + (1 if extra_gap > 0 else 0)
            if extra_gap > 0:
                extra_gap -= 1
            x += icon_widths[idx] + gap

    return fb


HTML_PAGE = f"""<!DOCTYPE html>
<html lang="ru">
<head>
  <meta charset="UTF-8">
  <title>OLED 128x64 Preview</title>
  <style>
    body {{
      background: #111;
      color: #eee;
      font-family: sans-serif;
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 16px;
      margin-top: 24px;
    }}
    canvas {{
      border: 2px solid #444;
      image-rendering: pixelated;
    }}
    .info {{
      font-size: 14px;
      color: #aaa;
    }}
  </style>
</head>
<body>
  <h1>OLED 128x64 Preview</h1>
  <canvas id="screen" width="{WIDTH * SCALE}" height="{HEIGHT * SCALE}"></canvas>
  <div class="info">Масштаб: 1 пиксель = {SCALE}×{SCALE}</div>
  <script>
    const scale = {SCALE};
    const width = {WIDTH};
    const height = {HEIGHT};
    const canvas = document.getElementById('screen');
    const ctx = canvas.getContext('2d');

    function drawFrame(data) {{
      ctx.fillStyle = '#000';
      ctx.fillRect(0, 0, canvas.width, canvas.height);
      ctx.fillStyle = '#0f0';
      const pixels = data.pixels;
      for (let y = 0; y < height; y++) {{
        for (let x = 0; x < width; x++) {{
          if (pixels[y * width + x]) {{
            ctx.fillRect(x * scale, y * scale, scale, scale);
          }}
        }}
      }}
    }}

    async function refresh() {{
      const response = await fetch('/frame');
      const payload = await response.json();
      drawFrame(payload);
    }}

    refresh();
  </script>
</body>
</html>
"""


class ScreenRequestHandler(SimpleHTTPRequestHandler):
    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        if parsed.path == "/":
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.end_headers()
            self.wfile.write(HTML_PAGE.encode("utf-8"))
            return

        if parsed.path == "/frame":
            fb = render_screen()
            payload = {
                "width": fb.width,
                "height": fb.height,
                "pixels": fb.to_list(),
            }
            data = json.dumps(payload)
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(data.encode("utf-8"))
            return

        super().do_GET()


def run_server(port: int = 8000) -> None:
    address = ("", port)
    with socketserver.TCPServer(address, ScreenRequestHandler) as httpd:
        print(f"Serving on http://localhost:{port}")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nServer stopped.")


if __name__ == "__main__":
    run_server()


