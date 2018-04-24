#include <EEPROM.h>
#include <SPI.h>
#include <STM32_GD2.h>

char directory[60][8];
int num_jpgs = 0;

class scroller {
public:
  signed short dragprev;
  int vel;      // velocity
  long base;    // screen x coordinate, in 1/16ths pixel
  long limit;
  void init(uint32_t lim) {
    dragprev = -32768;
    vel = 0;      // velocity
    base = 0;     // screen x coordinate, in 1/16ths pixel
    limit = lim;
  }
  void run(bool touching, int16_t sx) {
    if (touching & (dragprev != -32768)) {
      vel = (dragprev - sx) << 4;
    } else {
      int change = max(1, abs(vel) >> 5);
      if (vel < 0)
        vel += change;
      if (vel > 0)
        vel -= change;
    }
    dragprev = touching ? sx : -32768;
    base += vel;
    base = max(0, min(base, limit));
  }
  uint16_t getval() {
    return base >> 4;
  }
};

void setup()
{
  delay(1000);Serial.begin(9600);
  strcpy(directory[0],"healsky3");
  strcpy(directory[1],"selfie");
  strcpy(directory[2],"sunrise");
  strcpy(directory[3],"tile1920");
  strcpy(directory[4],"tree");
  num_jpgs = 5;
  GD.begin(FORCE_CALIBRATE + GD_STORAGE);
}

#define PROGRESS_Y  136

static void progress(long a, long b)
{
    GD.wr32(REG_MACRO_0, VERTEX2II(40 + 399 * a / b, PROGRESS_Y, 0, 0));
}

static void draw_progress()
{
  GD.Begin(LINES);

  GD.ColorRGB(0xc0c0c0);
  GD.LineWidth(16 * 5);
  GD.Vertex2ii(40, PROGRESS_Y);
  GD.Vertex2ii(440, PROGRESS_Y);

  GD.ColorRGB(0x000000);
  GD.LineWidth(16 * 3);
  GD.Vertex2ii(40, PROGRESS_Y);
  GD.Vertex2ii(440, PROGRESS_Y);

  GD.ColorRGB(0xffffff);
  GD.LineWidth(16 * 2);
  GD.Vertex2ii(40, PROGRESS_Y);
  GD.Macro(0);
  GD.swap();
}

static void paint_alpha()
{
  GD.ColorMask(0, 0, 0, 1);
  GD.Clear();
}

static void use_alpha()
{
  GD.ColorMask(1, 1, 1, 1);
  GD.BlendFunc(DST_ALPHA, ONE_MINUS_DST_ALPHA);
}

static char *getdir(char directory[60][8], int i)
{
  static char nm[8 + 1 + 3 + 1];
  nm[8] = 0;
  strncpy(nm, directory[i], 8);
  strcat(nm, ".jpg");
  //Serial.println("getdir:nm=" + String(nm));
  return nm;
}

static void showdir0(char directory[60][8], int num_jpgs, int yb, int sel)
{
  GD.Tag(0);
  GD.SaveContext();
  GD.ScissorSize(GD.w, 136);
  GD.cmd_gradient(0, 0, 0x3030c0, 0, 136, 0x000000);
  GD.ScissorXY(0, 136);
  GD.cmd_gradient(0, 136, 0x080800, 0, 272, 0x908880);
  GD.RestoreContext();

  GD.LineWidth(48);
  GD.Begin(RECTS);
  GD.ColorRGB(0xffffff);
  for (int j = 0; j < num_jpgs; j++) {
    int y = 40 * j - yb;
    if ((-50 < y) && (y < 272)) {
      GD.ColorA((sel == j) ? 0xff : 0xa0);
      GD.Vertex2f(16 * (240 - 100), 16 * (y + 6));
      GD.Vertex2f(16 * (240 + 100), 16 * (y + 40 - 6));
    }
  }
}

static void showdir1(char directory[60][8], int num_jpgs, int yb, int sel)
{
  GD.ColorA(0xff);
  GD.ColorRGB(0x000000);
  for (int j = 0; j < num_jpgs; j++) {
    int y = 40 * j - yb;
    if ((-50 < y) && (y < 272)) {
      GD.Tag(128 + j);
      GD.cmd_text(240, y + 18, 28, OPT_CENTER, getdir(directory, j));
    }
  }
}


void loop()
{
  int picked;

  scroller yscroll;

  yscroll.init((40L * (num_jpgs - 5)) << 4);

  do {
    GD.get_inputs();
    byte touching = (GD.inputs.x != -32768) && (GD.inputs.tag < 128);
    yscroll.run(touching, GD.inputs.y);

    showdir0(directory, num_jpgs, yscroll.base >> 4, -1);
    showdir1(directory, num_jpgs, yscroll.base >> 4, -1);
    GD.swap();
  } while (GD.inputs.tag < 128);
  picked = GD.inputs.tag - 128;
  Serial.println("picked=" + String(picked));

  GD.Clear();
  draw_progress();
  GD.cmd_loadimage(0, 0);
  if (GD.load(getdir(directory, picked), progress) ) {
    GD.Clear();
    GD.Begin(BITMAPS);
    GD.Vertex2ii(0, 0);
    GD.swap();
    delay(5000);
  }



#if 0
  uint32_t m_ptr, m_w, m_h;
  GD.cmd_getprops(m_ptr, m_w, m_h);
  GD.finish();

  uint16_t w = GD.rd16(m_w);
  uint16_t h = GD.rd16(m_h);

  byte prev_touching = 0;
  int cx, cy;
  float smooth_r;

  GD.BitmapSize(BILINEAR, BORDER, BORDER, w, h);
  float zoom = 0.3;
  int fadeout = 0;

  while (1) {
    GD.get_inputs();
    byte touching = GD.inputs.x != -32768;

    showdir0(directory, num_jpgs, yscroll.base >> 4, picked);
    showdir1(directory, num_jpgs, yscroll.base >> 4, picked);

    GD.ColorRGB(0xffffff);
    int zw = zoom * w / 2;
    int zh = zoom * h / 2;
    GD.Begin(BITMAPS);
    GD.cmd_scale(F16(zoom), F16(zoom));
    GD.cmd_setmatrix();
    GD.Vertex2ii(240 - zw, 136 - zh, 0, 0);

    zoom = min(1.0, zoom * 1.2);
    if (touching) {
Serial.println("prev_touching=" + String(prev_touching));
      if (!prev_touching) {
        cx = GD.inputs.x;
        cy = GD.inputs.y;
        smooth_r = 0;
      } else {
        int dx = cx - GD.inputs.x;
        int dy = cy - GD.inputs.y;
        float r = sqrt(dx * dx + dy * dy);
        smooth_r = .75 * smooth_r + 0.25 * r;
      }
      fadeout = 15;
    }

    if (fadeout) {
      fadeout--;
      smooth_r += 1;
Serial.println("fadeout=" + String(fadeout));
      if (fadeout == 0) break;
    }

    if (touching || fadeout) {
      int r0 = int(15 * smooth_r);
      int r1 = int(16 * smooth_r);
      float fade = min(fadeout / 15., smooth_r / 40);

      GD.SaveContext();
      GD.ClearColorA(128 * fade);
      paint_alpha();

      GD.Begin(POINTS);
      GD.BlendFunc(ZERO, ONE_MINUS_SRC_ALPHA);
      GD.PointSize(r1);
      GD.Vertex2ii(cx, cy);

      use_alpha();
      GD.ColorRGB(0x000000);
      GD.Begin(RECTS);
      GD.Vertex2ii(0, 0);
      GD.Vertex2ii(480, 272);
      GD.RestoreContext();

      paint_alpha();

      GD.Begin(POINTS);

      GD.ColorA(255 * fadeout / 15);
      GD.BlendFunc(ONE, ONE_MINUS_SRC_ALPHA);
      GD.PointSize(r1);
      GD.Vertex2ii(cx, cy);

      GD.ColorA(255);
      GD.BlendFunc(ZERO, ONE_MINUS_SRC_ALPHA);
      GD.PointSize(r0);
      GD.Vertex2ii(cx, cy);

      use_alpha();
      GD.PointSize(r1);
      GD.Vertex2ii(cx, cy);
    }
    GD.swap();

    prev_touching = touching;
  }
#endif
}
