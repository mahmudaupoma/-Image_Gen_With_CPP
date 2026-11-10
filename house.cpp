// House scene renderer — pure C++ (<cstdio>, <cmath>, <cstdlib> only).
// Compile : g++ -O2 -o house house.cpp
// Run     : house [columns]   (Windows cmd/PowerShell)   or   ./house [columns]   (Linux/Mac)
// Draws the scene as a colored block picture directly in the terminal,
// and also saves a full-resolution house.bmp you can open in any image viewer.

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

const int WIDTH  = 1200;
const int HEIGHT = 1150;
const double PI  = 3.14159265358979323846;

struct Color { unsigned char r, g, b; };
struct Point { double x, y; };

static unsigned char canvas[HEIGHT][WIDTH][3];

// ---- palette -------------------------------------------------------------
const Color SKY         = {100, 181, 246};
const Color GRASS       = {34, 170, 54};
const Color ROAD        = {58, 58, 64};
const Color ROAD_LINE   = {255, 255, 255};
const Color SUN_COLOR   = {255, 221, 0};
const Color CLOUD_COLOR = {255, 255, 255};
const Color WALL        = {242, 180, 60};
const Color ROOF        = {217, 30, 30};
const Color WINDOW_GLASS= {53, 101, 207};
const Color DOOR        = {110, 70, 40};
const Color DOORKNOB    = {255, 221, 0};
const Color TRUNK       = {121, 76, 40};
const Color LEAVES      = {24, 128, 46};
const Color CHIMNEY     = {150, 86, 52};
const Color SMOKE       = {235, 235, 235};
const Color FENCE       = {255, 255, 255};
const Color BUSH        = {34, 150, 46};
const Color BERRY       = {200, 30, 30};
const Color STEP        = {170, 170, 175};
const Color BLACK       = {10, 10, 10};

// ---------------------------------------------------------------------------
// Pixel-level primitives
// ---------------------------------------------------------------------------
void setPixel(int x, int y, Color c) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
    canvas[y][x][0] = c.r;
    canvas[y][x][1] = c.g;
    canvas[y][x][2] = c.b;
}

void fillBackground(Color c) {
    for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++)
            setPixel(x, y, c);
}

void drawLine(double x0, double y0, double x1, double y1, Color c) {
    int ix0 = (int)lround(x0), iy0 = (int)lround(y0);
    int ix1 = (int)lround(x1), iy1 = (int)lround(y1);
    int dx = abs(ix1 - ix0), dy = -abs(iy1 - iy0);
    int sx = (ix0 < ix1) ? 1 : -1;
    int sy = (iy0 < iy1) ? 1 : -1;
    int err = dx + dy;
    while (true) {
        setPixel(ix0, iy0, c);
        if (ix0 == ix1 && iy0 == iy1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; ix0 += sx; }
        if (e2 <= dx) { err += dx; iy0 += sy; }
    }
}

Point rotatePoint(Point p, Point center, double angleDeg) {
    double rad = angleDeg * PI / 180.0;
    double dx = p.x - center.x, dy = p.y - center.y;
    Point r;
    r.x = center.x + dx * cos(rad) - dy * sin(rad);
    r.y = center.y + dx * sin(rad) + dy * cos(rad);
    return r;
}

void fillPolygon(const Point pts[], int n, Color fillColor) {
    double minY = pts[0].y, maxY = pts[0].y;
    for (int i = 1; i < n; i++) { minY = min(minY, pts[i].y); maxY = max(maxY, pts[i].y); }
    int y0 = (int)floor(minY), y1 = (int)ceil(maxY);
    for (int y = y0; y <= y1; y++) {
        double xs[32]; int count = 0;
        for (int i = 0; i < n; i++) {
            Point a = pts[i], b = pts[(i + 1) % n];
            if ((a.y <= y && b.y > y) || (b.y <= y && a.y > y)) {
                double t = (y - a.y) / (b.y - a.y);
                xs[count++] = a.x + t * (b.x - a.x);
            }
        }
        sort(xs, xs + count);
        for (int i = 0; i + 1 < count; i += 2) {
            int xStart = (int)lround(xs[i]);
            int xEnd   = (int)lround(xs[i + 1]);
            for (int x = xStart; x <= xEnd; x++) setPixel(x, y, fillColor);
        }
    }
}

void outlinePolygon(const Point pts[], int n, Color lineColor) {
    for (int i = 0; i < n; i++)
        drawLine(pts[i].x, pts[i].y, pts[(i + 1) % n].x, pts[(i + 1) % n].y, lineColor);
}

// ---------------------------------------------------------------------------
// Shape primitives — every one takes (x, y, width, height, rotation, fillColor, lineColor)
// (x, y) is always the CENTER of the shape's bounding box.
// ---------------------------------------------------------------------------
void drawRectangle(double x, double y, double width, double height, double rotation,
                    Color fillColor, Color lineColor) {
    Point center = {x, y};
    Point corners[4] = {
        {x - width / 2, y - height / 2},
        {x + width / 2, y - height / 2},
        {x + width / 2, y + height / 2},
        {x - width / 2, y + height / 2}
    };
    for (int i = 0; i < 4; i++) corners[i] = rotatePoint(corners[i], center, rotation);
    fillPolygon(corners, 4, fillColor);
    outlinePolygon(corners, 4, lineColor);
}

// Isosceles triangle, apex pointing up before rotation.
void drawTriangle(double x, double y, double width, double height, double rotation,
                   Color fillColor, Color lineColor) {
    Point center = {x, y};
    Point verts[3] = {
        {x,             y - height / 2},
        {x - width / 2, y + height / 2},
        {x + width / 2, y + height / 2}
    };
    for (int i = 0; i < 3; i++) verts[i] = rotatePoint(verts[i], center, rotation);
    fillPolygon(verts, 3, fillColor);
    outlinePolygon(verts, 3, lineColor);
}

// radius = width/2 = height/2 expected; rotation kept only for interface consistency.
void drawCircle(double x, double y, double radius, double rotation,
                 Color fillColor, Color lineColor) {
    int r = (int)lround(radius);
    for (int dy = -r; dy <= r; dy++) {
        int dx = (int)lround(sqrt((double)r * r - (double)dy * dy));
        for (int xx = -dx; xx <= dx; xx++) setPixel((int)lround(x) + xx, (int)lround(y) + dy, fillColor);
    }
    const int segs = 48;
    Point prev = {x + radius * cos(rotation * PI / 180.0), y + radius * sin(rotation * PI / 180.0)};
    for (int i = 1; i <= segs; i++) {
        double ang = rotation + i * 360.0 / segs;
        double rad = ang * PI / 180.0;
        Point cur = {x + radius * cos(rad), y + radius * sin(rad)};
        drawLine(prev.x, prev.y, cur.x, cur.y, lineColor);
        prev = cur;
    }
}

// ---------------------------------------------------------------------------
// Composite scene objects — built only from the primitives above.
// ---------------------------------------------------------------------------
void drawSun(double x, double y, double radius, double rayLength, double rotation, Color color) {
    const int rays = 16;
    for (int i = 0; i < rays; i++) {
        double rad = (rotation + i * 360.0 / rays) * PI / 180.0;
        double ix = x + radius * cos(rad),           iy = y + radius * sin(rad);
        double ox = x + (radius + rayLength) * cos(rad), oy = y + (radius + rayLength) * sin(rad);
        drawLine(ix, iy, ox, oy, color);
    }
    drawCircle(x, y, radius, rotation, color, color);
}

void drawCloud(double x, double y, double width, double height, double rotation, Color color) {
    double r = height / 2;
    drawCircle(x - width * 0.30, y + height * 0.10, r * 0.70, rotation, color, color);
    drawCircle(x - width * 0.10, y - height * 0.15, r * 0.90, rotation, color, color);
    drawCircle(x + width * 0.15, y - height * 0.10, r * 1.00, rotation, color, color);
    drawCircle(x + width * 0.35, y + height * 0.10, r * 0.65, rotation, color, color);
}

// Simple "double-arc" bird silhouette made of straight segments.
void drawBird(double x, double y, double width, double height, double rotation, Color lineColor) {
    Point center = {x, y};
    Point p0 = rotatePoint({x - width / 2, y},             center, rotation);
    Point p1 = rotatePoint({x - width / 4, y - height / 2}, center, rotation);
    Point p2 = rotatePoint({x,             y},             center, rotation);
    Point p3 = rotatePoint({x + width / 4, y - height / 2}, center, rotation);
    Point p4 = rotatePoint({x + width / 2, y},             center, rotation);
    drawLine(p0.x, p0.y, p1.x, p1.y, lineColor);
    drawLine(p1.x, p1.y, p2.x, p2.y, lineColor);
    drawLine(p2.x, p2.y, p3.x, p3.y, lineColor);
    drawLine(p3.x, p3.y, p4.x, p4.y, lineColor);
}

void drawBush(double x, double y, double width, double height, double rotation,
              Color bushColor, Color berryColor) {
    double r = height / 2;
    drawCircle(x - width * 0.30, y,               r * 0.80, rotation, bushColor, bushColor);
    drawCircle(x,                y - height * 0.15, r * 1.00, rotation, bushColor, bushColor);
    drawCircle(x + width * 0.30, y,               r * 0.80, rotation, bushColor, bushColor);
    drawCircle(x - width * 0.15, y - height * 0.30, r * 0.60, rotation, bushColor, bushColor);
    drawCircle(x + width * 0.15, y - height * 0.30, r * 0.60, rotation, bushColor, bushColor);
    for (int i = -1; i <= 1; i++)
        drawCircle(x + i * width * 0.22, y, height * 0.08, rotation, berryColor, berryColor);
}

void drawFence(double x, double y, double width, double height, double rotation, Color fenceColor) {
    const int pickets = 7;
    double gap = width / pickets;
    double picketW = gap * 0.55;
    double startX = x - width / 2 + gap / 2;

    drawRectangle(x, y + height * 0.18, width, height * 0.14, rotation, fenceColor, BLACK);
    drawRectangle(x, y - height * 0.18, width, height * 0.14, rotation, fenceColor, BLACK);
    for (int i = 0; i < pickets; i++) {
        double px = startX + gap * i;
        drawRectangle(px, y, picketW, height, rotation, fenceColor, BLACK);
        drawTriangle(px, y - height / 2 - height * 0.10, picketW, height * 0.20, rotation, fenceColor, BLACK);
    }
}

void drawRoad(double y, double height, Color roadColor, Color lineColor) {
    drawRectangle(WIDTH / 2.0, y, (double)WIDTH, height, 0, roadColor, roadColor);
    const int dashes = 8;
    double gap = WIDTH / (double)dashes;
    double dashW = gap * 0.5;
    for (int i = 0; i < dashes; i++) {
        double dx = gap / 2 + gap * i;
        drawRectangle(dx, y, dashW, height * 0.10, 0, lineColor, lineColor);
    }
}

void drawFlowerBox(double x, double y, double width, double height) {
    drawRectangle(x, y, width, height, 0, DOOR, BLACK);
    for (int i = -2; i <= 2; i++) {
        Color petal = (i % 2 == 0) ? Color{230, 60, 110} : Color{255, 180, 60};
        drawCircle(x + i * width * 0.18, y - height * 0.9, height * 0.5, 0, petal, petal);
    }
}

void drawWindow(double x, double y, double width, double height, double rotation,
                 Color frameColor, Color glassColor) {
    drawRectangle(x, y, width, height, rotation, glassColor, frameColor);
    drawLine(x - width / 2, y, x + width / 2, y, frameColor);
    drawLine(x, y - height / 2, x, y + height / 2, frameColor);
}

void drawDoor(double x, double y, double width, double height, double rotation,
              Color doorColor, Color knobColor) {
    drawRectangle(x, y, width, height, rotation, doorColor, BLACK);
    drawCircle(x + width * 0.3, y, width * 0.08, rotation, knobColor, knobColor);
}

void drawChimney(double x, double y, double width, double height, double rotation,
                  Color chimneyColor, Color smokeColor) {
    drawRectangle(x, y, width, height, rotation, chimneyColor, BLACK);
    for (int i = 0; i < 3; i++)
        drawCircle(x + i * width * 0.25, y - height / 2 - i * height * 0.45 - 12,
                   width * 0.35 - i * 2, rotation, smokeColor, smokeColor);
}

// ---------------------------------------------------------------------------
// Terminal rendering — draws the canvas directly in cmd/terminal using
// truecolor ANSI background codes, two spaces per sampled cell (characters
// are taller than they are wide, so two narrow cells read as one square pixel).
// ---------------------------------------------------------------------------
void enableAnsiOnWindows() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hOut, &mode);
    SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

void renderToTerminal(int cols) {
    int rows = (int)((double)cols * HEIGHT / WIDTH / 2.0);
    if (rows < 1) rows = 1;
    double cellW = (double)WIDTH / cols;
    double cellH = (double)HEIGHT / rows;

    for (int ry = 0; ry < rows; ry++) {
        for (int rx = 0; rx < cols; rx++) {
            int x0 = (int)(rx * cellW),       x1 = (int)((rx + 1) * cellW);
            int y0 = (int)(ry * cellH),       y1 = (int)((ry + 1) * cellH);
            if (x1 <= x0) x1 = x0 + 1;
            if (y1 <= y0) y1 = y0 + 1;

            long sumR = 0, sumG = 0, sumB = 0, count = 0;
            for (int yy = y0; yy < y1 && yy < HEIGHT; yy++) {
                for (int xx = x0; xx < x1 && xx < WIDTH; xx++) {
                    sumR += canvas[yy][xx][0];
                    sumG += canvas[yy][xx][1];
                    sumB += canvas[yy][xx][2];
                    count++;
                }
            }
            int avgR = (int)(sumR / count), avgG = (int)(sumG / count), avgB = (int)(sumB / count);
            printf("\x1b[48;2;%d;%d;%dm  ", avgR, avgG, avgB);
        }
        printf("\x1b[0m\n");
    }
}

// ---------------------------------------------------------------------------
// BMP writer (24-bit, uncompressed) — no external libraries needed.
// ---------------------------------------------------------------------------
void putLE32(unsigned char* buf, unsigned int v) {
    buf[0] = v & 0xFF; buf[1] = (v >> 8) & 0xFF; buf[2] = (v >> 16) & 0xFF; buf[3] = (v >> 24) & 0xFF;
}
void putLE16(unsigned char* buf, unsigned short v) {
    buf[0] = v & 0xFF; buf[1] = (v >> 8) & 0xFF;
}

void saveBMP(const char* filename) {
    int rowSize  = (WIDTH * 3 + 3) & ~3;
    int dataSize = rowSize * HEIGHT;
    int fileSize = 54 + dataSize;

    unsigned char header[54];
    memset(header, 0, sizeof(header));
    header[0] = 'B'; header[1] = 'M';
    putLE32(&header[2],  (unsigned int)fileSize);
    putLE32(&header[10], 54);
    putLE32(&header[14], 40);
    putLE32(&header[18], (unsigned int)WIDTH);
    putLE32(&header[22], (unsigned int)HEIGHT);
    putLE16(&header[26], 1);
    putLE16(&header[28], 24);
    putLE32(&header[34], (unsigned int)dataSize);

    FILE* f = fopen(filename, "wb");
    if (!f) { fprintf(stderr, "Could not open %s for writing\n", filename); exit(1); }
    fwrite(header, 1, 54, f);

    unsigned char* row = (unsigned char*)malloc(rowSize);
    for (int y = HEIGHT - 1; y >= 0; y--) {
        memset(row, 0, rowSize);
        for (int x = 0; x < WIDTH; x++) {
            row[x * 3 + 0] = canvas[y][x][2]; // B
            row[x * 3 + 1] = canvas[y][x][1]; // G
            row[x * 3 + 2] = canvas[y][x][0]; // R
        }
        fwrite(row, 1, rowSize, f);
    }
    free(row);
    fclose(f);
}

// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    int cols = 50;                       // terminal width in characters; pass a number to change it
    if (argc > 1) cols = atoi(argv[1]);
    if (cols < 10) cols = 10;

    printf("Draw the house picture? (y/n): ");
    char answer = 'n';
    if (scanf(" %c", &answer) != 1) answer = 'n';
    if (answer != 'y' && answer != 'Y') {
        printf("Okay, skipping the drawing.\n");
        return 0;
    }

    fillBackground(SKY);

    drawRectangle(WIDTH / 2.0, HEIGHT * 0.765, WIDTH, HEIGHT * 0.17, 0, GRASS, GRASS);
    drawRoad(HEIGHT * 0.88, HEIGHT * 0.10, ROAD, ROAD_LINE);
    drawRectangle(WIDTH / 2.0, HEIGHT * 0.965, WIDTH, HEIGHT * 0.07, 0, GRASS, GRASS);

    drawSun(WIDTH * 0.86, HEIGHT * 0.16, 65, 45, 0, SUN_COLOR);
    drawCloud(WIDTH * 0.16, HEIGHT * 0.19, 160, 70, 0, CLOUD_COLOR);
    drawCloud(WIDTH * 0.46, HEIGHT * 0.12, 130, 60, 0, CLOUD_COLOR);

    drawBird(WIDTH * 0.27, HEIGHT * 0.30, 50, 22, 0, BLACK);
    drawBird(WIDTH * 0.22, HEIGHT * 0.34, 40, 18, 0, BLACK);
    drawBird(WIDTH * 0.31, HEIGHT * 0.37, 45, 20, 0, BLACK);

    // ---- Trees: trunk + 3 stacked foliage tiers; every coordinate computed here ----
    double treeCenters[2] = {WIDTH * 0.12, WIDTH * 0.88};
    double treeGroundY = HEIGHT * 0.72, treeW = 140, treeH = 300;
    for (int t = 0; t < 2; t++) {
        double treeX = treeCenters[t];
        double trunkW = treeW * 0.20, trunkH = treeH * 0.22;
        double trunkY = treeGroundY + treeH / 2 - trunkH / 2;
        drawRectangle(treeX, trunkY, trunkW, trunkH, 0, TRUNK, BLACK);

        double foliageH = treeH - trunkH;
        double foliageBaseY = treeGroundY + treeH / 2 - trunkH;
        double tier1Y = foliageBaseY - foliageH * 0.22, tier1W = treeW,        tier1H = foliageH * 0.45;
        double tier2Y = foliageBaseY - foliageH * 0.50, tier2W = treeW * 0.78, tier2H = foliageH * 0.42;
        double tier3Y = foliageBaseY - foliageH * 0.76, tier3W = treeW * 0.55, tier3H = foliageH * 0.40;
        drawTriangle(treeX, tier1Y, tier1W, tier1H, 0, LEAVES, BLACK);
        drawTriangle(treeX, tier2Y, tier2W, tier2H, 0, LEAVES, BLACK);
        drawTriangle(treeX, tier3Y, tier3W, tier3H, 0, LEAVES, BLACK);
    }

    // ---- House: wall, roof, chimney, windows, flower boxes, door, steps ----
    double houseX = WIDTH * 0.50, houseY = HEIGHT * 0.60, houseW = 420, houseH = 440;

    double wallW = houseW, wallH = houseH * 0.55;
    double wallX = houseX, wallY = houseY + houseH / 2 - wallH / 2;
    drawRectangle(wallX, wallY, wallW, wallH, 0, WALL, BLACK);

    double roofW = houseW * 1.15, roofH = houseH * 0.45;
    double roofX = houseX, roofY = houseY - houseH / 2 + roofH / 2;
    drawTriangle(roofX, roofY, roofW, roofH, 0, ROOF, BLACK);

    double chimneyW = houseW * 0.09, chimneyH = houseH * 0.18;
    double chimneyX = houseX + houseW * 0.28, chimneyY = roofY - roofH * 0.12;
    drawChimney(chimneyX, chimneyY, chimneyW, chimneyH, 0, CHIMNEY, SMOKE);

    double windowW = houseW * 0.22, windowH = wallH * 0.40, windowY = wallY - wallH * 0.10;
    double leftWindowX = houseX - houseW * 0.25, rightWindowX = houseX + houseW * 0.25;
    drawWindow(leftWindowX,  windowY, windowW, windowH, 0, BLACK, WINDOW_GLASS);
    drawWindow(rightWindowX, windowY, windowW, windowH, 0, BLACK, WINDOW_GLASS);

    double boxW = houseW * 0.24, boxH = wallH * 0.10, boxY = wallY + wallH * 0.12;
    drawFlowerBox(leftWindowX,  boxY, boxW, boxH);
    drawFlowerBox(rightWindowX, boxY, boxW, boxH);

    double doorW = houseW * 0.16, doorH = wallH * 0.55, doorY = wallY + wallH * 0.25;
    drawDoor(houseX, doorY, doorW, doorH, 0, DOOR, DOORKNOB);

    double step1Y = wallY + wallH / 2 + houseH * 0.02, step1W = houseW * 0.30;
    double step2Y = wallY + wallH / 2 + houseH * 0.06, step2W = houseW * 0.22;
    drawRectangle(houseX, step1Y, step1W, houseH * 0.04, 0, STEP, BLACK);
    drawRectangle(houseX, step2Y, step2W, houseH * 0.04, 0, STEP, BLACK);

    drawBush(WIDTH * 0.27, HEIGHT * 0.80, 110, 70, 0, BUSH, BERRY);
    drawBush(WIDTH * 0.73, HEIGHT * 0.80, 110, 70, 0, BUSH, BERRY);

    drawFence(WIDTH * 0.23, HEIGHT * 0.80, 170, 70, 0, FENCE);
    drawFence(WIDTH * 0.77, HEIGHT * 0.80, 170, 70, 0, FENCE);

    enableAnsiOnWindows();
    renderToTerminal(cols);

    saveBMP("house.bmp");
    printf("Also saved full-resolution image to house.bmp (%dx%d)\n", WIDTH, HEIGHT);
    return 0;
}
