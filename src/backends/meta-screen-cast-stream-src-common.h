/*
 * Copyright (C) 2021
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms  of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 */

#ifndef META_SCREEN_CAST_STREAM_SRC_COMMON_H
#define META_SCREEN_CAST_STREAM_SRC_COMMON_H

#include "backends/meta-screen-cast-stream-src.h"

gboolean meta_screen_cast_stream_src_common_is_cursor_in_stream (MetaScreenCastStreamSrc *src,
                                                                 MetaRectangle           *stream_area);

#endif /* META_SCREEN_CAST_STREAM_SRC_COMMON_H */