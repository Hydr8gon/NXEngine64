
#ifndef _PROFILE_H
#define _PROFILE_H
#include "game.h"
#include "p_arms.h"
#include "player.h"
#include <vector>

// how many bytes of data long a profile.dat is.
#define PROFILE_LENGTH 0x604

struct Profile
{
  int stage = 0;
  int songno = 0;
  int px, py, pdir = 0;
  int hp, maxhp, num_whimstars = 0;
  uint32_t equipmask = 0;

  int curWeapon = 0;
  struct
  {
    bool hasWeapon = false;
    int level = 0;
    int xp = 0;
    int ammo, maxammo = 0;
  } weapons[WPN_COUNT];

  std::vector<int> wpnOrder = {};

  int inventory[MAX_INVENTORY] = { 0 };
  int ninventory = 0;

  bool flags[NUM_GAMEFLAGS] = { false };

  struct
  {
    int slotno = 0;
    int scriptno = 0;
  } teleslots[NUM_TELEPORTER_SLOTS];
  int num_teleslots = 0;
};

// Implemented in platform main.cpp
uint8_t *profile_load_data(int num);
bool profile_save_data(int num, uint8_t *data, size_t size);

bool profile_load(int num, Profile *file);
bool profile_save(int num, Profile *file);
std::string GetProfileName(int num);
bool ProfileExists(int num);
bool AnyProfileExists();

#endif
