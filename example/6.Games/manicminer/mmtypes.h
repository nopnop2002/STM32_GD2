struct level {
  char name[33];
  uint32_t border;
  struct { byte x, y; } items[5];
  byte air;
  byte conveyordir;
  byte portalx, portaly;
  struct {
    byte a, x, y, d, x0, x1; } hguard[8];
  byte wx, wy, wd, wf;
  byte bidir;
};

struct guardian {
  byte a;
  byte x, y;
  signed char d;
  byte x0, x1;
  byte f;
};
