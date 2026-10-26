#include "bh_menu_edits.h"
#include "bh_menu_edit_names.h"
#include "bh_serial_options.h"
#include <stdio.h>
#include <string.h>
static void paint(BHMenuEdits *c, long row, const char *text) {
  unsigned char body[256];
  unsigned long n;
  if (!c || !c->rows || row < 0) {
    if (c)
      c->error = 1;
    return;
  }
  if (text) {
    n = (unsigned long)strlen(text);
    if (n > 255) {
      c->error = 1;
      return;
    }
    memcpy(body, text, n + 1);
  }
  ++c->refreshes;
  if (!bh_menu_row_refresh(c->rows, (unsigned long)row, NULL,
                           text ? body : NULL))
    c->error = 1;
}
static void short_text(char *out, const char *text) {
  strncpy(out, text, 20);
  out[20] = 0;
}
static BHInputProfile *profile(BHMenuEdits *c) {
  return c && c->settings ? c->settings->input_profile : NULL;
}
static void key(BHMenuEdits *c, long row, unsigned long offset, int table) {
  BHInputProfile *p = profile(c);
  unsigned long value;
  if (!p) {
    if (c)
      c->error = 1;
    return;
  }
  value = ((unsigned char *)p)[offset];
  if ((table == 1 && value >= 17) || (table == 2 && value >= 5)) {
    c->error = 1;
    return;
  }
  paint(c, row,
        table == 0   ? key_names[value]
        : table == 1 ? joystick_names[value]
                     : mouse_names[value]);
}
void __cdecl bh_menu_edit_video(BHMenuEdits *c, long row) {
  long value;
  char text[21];
  if (!c || !c->config)
    return;
  if (row == 0) {
    memcpy(&value, c->config->settings + 4, 4);
    if (value == 0)
      paint(c, 0, "Disable");
    else if (value == 1)
      paint(c, 0, "Allow");
  } else if (row == 1) {
    short_text(text, (const char *)c->config->settings + 12);
    paint(c, 1, text);
  } else if (row == 2) {
    memcpy(&value, c->config->settings + 8, 4);
    if (value == 0)
      paint(c, 2, "320 x 240");
    else if (value == 1)
      paint(c, 2, "640 x 480");
  }
}
void __cdecl bh_menu_edit_audio(BHMenuEdits *c, long row) {
  char text[128];
  double value;
  long percent;
  if (!c || !c->audio)
    return;
  if (row != 1 && row != 3)
    return;
  value =
      row == 1 ? bh_wave_audio_volume(c->audio) : bh_cd_audio_volume(c->audio);
  percent = (long)(.5 - value * (-10.0)) * 10;
  sprintf(text, "%d%% %s", percent, "Volume");
  paint(c, row == 1 ? 0 : 2, text);
  paint(c, row, text);
}
void __cdecl bh_menu_edit_profile(BHMenuEdits *c, long row) {
  char text[21];
  BHInputProfile *p = profile(c);
  if (row == 0 && p) {
    short_text(text, (const char *)p + 12);
    paint(c, 0, text);
  } else if (row == 1)
    paint(c, 1, NULL);
}
void __cdecl bh_menu_edit_motion(BHMenuEdits *c, long row) {
  if (row >= 1 && row <= 6)
    key(c, row, 0x1b + row, 0);
  else if (row >= 7 && row <= 9)
    key(c, row, 0x1b + row, 1);
  else if (row >= 10 && row <= 12)
    key(c, row, 0x1b + row, 2);
}
void __cdecl bh_menu_edit_primary(BHMenuEdits *c, long row) {
  if (row >= 1 && row <= 9)
    key(c, row, 0x27 + row, 0);
}
void __cdecl bh_menu_edit_secondary(BHMenuEdits *c, long row) {
  if (row >= 1 && row <= 9)
    key(c, row, 0x30 + row, 0);
}
void __cdecl bh_menu_edit_camera(BHMenuEdits *c, long row) {
  if (row >= 1 && row <= 10)
    key(c, row, 0x39 + row, 0);
  else if (row == 11)
    key(c, row, 0x45, 1);
  else if (row == 12)
    key(c, row, 0x44, 1);
  else if (row == 13)
    key(c, row, 0x47, 2);
  else if (row == 14)
    key(c, row, 0x46, 2);
}
void __cdecl bh_menu_edit_sessions(BHMenuEdits *c, long row) {
  char text[21];
  long i = row - 2;
  if (!c || !c->sessions)
    return;
  if (i < c->sessions->count) {
    if (i < 0 || i >= 8) {
      c->error = 1;
      return;
    }
    short_text(text, c->sessions->records[i].name);
    paint(c, row, text);
  } else
    paint(c, row, "<Searching>");
}
static const unsigned char *record(BHMenuEdits *c) {
  if (!c || !c->settings || !c->settings->selected_network) {
    if (c)
      c->error = 1;
    return NULL;
  }
  return c->settings->selected_network->bytes;
}
void __cdecl bh_menu_edit_modem(BHMenuEdits *c, long row) {
  const unsigned char *r = record(c);
  char text[21];
  if (!r)
    return;
  if (row == 1) {
    short_text(text, (const char *)r + 0x1c);
    paint(c, 1, text);
  } else if (row == 2) {
    if (!r[0x7c])
      paint(c, 2, "<Wait for Call>");
    else {
      short_text(text, (const char *)r + 0x7c);
      paint(c, 2, text);
    }
  }
}
void __cdecl bh_menu_edit_tcpip(BHMenuEdits *c, long row) {
  const unsigned char *r = record(c);
  char text[21];
  if (!r || row != 1)
    return;
  if (!r[0x1c])
    paint(c, 1, "<Auto Search>");
  else {
    short_text(text, (const char *)r + 0x1c);
    paint(c, 1, text);
  }
}
void __cdecl bh_menu_edit_serial(BHMenuEdits *c, long row) {
  const unsigned char *r = record(c);
  char text[128];
  long value;
  const char *label;
  if (!r || row < 1 || row > 5)
    return;
  memcpy(&value, r + 0x18 + row * 4, 4);
  switch (row) {
  case 1:
    sprintf(text, "%d", value);
    label = text;
    break;
  case 2:
    label = bh_serial_baud_label(value);
    break;
  case 3:
    label = bh_serial_stop_label(value);
    break;
  case 4:
    label = bh_serial_parity_label(value);
    break;
  default:
    label = bh_serial_flow_label(value);
    break;
  }
  paint(c, row, label);
}
static void refresh(BHMenuEdits *c, long row) {
  BHMenuSettingsRow *r;
  if (!c || !c->menu || !c->menu->rows || row < 0 ||
      (unsigned long)row >= c->menu->row_count || !(r = c->menu->rows[row])) {
    if (c)
      c->error = 1;
    return;
  }
  if (r->refresh)
    r->refresh(c->menu, row);
}
static void capture(BHMenuEdits *c, BHMenuKeyCommit commit) {
  BHMenuKeyCommit previous;
  if (!c || !c->capture)
    return;
  c->capture->settings = c->settings;
  c->capture->menu = c->menu;
  previous = c->capture->commit;
  c->capture->commit = commit;
  if (!bh_input_key_capture_begin(c->capture)) {
    c->capture->commit = previous;
    c->error = 1;
  }
}
void __cdecl bh_menu_action_motion(BHMenuEdits *c, long row) {
  BHInputProfile *p = profile(c);
  unsigned char *bytes;
  unsigned long offset;
  if (!p)
    return;
  if (row >= 7 && row <= 12) {
    bytes = (unsigned char *)p;
    offset = (unsigned long)(0x1b + row);
    bytes[offset] = (unsigned char)((bytes[offset] + 1) % (row <= 9 ? 17 : 5));
    refresh(c, row);
  } else
    capture(c, bh_menu_keys_motion);
}
void __cdecl bh_menu_action_primary(BHMenuEdits *c, long row) {
  (void)row;
  capture(c, bh_menu_keys_weapons);
}
void __cdecl bh_menu_action_secondary(BHMenuEdits *c, long row) {
  (void)row;
  capture(c, bh_menu_keys_camera);
}
void __cdecl bh_menu_action_camera(BHMenuEdits *c, long row) {
  BHInputProfile *p = profile(c);
  unsigned char *bytes;
  unsigned long offset;
  if (!p)
    return;
  if (row >= 11 && row <= 14) {
    bytes = (unsigned char *)p;
    offset = row == 11 ? 0x45 : row == 12 ? 0x44 : row == 13 ? 0x47 : 0x46;
    bytes[offset] = (unsigned char)((bytes[offset] + 1) % (row <= 12 ? 17 : 5));
    refresh(c, row);
  } else
    capture(c, bh_menu_keys_extra);
}
void __cdecl bh_menu_action_mission(BHMenuEdits *c, long row) {
  if (!c || !c->mode || !c->mission)
    return;
  *c->mode = 1;
  *c->mission = row;
}
void __cdecl bh_menu_action_ship(BHMenuEdits *c, long row) {
  short unlocked;
  unsigned long i;
  if (!c || !c->config || !c->ship || !c->mission_menu)
    return;
  for (i = 0; i < 9; ++i)
    if (!c->mission_rows[i]) {
      c->error = 1;
      return;
    }
  *c->ship = row;
  memcpy(&unlocked, c->config->settings + 2, 2);
  for (i = 0; i < 9; ++i)
    c->mission_rows[i]->action =
        (long)i <= unlocked ? bh_menu_action_mission : NULL;
  c->mission_menu->selection = unlocked;
  c->mission_menu->display_selection = (float)unlocked;
}
void __cdecl bh_menu_edit_row_callback(BHMenuSettingsMenu *m, long row) {
  BHMenuEditBridge *b = (BHMenuEditBridge *)m->user;
  static void(__cdecl *const edits[11])(BHMenuEdits *, long) = {
      bh_menu_edit_video,  bh_menu_edit_audio,    bh_menu_edit_profile,
      bh_menu_edit_motion, bh_menu_edit_primary,  bh_menu_edit_secondary,
      bh_menu_edit_camera, bh_menu_edit_sessions, bh_menu_edit_modem,
      bh_menu_edit_tcpip,  bh_menu_edit_serial};
  if (b && b->kind >= 0 && b->kind <= BH_MENU_EDIT_SERIAL)
    edits[b->kind](b->context, row);
}
