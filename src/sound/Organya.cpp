/* Based on */
/* SIMPLE CAVE STORY MUSIC PLAYER (Organya) */
/* Written by Joel Yliluoma -- http://iki.fi/bisqwit/ */
/* https://bisqwit.iki.fi/jutut/kuvat/programming_examples/doukutsu-org/orgplay.cc */

#include "Organya.h"

#include "../assets.h"
#include "../ResourceManager.h"
#include "../common/glob.h"
#include "../common/misc.h"
#include "../Utils/Logger.h"
#include "../Utils/Common.h"
#include "../settings.h"
#include "Pixtone.h"
#include "SoundManager.h"

#include <SDL.h>
#include <SDL_mixer.h>
#include <cmath>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

//------------------------------------------------------------------------------

#define MIN_VOLUME 0.204

namespace NXE
{
namespace Sound
{

static int16_t WaveTable[100 * 256] = {0};
static std::vector<int16_t> DrumSamples[8];

Organya::Organya()
{
  LOG_INFO("Organya init...");
  _loadWavetable();
  _loadDrums();
  LOG_INFO("Organya init done");
}

Organya::~Organya() {}

Organya *Organya::getInstance()
{
  return Singleton<Organya>::get();
}

void Organya::init() {}

void Organya::shutdown() {}

//------------------------------------------------------------------------------

bool Organya::_loadWavetable()
{
  AFile *fp
      = aopen(widen(ResourceManager::getInstance()->getPath("wavetable.dat", false)).c_str());
  if (!fp)
  {
    LOG_ERROR("Unable to open wavetable.dat");
    return false;
  }

  for (size_t a = 0; a < 100 * 256; ++a)
    WaveTable[a] = (signed char)agetc(fp);

  aclose(fp);

  return true;
}

bool Organya::_loadDrums()
{
  for (uint8_t drumno = 0; drumno < 8; ++drumno)
  {
    if (drum_pxt_table[drumno] == SFX::SND_NULL)
      continue; // Leave that non-existed drum file unloaded

    // Load the drum parameters
    char fname[256];
    sprintf(fname, "%sfx%02x.pxt", ResourceManager::getInstance()->getPathForDir("pxt/").c_str(), (uint8_t) drum_pxt_table[drumno]);

    stPXSound snd;

    LOG_DEBUG("load_drum: loading {} into drum index {}", fname, drumno);

    if (!snd.load(fname))
      return false;

    snd.render();

    // Synthesize and mix the drum's component channels
    auto &sample = DrumSamples[drumno];

    if (snd.final_size > sample.size())
      sample.resize(snd.final_size, 0);
    for (size_t a = 0; a < snd.final_size; ++a)
      sample[a] = snd.final_buffer[a];

    snd.freeBuf();
  }

  return true;
}

void Organya::_setPlayPosition(uint32_t pos)
{
  song.cur_beat = pos;
}

bool Song::Load(const std::string &fname)
{
  AFile *fp = aopen(widen(fname).c_str());
  if (!fp)
  {
    LOG_WARN("Song::Load: no such file: '{}'", fname);
    return false;
  }
  for (int i = 0; i < 6; ++i)
    agetc(fp); // Ignore file signature ("Org-02")
  last_pos    = 0;
  cur_beat    = 0;
  total_beats = 0;
  // Load song parameters
  ms_per_beat    = ageti(fp);
  steps_per_bar  = agetc(fp);
  beats_per_step = agetc(fp);
  loop_start     = agetl(fp);
  loop_end       = agetl(fp);
  // Load each instrument parameters (and initialize them)
  for (auto &i : ins)
  {
    i = {ageti(fp), agetc(fp), 1, 255, false, agetc(fp) != 0, ageti(fp), {}, 0, 0, 0, 0, 0, 0, 0};
  }
  // Load events for each instrument
  for (auto &i : ins)
  {
    std::vector<std::pair<int, Instrument::Event>> events(i.n_events);
    for (auto &n : events)
      n.first = agetl(fp);
    for (auto &n : events)
      n.second.note = agetc(fp);
    for (auto &n : events)
      n.second.length = agetc(fp);
    for (auto &n : events)
      n.second.volume = agetc(fp);
    for (auto &n : events)
      n.second.panning = agetc(fp);
    i.events.insert(events.begin(), events.end());
  }
  aclose(fp);
  return true;
}

static const int frequency_table[12] = {262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494};
//static const int octave_table[8] = {32, 64, 128, 256, 512, 1024, 2048, 4096};
static const int octave_table[8] = {32, 64, 64, 128, 128, 128, 128, 128};
static const int wave_length_table[8] = {256, 256, 128, 128, 64, 32, 16, 8};
static const int panning_table[13] = {0, 43, 86, 129, 172, 215, 256, 297, 340, 383, 426, 469, 512};

void Song::Synth()
{
  unsigned sampling_rate = SAMPLE_RATE;
  // Determine playback settings:
  double samples_per_millisecond = sampling_rate * 1e-3, master_volume = 4e-6;
  int samples_per_beat = ms_per_beat * samples_per_millisecond; // rounded.

  // Begin synthesis

  last_gen_tick = SDL_GetTicks();
  last_gen_beat = cur_beat;

  if (cur_beat == loop_end)
    cur_beat = loop_start;
  // Synthesize this beat in stereo sound (two channels).
  samples.clear();
  samples.resize(0);
  samples.resize(samples_per_beat * 2, 0.f);

  for (auto &i : ins)
  {
    // Check if there is an event for this beat
    auto j = i.events.find(cur_beat);
    if (j != i.events.end())
    {
      auto &event = j->second;
      if (event.note != 255)
      {
        int octave = event.note / 12;
        int key = event.note % 12;
        double freq = frequency_table[key] * octave_table[octave] + (i.tuning - 1000);

        if (&i >= &ins[8])
        {
          const auto idx = &i - &ins[0];
          const auto &d  = DrumSamples[idx - 8];
          i.wave_step = 1;
          i.phaseinc = (event.note * 800 + 100) / (double) sampling_rate;
          i.phaseacc = 0;
          i.cur_wave = &d[0];
          i.cur_wavesize = d.size();
          i.cur_length = d.size() / i.phaseinc;
        } else {
          i.wave_step = (256 / wave_length_table[octave]);
          i.phaseinc = (freq / sampling_rate);
          i.phaseacc = fmod(i.phaseacc, wave_length_table[octave]);
          i.cur_wave     = &WaveTable[256 * (i.wave % 100)];
          i.cur_wavesize = 256;
          if (i.pipi && i.last_note == 255) {
            i.cur_length = int((octave + 1) * 4 * wave_length_table[octave] /*/ i.phaseinc*/);
          } else {
            i.cur_length = (event.length * samples_per_beat);
          }
        }

        i.last_note = event.note;

        if (i.cur_wavesize <= 0)
        {
          i.cur_length = 0;
        }
      }

      if (i.last_note != 255) {
        if (event.volume != 255) {
          //i.cur_vol = event.volume * master_volume;
          i.cur_vol = powf(10.0, ((event.volume - 255) * 8) / 2000.0) / 128.0;
        }

        if (event.panning != 255) {
          i.cur_pan = event.panning;
        }
      }
    }

    // Generate wave data. Calculate left & right volumes...
    const double pan = (panning_table[clamp(i.cur_pan, 0, 12)] - 256) * 10.0;
    double left = 1.0;
    double right = 1.0;

    if (pan < 0) {
      right = pow(10.0, pan / 2000.0);
    } else if (pan > 0) {
      left = pow(10.0, -pan / 2000.0);
    }

    left *= i.cur_vol;
    right *= i.cur_vol;

    int n = samples_per_beat > i.cur_length ? i.cur_length : samples_per_beat;
    for (int p = 0; p < n; ++p)
    {
      const double pos = i.phaseacc * i.wave_step;
      // Take a sample from the wave data.
      double sample = 0;

      if (settings->music_interpolation == 1)
      {
        // Perform linear interpolation
        unsigned int position_integral = unsigned(pos);
        const double position_fractional = (pos - position_integral);

        const double sample1 = i.cur_wave[position_integral % i.cur_wavesize];
        const double sample2 = i.cur_wave[(position_integral + 1) % i.cur_wavesize];

        sample = sample1 + (sample2 - sample1) * position_fractional;
      }
      else if (settings->music_interpolation == 2)
      {
        // Perform cubic interpolation
        const unsigned int position_integral = unsigned(pos) % i.cur_wavesize;
        const double position_fractional = (pos - (double)((int) pos));

        const float s1 = (float) (i.cur_wave[position_integral]);
        const float s2 = (float) (i.cur_wave[clamp(position_integral + 1, (unsigned int) 0, (unsigned int) (i.cur_wavesize - 1))]);
        const float sn = (float) (i.cur_wave[clamp(position_integral + 2, (unsigned int) 0, (unsigned int) (i.cur_wavesize - 1))]);
        const float sp = (float) (i.cur_wave[MAX(((int) position_integral) - 1, 0)]);
        const float mu2 = position_fractional * position_fractional;
        const float a0 = sn - s2 - sp + s1;
        const float a1 = sp - s1 - a0;
        const float a2 = s2 - sp;
        const float a3 = s1;

        sample = a0 * position_fractional * mu2 + a1 * mu2 + a2 * position_fractional + a3;
      }
      else
      {
        // Perform nearest-neighbour interpolation
        sample = i.cur_wave[ unsigned(pos) % i.cur_wavesize ];
      }


      // Save audio in float32 format:
      samples[p * 2 + 0] += sample * left;
      samples[p * 2 + 1] += sample * right;
      i.phaseacc += i.phaseinc;
    }
    i.cur_length -= n;
  }
  ++cur_beat;
}

bool Organya::load(const std::string &fname)
{
  song.loaded  = false;
  song.playing = false;

  if (!song.Load(fname))
    return false;
  // Reset position
  _setPlayPosition(0);

  // Set as loaded
  song.loaded = true;

  return true;
}

std::function<void(void *, uint8_t *, int)> musicCallback;

void myMusicPlayer(void *udata, uint8_t *stream, int len)
{
  musicCallback(udata, stream, len);
}

void Organya::_musicCallback(void *udata, uint8_t *stream, uint32_t len)
{
  SDL_memset(stream, 0, len);
  if (!song.playing)
    return;
  int16_t *str = reinterpret_cast<int16_t *>(stream);

  uint32_t idx = song.last_pos;
  for (uint32_t i = 0; i < len / 2; i++)
  {
    if (idx >= song.samples.size())
    {
      song.Synth();
      idx = 0;
    }
    // extended range
    int32_t sample = song.samples[idx] * 32767.0 * volume * (double)(settings->music_volume / 100.);
    // clip to int16
    if (sample > 32767)
    {
      sample = 32767;
    }
    if (sample < -32768)
    {
      sample = -32768;
    }

    str[i] = (int16_t)sample;
    idx++;
  }

  song.last_pos = idx;
}

bool Organya::start(int startBeat)
{
  if (!song.loaded)
    return false;

  song.playing = true;
  fading       = false;
  volume       = 0.75;
  musicCallback
      = std::bind(&Organya::_musicCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
  _setPlayPosition(startBeat);
  song.Synth();
  Mix_HookMusic(myMusicPlayer, NULL);
  return true;
}

uint32_t Organya::stop()
{
  if (song.playing)
  {
    /* Okay, this is hackish, and still misses a beat or two.
       Sadly, there's no better way on SDL, because it writes
       to audio device in bulk and there's no way of knowing
       how many samples actually played.
    */
    uint32_t delta    = SDL_GetTicks() - song.last_gen_tick;
    uint32_t beats    = (double)delta / (double)song.ms_per_beat;
    uint32_t cur_beat = song.last_gen_beat + beats;
    if (cur_beat >= song.loop_end)
    {
      cur_beat = song.loop_start + (cur_beat - song.loop_end);
    }

    song.playing = false;
    Mix_HookMusic(NULL, NULL);
    return cur_beat;
  }
  return 0;
}

bool Organya::isPlaying()
{
  return song.playing;
}

void Organya::fade()
{
  fading         = true;
  last_fade_time = 0;
}

void Organya::pause()
{
  song.playing = false;
}

void Organya::resume()
{
  song.playing = true;
}

void Organya::setVolume(float newVolume)
{
  volume = newVolume;
}

void Organya::runFade()
{
  if (!fading)
    return;
  uint32_t curtime = SDL_GetTicks();

  if ((curtime - last_fade_time) >= 25)
  {
    float newvol = (volume - 0.01);
    if (newvol <= 0.0)
    {
      fading = false;
      stop();
    }
    else
    {
      setVolume(newvol);
    }
    last_fade_time = curtime;
  }
}

} // namespace Sound
} // namespace NXE
