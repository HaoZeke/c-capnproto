/* Little-endian loads for a stream frame header.
 *
 * The segment count and the segment sizes are little-endian uint32
 * on the wire. A host load of those bytes is the wrong integer on a
 * big-endian machine. */

#ifndef FRAME_LE_H
#define FRAME_LE_H

#include <stddef.h>
#include <stdint.h>

static inline uint32_t frame_le32(const uint8_t *p)
{
	return (uint32_t) p[0]
		| ((uint32_t) p[1] << 8)
		| ((uint32_t) p[2] << 16)
		| ((uint32_t) p[3] << 24);
}

#endif
