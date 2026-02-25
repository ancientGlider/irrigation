import json
import socketserver
from http.server import SimpleHTTPRequestHandler
from urllib.parse import urlparse


WIDTH = 250
HEIGHT = 250
SCALE = 4
ICON_SIZE = 13
ICONS_PER_COLUMN = 15
COLUMNS = 8
MAX_ICONS = ICONS_PER_COLUMN * COLUMNS  # 120
COLUMN_X = [0, 20, 60, 80, 120, 140, 180, 200]
ICONS_SOURCE = "tools/display/plant_icons.json"


class FrameBuffer:
    def __init__(self, width: int, height: int) -> None:
        self.width = width
        self.height = height
        self.buffer = [[0 for _ in range(width)] for _ in range(height)]

    def set_pixel(self, x: int, y: int, value: int = 1) -> None:
        if 0 <= x < self.width and 0 <= y < self.height:
            self.buffer[y][x] = 1 if value else 0

    def to_list(self) -> list[int]:
        flat: list[int] = []
        for row in self.buffer:
            flat.extend(row)
        return flat


def parse_icon(rows: list[str]) -> list[str]:
    pattern: list[str] = []
    for row_idx in range(ICON_SIZE):
        if row_idx < len(rows):
            raw_row = rows[row_idx]
        else:
            raw_row = ""

        sanitized = []
        for char in raw_row[:ICON_SIZE]:
            sanitized.append("#" if char in ("1", "#", "X") else ".")
        if len(sanitized) < ICON_SIZE:
            sanitized.extend("." for _ in range(ICON_SIZE - len(sanitized)))
        pattern.append("".join(sanitized))
    return pattern


def load_icon_patterns(path: str) -> list[list[str]]:
    try:
        with open(path, "r", encoding="utf-8") as fh:
            data = json.load(fh)
    except FileNotFoundError:
        return []

    patterns: list[list[str]] = []
    for _, rows in data.items():
        patterns.append(parse_icon(rows))
        if len(patterns) >= MAX_ICONS:
            break
    return patterns


ICON_PATTERNS = load_icon_patterns(ICONS_SOURCE)
TOTAL_ICONS = len(ICON_PATTERNS)


def draw_icon(fb: FrameBuffer, x: int, y: int, pattern: list[str]) -> None:
    for row, line in enumerate(pattern):
        for col, char in enumerate(line):
            if char == "#":
                fb.set_pixel(x + col, y + row)


def render_gallery() -> FrameBuffer:
    fb = FrameBuffer(WIDTH, HEIGHT)
    for index, pattern in enumerate(ICON_PATTERNS):
        column = index // ICONS_PER_COLUMN
        if column >= len(COLUMN_X):
            break
        row = index % ICONS_PER_COLUMN
        x = COLUMN_X[column]
        y = row * 14
        draw_icon(fb, x, y, pattern)
    return fb


HTML_PAGE = f"""<!DOCTYPE html>
<html lang="ru">
<head>
  <meta charset="UTF-8">
  <title>Icon Gallery 13×13</title>
  <style>
    body {{
      background: #111;
      color: #eee;
      font-family: sans-serif;
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 12px;
      margin-top: 20px;
    }}
    canvas {{
      border: 2px solid #444;
      image-rendering: pixelated;
    }}
  </style>
</head>
<body>
  <h1>Icon Gallery (13×13 px)</h1>
  <canvas id="screen" width="{WIDTH * SCALE}" height="{HEIGHT * SCALE}"></canvas>
  <div>Масштаб: 4×. Загружено {TOTAL_ICONS} иконок из plant_icons.json.</div>
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


class IconRequestHandler(SimpleHTTPRequestHandler):
    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        if parsed.path == "/":
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.end_headers()
            self.wfile.write(HTML_PAGE.encode("utf-8"))
            return

        if parsed.path == "/frame":
            fb = render_gallery()
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


def run_server(port: int = 8001) -> None:
    address = ("", port)
    with socketserver.TCPServer(address, IconRequestHandler) as httpd:
        print(f"Serving icon gallery on http://localhost:{port}")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nServer stopped.")


if __name__ == "__main__":
    run_server()
import json
import math
import socketserver
from http.server import SimpleHTTPRequestHandler
from urllib.parse import urlparse


WIDTH = 250
HEIGHT = 250
SCALE = 4
ICON_SIZE = 13
ICONS_PER_COLUMN = 15
COLUMNS = 8
TOTAL_ICONS = ICONS_PER_COLUMN * COLUMNS


class FrameBuffer:
    def __init__(self, width: int, height: int) -> None:
        self.width = width
        self.height = height
        self.buffer = [[0 for _ in range(width)] for _ in range(height)]

    def clear(self) -> None:
        for y in range(self.height):
            for x in range(self.width):
                self.buffer[y][x] = 0

    def set_pixel(self, x: int, y: int, value: int = 1) -> None:
        if 0 <= x < self.width and 0 <= y < self.height:
            self.buffer[y][x] = 1 if value else 0

    def to_list(self) -> list[int]:
        flat: list[int] = []
        for row in self.buffer:
            flat.extend(row)
        return flat


U8G2_FONT_5X8 = {
    " ": [
        "00000",
        "00000",
        "00000",
        "00000",
        "00000",
        "00000",
        "00000",
        "00000",
    ],
    "0": [
        "01110",
        "10001",
        "10011",
        "10101",
        "11001",
        "10001",
        "01110",
        "00000",
    ],
    "1": [
        "00100",
        "01100",
        "00100",
        "00100",
        "00100",
        "00100",
        "01110",
        "00000",
    ],
    "2": [
        "01110",
        "10001",
        "00001",
        "00110",
        "01000",
        "10000",
        "11111",
        "00000",
    ],
    "3": [
        "01110",
        "10001",
        "00001",
        "00110",
        "00001",
        "10001",
        "01110",
        "00000",
    ],
    "4": [
        "00010",
        "00110",
        "01010",
        "10010",
        "11111",
        "00010",
        "00010",
        "00000",
    ],
    "5": [
        "11111",
        "10000",
        "11110",
        "00001",
        "00001",
        "10001",
        "01110",
        "00000",
    ],
    "6": [
        "00110",
        "01000",
        "10000",
        "11110",
        "10001",
        "10001",
        "01110",
        "00000",
    ],
    "7": [
        "11111",
        "00001",
        "00010",
        "00100",
        "01000",
        "01000",
        "01000",
        "00000",
    ],
    "8": [
        "01110",
        "10001",
        "10001",
        "01110",
        "10001",
        "10001",
        "01110",
        "00000",
    ],
    "9": [
        "01110",
        "10001",
        "10001",
        "01111",
        "00001",
        "00010",
        "01100",
        "00000",
    ],
}


def draw_text(fb: FrameBuffer, x: int, y: int, text: str) -> None:
    glyph_height = len(next(iter(U8G2_FONT_5X8.values())))
    glyph_width = len(next(iter(U8G2_FONT_5X8.values()))[0])
    cursor = x
    for ch in text:
        glyph = U8G2_FONT_5X8.get(ch, U8G2_FONT_5X8[" "])
        for row in range(glyph_height):
            for col in range(glyph_width):
                if glyph[row][col] == "1":
                    fb.set_pixel(cursor + col, y + row)
        cursor += glyph_width + 1


def blank_icon() -> list[list[int]]:
    return [[0 for _ in range(ICON_SIZE)] for _ in range(ICON_SIZE)]


def to_pattern(matrix: list[list[int]]) -> list[str]:
    return ["".join("#" if cell else "." for cell in row) for row in matrix]


def make_gear(radius: float, teeth: int, tooth_len: float, hole_radius: float, spoke: bool, notch: bool) -> list[str]:
    grid = blank_icon()
    center = ICON_SIZE // 2
    for y in range(ICON_SIZE):
        for x in range(ICON_SIZE):
            dx = x - center
            dy = y - center
            dist = math.hypot(dx, dy)
            if dist <= radius:
                grid[y][x] = 1
            elif radius < dist <= radius + tooth_len:
                angle = (math.atan2(dy, dx) + 2.0 * math.pi) % (2.0 * math.pi)
                position = angle / (2.0 * math.pi) * teeth
                if abs(position - round(position)) < 0.35:
                    grid[y][x] = 1

    if hole_radius > 0:
        for y in range(ICON_SIZE):
            for x in range(ICON_SIZE):
                dx = x - center
                dy = y - center
                if math.hypot(dx, dy) < hole_radius:
                    grid[y][x] = 0

    if spoke:
        for y in range(ICON_SIZE):
            if abs(y - center) <= 1:
                for x in range(ICON_SIZE):
                    if math.hypot(x - center, y - center) <= radius - 1:
                        grid[y][x] = 1
        for x in range(ICON_SIZE):
            if abs(x - center) <= 1:
                for y in range(ICON_SIZE):
                    if math.hypot(x - center, y - center) <= radius - 1:
                        grid[y][x] = 1

    if notch:
        radius_int = max(0, min(ICON_SIZE - 1, int(round(radius))))
        for offset in range(-1, 2):
            x_pos = center + radius_int - 1
            x_neg = center - radius_int + 1
            y_idx = center + offset
            if 0 <= y_idx < ICON_SIZE:
                if 0 <= x_pos < ICON_SIZE:
                    grid[y_idx][x_pos] = 0
                if 0 <= x_neg < ICON_SIZE:
                    grid[y_idx][x_neg] = 0

    return to_pattern(grid)


def make_drop(tip_width: float, body_half: float, bulge: float, tail: bool, spray: bool, split: bool) -> list[str]:
    grid = blank_icon()
    center = ICON_SIZE // 2
    for y in range(ICON_SIZE):
        t = y / (ICON_SIZE - 1)
        half_width = body_half * t + tip_width * (1 - t) ** bulge
        for x in range(ICON_SIZE):
            if abs(x - center) <= half_width:
                grid[y][x] = 1

    if tail:
        for x in range(center - 1, center + 2):
            for y in range(ICON_SIZE - 3, ICON_SIZE):
                grid[y][x] = 1

    if spray:
        for x in range(0, ICON_SIZE, 3):
            grid[2][x] = 1
            grid[4][(x + 1) % ICON_SIZE] = 1

    if split:
        for y in range(center + 1, ICON_SIZE):
            grid[y][center] = 0

    return to_pattern(grid)


def make_info(outer_radius: float, ring: float, dot_radius: float, stem_width: int, accent: str) -> list[str]:
    grid = blank_icon()
    center = ICON_SIZE // 2
    for y in range(ICON_SIZE):
        for x in range(ICON_SIZE):
            dx = x - center
            dy = y - center
            dist = math.hypot(dx, dy)
            if outer_radius - ring <= dist <= outer_radius:
                grid[y][x] = 1

    # Dot
    for y in range(ICON_SIZE):
        for x in range(ICON_SIZE):
            dx = x - center
            dy = y - (center - 3)
            if math.hypot(dx, dy) <= dot_radius:
                grid[y][x] = 1

    # Stem
    stem_half = stem_width / 2.0
    for y in range(center - 1, ICON_SIZE - 1):
        for x in range(ICON_SIZE):
            if abs(x - center) <= stem_half:
                grid[y][x] = 1

    if accent == "tilt":
        for y in range(center, ICON_SIZE):
            x = center + (y - center) // 2
            if 0 <= x < ICON_SIZE:
                grid[y][x] = 1
    elif accent == "spark":
        for dx in (-2, 0, 2):
            if 0 <= center + dx < ICON_SIZE:
                grid[1][center + dx] = 1
    elif accent == "minus":
        for x in range(center - 3, center + 4):
            grid[center + 2][x] = 0
        for x in range(center - 2, center + 3):
            grid[center + 4][x] = 1

    return to_pattern(grid)


def make_toggle(radius: float, border: float, angle_deg: float, invert: bool, core: bool, spark: bool) -> list[str]:
    grid = blank_icon()
    center = ICON_SIZE // 2
    angle_rad = math.radians(angle_deg)
    nx = math.cos(angle_rad)
    ny = math.sin(angle_rad)

    for y in range(ICON_SIZE):
        for x in range(ICON_SIZE):
            dx = x - center
            dy = y - center
            dist = math.hypot(dx, dy)
            if dist <= radius:
                if dist >= radius - border:
                    grid[y][x] = 1
                else:
                    side = dx * nx + dy * ny
                    if (side >= 0 and not invert) or (side < 0 and invert):
                        grid[y][x] = 1

    if core:
        for y in range(ICON_SIZE):
            for x in range(ICON_SIZE):
                dx = x - center
                dy = y - center
                if math.hypot(dx, dy) < radius / 2.5:
                    grid[y][x] = 1 if not invert else 0

    if spark:
        directions = [(2, -4), (-2, -4), (4, -1), (-4, -1), (2, 4), (-2, 4)]
        for dx, dy in directions:
            px = center + dx
            py = center + dy
            if 0 <= px < ICON_SIZE and 0 <= py < ICON_SIZE:
                grid[py][px] = 1

    return to_pattern(grid)


def generate_icon(index: int) -> list[str]:
    column = index // ICONS_PER_COLUMN
    local = index % ICONS_PER_COLUMN

    if column <= 1:
        teeth = 6 + (index % 9)
        radius = 4.0 + (0.12 * local) + (0.1 * column)
        tooth_len = 0.8 + (local % 5) * 0.2
        hole = 0.0 if (local % 3 == 0 and column == 0) else 0.5 + 0.3 * ((local + column) % 4)
        spoke = (local % 4) == 1
        notch = (local + column) % 5 == 0
        return make_gear(radius, teeth, tooth_len, hole, spoke, notch)

    if 2 <= column <= 3:
        tip = 0.5 + (local % 4) * 0.2
        body = 4.8 + (0.25 * (local % 6))
        bulge = 1.2 + (local % 5) * 0.25
        tail = (local % 2) == 0
        spray = (local % 3) == 1
        split = (local + column) % 4 == 0
        return make_drop(tip, body, bulge, tail, spray, split)

    if 4 <= column <= 5:
        outer = 4.8 + (0.2 * (local % 5))
        ring = 1.0 + (0.1 * (local % 3))
        dot = 1.0 + (0.3 * (local % 4))
        stem = 1 + (local % 3)
        accents = ["tilt", "spark", "minus", "none"]
        accent = accents[(local + column) % len(accents)]
        if accent == "none":
            accent = ""
        return make_info(outer, ring, dot, stem, accent)

    radius = 5.4 + (0.2 * (local % 5))
    border = 1.0 + (0.15 * (local % 4))
    angle = 30 + (local % 6) * 10 * (1 if column == 6 else -1)
    invert = (local % 2) == 1
    core = (local % 3) == 0
    spark = (local + column) % 4 == 2
    return make_toggle(radius, border, angle, invert, core, spark)


ICON_PATTERNS = [generate_icon(i) for i in range(TOTAL_ICONS)]


def draw_icon(fb: FrameBuffer, x: int, y: int, pattern: list[str]) -> None:
    for row, line in enumerate(pattern):
        for col, char in enumerate(line):
            if char == "#":
                fb.set_pixel(x + col, y + row)


COLUMN_X = [0, 20, 60, 80, 120, 140, 180, 200]


def render_gallery() -> FrameBuffer:
    fb = FrameBuffer(WIDTH, HEIGHT)
    for col, x in enumerate(COLUMN_X):
        for row in range(ICONS_PER_COLUMN):
            index = col * ICONS_PER_COLUMN + row
            y = row * 14
            draw_icon(fb, x, y, ICON_PATTERNS[index])

    return fb


def save_selected_icons(indices: list[int], path: str) -> None:
    lines: list[str] = []
    for idx in indices:
        zero = idx - 1
        if 0 <= zero < TOTAL_ICONS:
            lines.append(f"Icon {idx}")
            lines.extend(ICON_PATTERNS[zero])
            lines.append("")
    with open(path, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines))


save_selected_icons([5, 93], "tools/display/_tmp_icons_5_93.txt")


HTML_PAGE = f"""<!DOCTYPE html>
<html lang="ru">
<head>
  <meta charset="UTF-8">
  <title>Icon Gallery 13×13</title>
  <style>
    body {{
      background: #111;
      color: #eee;
      font-family: sans-serif;
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 12px;
      margin-top: 20px;
    }}
    canvas {{
      border: 2px solid #444;
      image-rendering: pixelated;
    }}
  </style>
</head>
<body>
  <h1>Icon Gallery (13×13 px)</h1>
  <canvas id="screen" width="{WIDTH * SCALE}" height="{HEIGHT * SCALE}"></canvas>
  <div>Масштаб: 4×. Колонки 0/20 — кнопка 1, 60/80 — кнопка 2, 120/140 — кнопка 3, 180/200 — кнопка 4.</div>
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


class IconRequestHandler(SimpleHTTPRequestHandler):
    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        if parsed.path == "/":
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.end_headers()
            self.wfile.write(HTML_PAGE.encode("utf-8"))
            return

        if parsed.path == "/frame":
            fb = render_gallery()
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


def run_server(port: int = 8001) -> None:
    address = ("", port)
    with socketserver.TCPServer(address, IconRequestHandler) as httpd:
        print(f"Serving icon gallery on http://localhost:{port}")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nServer stopped.")


if __name__ == "__main__":
    run_server()


