#ifndef FS_COMPAT_H
#define FS_COMPAT_H

#include "fast_sim.h"

// Math aliases (keeping some for convenience if needed, but using fs prefix)
static inline float fsDistance(fsVec2 a, fsVec2 b) { return fsLength(fsSub(a, b)); }
static inline float fsDistanceSq(fsVec2 a, fsVec2 b) { return fsLengthSq(fsSub(a, b)); }
static inline fsVec2 fsMulAdd(fsVec2 a, float s, fsVec2 b) { return fsAdd(a, fsMul(b, s)); }

#endif
