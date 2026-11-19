/* miniaudio implementation: playback device, built-in decoders (WAV, FLAC, MP3),
   Ogg Vorbis via stb_vorbis in extras/, and lock-free PCM ring buffer. */

#define STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#undef STB_VORBIS_HEADER_ONLY
#include "extras/stb_vorbis.c"
