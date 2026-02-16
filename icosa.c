/* icosa.c — Bouncing glenz vector over a checkerboard floor
 * Inspired by the 2nd Reality demo (Future Crew, 1993).
 * The shape is a tetrakis hexahedron rendered as a translucent wireframe
 * with physics-based bouncing and squash-and-stretch deformation.
 */

#include <math.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

/* Tetrakis hexahedron: cube + pyramid on each face */
#define NVERTS 14
#define NEDGES 36
#define PYR 1.5f /* pyramid tip distance from center */

static const float base_verts[NVERTS][3] = {
    /* Cube corners (0-7) */
    { 1,  1,  1}, { 1,  1, -1}, { 1, -1,  1}, { 1, -1, -1},
    {-1,  1,  1}, {-1,  1, -1}, {-1, -1,  1}, {-1, -1, -1},
    /* Pyramid tips: +x, -x, +y, -y, +z, -z (8-13) */
    {PYR, 0, 0}, {-PYR, 0, 0},
    {0, PYR, 0}, {0, -PYR, 0},
    {0, 0, PYR}, {0, 0, -PYR}
};

static const int edges[NEDGES][2] = {
    /* Cube edges (12) */
    {0,1},{0,2},{0,4},{1,3},{1,5},{2,3},{2,6},{3,7},{4,5},{4,6},{5,7},{6,7},
    /* +x face → tip 8 */  {8,0},{8,1},{8,2},{8,3},
    /* -x face → tip 9 */  {9,4},{9,5},{9,6},{9,7},
    /* +y face → tip 10 */ {10,0},{10,1},{10,4},{10,5},
    /* -y face → tip 11 */ {11,2},{11,3},{11,6},{11,7},
    /* +z face → tip 12 */ {12,0},{12,2},{12,4},{12,6},
    /* -z face → tip 13 */ {13,1},{13,3},{13,5},{13,7}
};

static unsigned char *fb;        /* braille dot framebuffer */
static unsigned char *floor_map; /* per-cell: 0=sky, 1=dark, 2=light */
static int cw, ch;
static int pw, ph;
static int horizon;
static char *rbuf;
static struct termios orig_tios;

static void cleanup_terminal(void) {
    static const char seq[] = "\033[0m\033[?25h\033[?1049l";
    write(STDOUT_FILENO, seq, sizeof(seq) - 1);
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_tios);
}

static void on_signal(int sig) {
    cleanup_terminal();
    signal(sig, SIG_DFL);
    raise(sig);
}

static void fb_clear(void) { memset(fb, 0, (size_t)cw * ch); }

static void fb_set(int x, int y) {
    if (x < 0 || x >= pw || y < 0 || y >= ph) return;
    static const unsigned char bits[2][4] = {
        {0x01, 0x02, 0x04, 0x40},
        {0x08, 0x10, 0x20, 0x80}
    };
    fb[(y / 4) * cw + (x / 2)] |= bits[x & 1][y & 3];
}

static void draw_line(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        fb_set(x0, y0);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void compute_floor(void) {
    floor_map = calloc((size_t)cw * ch, 1);
    horizon = ch * 55 / 100;

    float floor_h = (float)(ch - horizon);
    for (int row = horizon + 1; row < ch; row++) {
        float t = (float)(row - horizon) / floor_h; /* 0→1 from horizon→bottom */
        float z = 1.0f / t;                         /* perspective depth */
        for (int col = 0; col < cw; col++) {
            float x = ((float)col / (float)cw - 0.5f) * z * 8.0f;
            int ix = (int)floorf(x);
            int iz = (int)floorf(z * 4.0f);
            floor_map[row * cw + col] = (unsigned char)(((ix + iz) & 1) ? 1 : 2);
        }
    }
}

static char *put_str(char *p, const char *s) {
    while (*s) *p++ = *s++;
    return p;
}

static void render(void) {
    char *p = rbuf;
    p = put_str(p, "\033[H");

    int prev = -1;
    for (int y = 0; y < ch; y++) {
        for (int x = 0; x < cw; x++) {
            int idx = y * cw + x;
            int mode = fb[idx] ? 3 : floor_map[idx];

            if (mode != prev) {
                switch (mode) {
                case 0: p = put_str(p, "\033[0m"); break;
                case 1: p = put_str(p, "\033[48;5;236m"); break;
                case 2: p = put_str(p, "\033[48;5;252m"); break;
                case 3: p = put_str(p, "\033[0;96m"); break;
                }
                prev = mode;
            }

            if (fb[idx]) {
                unsigned int cp = 0x2800 + fb[idx];
                *p++ = (char)(0xE0 | (cp >> 12));
                *p++ = (char)(0x80 | ((cp >> 6) & 0x3F));
                *p++ = (char)(0x80 | (cp & 0x3F));
            } else {
                *p++ = ' ';
            }
        }
        p = put_str(p, "\033[0m");
        prev = -1;
        if (y < ch - 1) *p++ = '\n';
    }

    fwrite(rbuf, 1, (size_t)(p - rbuf), stdout);
    fflush(stdout);
}

static void usage(void) {
    fputs(
        "icosa — bouncing glenz vector over a checkerboard floor\n"
        "\n"
        "Inspired by the 2nd Reality demo (Future Crew, 1993).\n"
        "Renders a spinning tetrakis hexahedron with braille-dot wireframe,\n"
        "physics-based bouncing, and squash-and-stretch deformation.\n"
        "\n"
        "Usage: icosa [OPTIONS]\n"
        "\n"
        "Options:\n"
        "  -h, --help    Show this help message\n"
        "\n"
        "Controls:\n"
        "  Any key        Quit\n"
        "\n"
        "Designed for use with demomotd as a terminal greeting effect.\n"
        "Can also be run standalone or with timeout(1):\n"
        "  timeout 5 icosa\n",
        stdout);
}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage();
            return 0;
        }
        fprintf(stderr, "icosa: unknown option '%s'\n", argv[i]);
        return 1;
    }

    /* Signal handlers for SIGTERM (from timeout) and SIGINT */
    struct sigaction sa = {0};
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = on_signal;
    sa.sa_flags = SA_RESETHAND;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) < 0 || ws.ws_col < 20 || ws.ws_row < 10)
        return 1;

    cw = ws.ws_col;
    ch = ws.ws_row;
    pw = cw * 2;
    ph = ch * 4;

    fb = calloc((size_t)cw * ch, 1);
    rbuf = malloc((size_t)ch * ((size_t)cw * 20 + 16) + 64);
    if (!fb || !rbuf) return 1;

    compute_floor();

    /* Raw mode so any keypress (including ctrl-c) is readable as input */
    tcgetattr(STDIN_FILENO, &orig_tios);
    struct termios raw = orig_tios;
    raw.c_lflag &= ~(ICANON | ECHO | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    fputs("\033[?1049h\033[?25l\033[2J", stdout);
    fflush(stdout);

    float scale = fminf((float)pw, (float)ph) * 0.45f;
    float center_x = (float)pw / 2.0f;
    float floor_py = (float)(horizon * 4); /* horizon in braille pixels */

    /* Physics in braille-pixel units — scales with terminal size */
    float max_bounce = floor_py * 0.55f;  /* max bounce height in pixels */
    float fall_frames = 22.0f;            /* frames to fall from max (~0.7s) */
    float grav = 2.0f * max_bounce / (fall_frames * fall_frames);
    float restart_vel = sqrtf(2.0f * max_bounce * grav);
    float pos = max_bounce;  /* start at top of bounce */
    float vel = 0;
    const float damp = 0.82f;
    float squash = 0;
    const float squash_decay = 0.70f;

    float ax = 0, ay = 0, az = 0;
    struct pollfd pfd = { .fd = STDIN_FILENO, .events = POLLIN };

    for (;;) {
        /* Any keypress = exit */
        if (poll(&pfd, 1, 0) > 0)
            break;

        fb_clear();

        /* Gravity and bounce (all in braille-pixel units) */
        vel -= grav;
        pos += vel;
        if (pos <= 0) {
            pos = 0;
            squash = fminf(fabsf(vel) / restart_vel * 0.5f, 0.5f);
            vel = fabsf(vel) * damp;
            if (vel < grav * 8.0f) vel = restart_vel;
        }
        squash *= squash_decay;

        float yscale = 1.0f - squash;
        float xzscale = 1.0f + squash * 0.5f;

        /* Object center: centered horizontally, bounces vertically */
        float obj_cx = center_x;
        float obj_cy = floor_py - pos - scale * 0.2f;

        float s1 = sinf(ax), c1 = cosf(ax);
        float s2 = sinf(ay), c2 = cosf(ay);
        float s3 = sinf(az), c3 = cosf(az);

        float proj[NVERTS][2];
        for (int i = 0; i < NVERTS; i++) {
            float x = base_verts[i][0];
            float y = base_verts[i][1];
            float z = base_verts[i][2];

            /* Rotate */
            float y1 = y * c1 - z * s1;
            float z1 = y * s1 + z * c1;
            float x2 = x * c2 + z1 * s2;
            float z2 = -x * s2 + z1 * c2;
            float x3 = x2 * c3 - y1 * s3;
            float y3 = x2 * s3 + y1 * c3;

            /* Squash/stretch in screen space (vertical squash on impact) */
            x3 *= xzscale;
            y3 *= yscale;

            /* Perspective */
            float d = 5.0f + z2 * 0.3f;
            proj[i][0] = obj_cx + (x3 / d) * scale;
            proj[i][1] = obj_cy + (y3 / d) * scale;
        }

        for (int i = 0; i < NEDGES; i++)
            draw_line((int)proj[edges[i][0]][0], (int)proj[edges[i][0]][1],
                      (int)proj[edges[i][1]][0], (int)proj[edges[i][1]][1]);

        render();

        ax += 0.05f;
        ay += 0.07f;
        az += 0.03f;

        /* Frame delay ~30fps — use poll as the timer */
        poll(&pfd, 1, 33);
        if (pfd.revents & POLLIN)
            break;
    }

    cleanup_terminal();
    free(fb);
    free(rbuf);
    free(floor_map);
    return 0;
}
