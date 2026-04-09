/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * See COPYING.txt for license details.
 */

/*
 *
 * Header for D1X movie playback support (PSX extra movies)
 *
 */

#pragma once

#include <memory>
#include <span>
#include "d2x-rebirth/libmve/mvelib.h"
#include "dsx-ns.h"
#include "physfsrwops.h"

namespace dsx {

enum class play_movie_warn_missing : bool
{
	verbose,
	urgent,
};

enum class movie_play_status : bool
{
	skipped,   // movie wasn't present
	started,   // movie was present and started; it may or may not have completed
};

#if DXX_USE_OGL
#define MOVIE_WIDTH  (grd_curscreen->get_screen_width() < 640 ? static_cast<uint16_t>(640) : grd_curscreen->get_screen_width())
#define MOVIE_HEIGHT (grd_curscreen->get_screen_height() < 480 ? static_cast<uint16_t>(480) : grd_curscreen->get_screen_height())
#else
#define MOVIE_WIDTH  static_cast<uint16_t>(640)
#define MOVIE_HEIGHT static_cast<uint16_t>(480)
#endif

movie_play_status PlayMovie(std::span<const char> subtitles, const char *filename, play_movie_warn_missing);

// No-op for D1X — movies are loaded directly from DXA archives
// which are auto-mounted by PHYSFSX_addArchiveContent()
[[nodiscard]]
static inline std::unique_ptr<int> init_movies() { return nullptr; }

}
