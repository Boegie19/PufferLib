#ifndef FAST_SIM_H
#define FAST_SIM_H

#include <math.h>
#include <float.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct fsVec2 {
    float x;
    float y;
} fsVec2;

typedef struct fsRot {
    float c, s;
} fsRot;

static const fsVec2 fsVec2_zero = {0.0f, 0.0f};
static const fsRot fsRot_identity = {1.0f, 0.0f};

typedef struct fsTransform {
    fsVec2 p;
    fsRot q;
} fsTransform;

typedef struct fsAABB {
    fsVec2 min;
    fsVec2 max;
} fsAABB;

typedef enum fsShapeType {
    FS_CIRCLE,
    FS_BOX
} fsShapeType;

typedef struct fsShape {
    fsShapeType type;
    union {
        struct {
            float radius;
        } circle;
        struct {
            fsVec2 halfExtents;
        } box;
    };
} fsShape;

// Body index type (used instead of pointer for SoA layout)
typedef uint16_t fsBodyIndex;
#define FS_BODY_INVALID 0xFFFF

// Helper macros for body property access (for easier transition from AoS)
#define FS_BODY_POS(w, i) ((w)->positions[i])
#define FS_BODY_VEL(w, i) ((w)->velocities[i])
#define FS_BODY_ANGLE(w, i) ((w)->angles[i])
#define FS_BODY_ANGULAR_VEL(w, i) ((w)->angularVelocities[i])
#define FS_BODY_FORCE(w, i) ((w)->forces[i])
#define FS_BODY_TORQUE(w, i) ((w)->torques[i])
#define FS_BODY_MASS(w, i) ((w)->masses[i])
#define FS_BODY_INV_MASS(w, i) ((w)->invMasses[i])
#define FS_BODY_INERTIA(w, i) ((w)->inertias[i])
#define FS_BODY_INV_INERTIA(w, i) ((w)->invInertias[i])
#define FS_BODY_FRICTION(w, i) ((w)->frictions[i])
#define FS_BODY_RESTITUTION(w, i) ((w)->restitutions[i])
#define FS_BODY_SHAPE(w, i) ((w)->shapes[i])
#define FS_BODY_USER_DATA(w, i) ((w)->userDatas[i])
#define FS_BODY_IS_ACTIVE(w, i) ((w)->isActive[i])
#define FS_BODY_IS_STATIC(w, i) ((w)->isStatic[i])
#define FS_BODY_IS_SENSOR(w, i) ((w)->isSensor[i])
#define FS_BODY_CATEGORY_BITS(w, i) ((w)->categoryBits[i])
#define FS_BODY_MASK_BITS(w, i) ((w)->maskBits[i])
#define FS_BODY_BOUNDING_RADIUS(w, i) ((w)->boundingRadii[i])

// Math helpers
static inline fsVec2 fsAdd(fsVec2 a, fsVec2 b) { return (fsVec2){a.x + b.x, a.y + b.y}; }
static inline fsVec2 fsSub(fsVec2 a, fsVec2 b) { return (fsVec2){a.x - b.x, a.y - b.y}; }
static inline fsVec2 fsMul(fsVec2 a, float s) { return (fsVec2){a.x * s, a.y * s}; }
static inline float fsDot(fsVec2 a, fsVec2 b) { return a.x * b.x + a.y * b.y; }
static inline float fsCross(fsVec2 a, fsVec2 b) { return a.x * b.y - a.y * b.x; }
static inline fsVec2 fsCrossSV(float s, fsVec2 v) { return (fsVec2){-s * v.y, s * v.x}; }
static inline float fsLengthSq(fsVec2 v) { return v.x * v.x + v.y * v.y; }
static inline float fsLength(fsVec2 v) { return sqrtf(fsLengthSq(v)); }
static inline fsVec2 fsNormalize(fsVec2 v) {
    float len = fsLength(v);
    if (len < FLT_EPSILON) return (fsVec2){0, 0};
    return fsMul(v, 1.0f / len);
}

static inline fsVec2 fsRotMul(fsRot q, fsVec2 v) {
    return (fsVec2){q.c * v.x - q.s * v.y, q.s * v.x + q.c * v.y};
}

static inline fsVec2 fsRotMulT(fsRot q, fsVec2 v) {
    return (fsVec2){q.c * v.x + q.s * v.y, -q.s * v.x + q.c * v.y};
}

static inline fsRot fsMakeRot(float angle) {
    return (fsRot){cosf(angle), sinf(angle)};
}

static inline float fsShapeBoundingRadius(const fsShape* shape) {
    if (shape->type == FS_CIRCLE) {
        return shape->circle.radius;
    }
    // circumscribed circle radius for a box
    return sqrtf((shape->box.halfExtents.x * shape->box.halfExtents.x) +
                 (shape->box.halfExtents.y * shape->box.halfExtents.y));
}

// Collision Resolution Primitives
typedef struct fsContact {
    fsVec2 normal;
    float depth;
    fsVec2 point;
    fsBodyIndex a;
    fsBodyIndex b;
} fsContact;

static inline bool fsTestCircleCircle(fsVec2 p1, float r1, fsVec2 p2, float r2, fsContact* out) {
    fsVec2 d = fsSub(p2, p1);
    float distSq = fsLengthSq(d);
    float radiusSum = r1 + r2;
    if (distSq > radiusSum * radiusSum) return false;
    
    float dist = sqrtf(distSq);
    if (dist > 0) {
        out->normal = fsMul(d, 1.0f / dist);
        out->depth = radiusSum - dist;
        out->point = fsAdd(p1, fsMul(out->normal, r1 - out->depth * 0.5f));
    } else {
        out->normal = (fsVec2){1, 0};
        out->depth = radiusSum;
        out->point = p1;
    }
    return true;
}

static inline bool fsTestCircleBox(fsVec2 cp, float r, fsVec2 bp, fsVec2 bh, fsRot bq, fsContact* out) {
    // Transform circle to box local space
    fsVec2 localCircle = fsRotMulT(bq, fsSub(cp, bp));
    
    // Closest point on box to circle
    fsVec2 closest;
    closest.x = fmaxf(-bh.x, fminf(localCircle.x, bh.x));
    closest.y = fmaxf(-bh.y, fminf(localCircle.y, bh.y));
    
    fsVec2 d = fsSub(localCircle, closest);
    float distSq = fsLengthSq(d);
    
    if (distSq > r * r) return false;
    
    float dist = sqrtf(distSq);
    if (dist > FLT_EPSILON) {
        out->normal = fsRotMul(bq, fsMul(d, 1.0f / dist));
        out->depth = r - dist;
    } else {
        // Circle center is inside box
        // Find closest face
        float dx = bh.x - fabsf(localCircle.x);
        float dy = bh.y - fabsf(localCircle.y);
        
        if (dx < dy) {
            out->normal = fsRotMul(bq, (fsVec2){localCircle.x > 0 ? 1.0f : -1.0f, 0});
            out->depth = r + dx;
        } else {
            out->normal = fsRotMul(bq, (fsVec2){0, localCircle.y > 0 ? 1.0f : -1.0f});
            out->depth = r + dy;
        }
    }
    out->point = fsSub(cp, fsMul(out->normal, r - out->depth * 0.5f));
    return true;
}

#define MAX_BODIES 1024

// Spatial hash grid for broadphase optimization
#define GRID_CELL_SIZE 64.0f
#define GRID_TABLE_SIZE 4096
#define MAX_GRID_BODIES 16

typedef struct GridCell {
    uint16_t bodyIndices[MAX_GRID_BODIES];
    uint16_t count;
} GridCell;

typedef enum fsContactType {
    FS_CONTACT_BEGIN,
    FS_CONTACT_END
} fsContactType;

typedef struct fsContactEvent {
    fsContactType type;
    fsBodyIndex a;
    fsBodyIndex b;
    fsVec2 normal;
} fsContactEvent;

#define MAX_EVENTS 4096
// Structure of Arrays (SoA) for better cache locality and SIMD
typedef struct fsWorld {
    // Position and velocity
    fsVec2 positions[MAX_BODIES];
    fsVec2 velocities[MAX_BODIES];
    float angles[MAX_BODIES];
    float angularVelocities[MAX_BODIES];
    
    // Force and torque (cleared each frame)
    fsVec2 forces[MAX_BODIES];
    float torques[MAX_BODIES];
    
    // Mass properties
    float masses[MAX_BODIES];
    float invMasses[MAX_BODIES];
    float inertias[MAX_BODIES];
    float invInertias[MAX_BODIES];
    
    // Material properties
    float frictions[MAX_BODIES];
    float restitutions[MAX_BODIES];
    
    // Collision shapes
    fsShape shapes[MAX_BODIES];
    
    // User data
    void* userDatas[MAX_BODIES];
    
    // State flags
    bool isActive[MAX_BODIES];
    bool isStatic[MAX_BODIES];
    bool isSensor[MAX_BODIES];
    
    // Collision filtering
    uint32_t categoryBits[MAX_BODIES];
    uint32_t maskBits[MAX_BODIES];
    
    // Cached values for performance
    float boundingRadii[MAX_BODIES];
    int32_t gridMinX[MAX_BODIES], gridMaxX[MAX_BODIES];
    int32_t gridMinY[MAX_BODIES], gridMaxY[MAX_BODIES];
    fsRot cachedRotations[MAX_BODIES]; // Cached rotation matrices for static bodies
    bool rotationCached[MAX_BODIES]; // Flag indicating if rotation is cached

    // World properties
    fsVec2 gravity;
    fsContactEvent events[MAX_EVENTS];
    uint16_t numEvents;
    GridCell grid[GRID_TABLE_SIZE];
    uint32_t pairCheckTimestamp[MAX_BODIES];
    uint32_t currentTimestamp;
    uint16_t freeList[MAX_BODIES];
    uint16_t freeListCount;
} fsWorld;

static inline void fsWorld_Init(fsWorld* w, fsVec2 gravity) {
    memset(w, 0, sizeof(fsWorld));
    w->gravity = gravity;
    // Initialize free list with all body indices in reverse order
    for (uint16_t i = 0; i < MAX_BODIES; i++) {
        w->freeList[i] = MAX_BODIES - 1 - i;
        w->cachedRotations[i] = fsRot_identity;
    }
    w->freeListCount = MAX_BODIES;
}

// Get rotation matrix, using cached version for static bodies
static inline fsRot fsGetRotation(fsWorld* w, uint16_t index) {
    if (w->isStatic[index] && w->rotationCached[index]) {
        return w->cachedRotations[index];
    }
    fsRot rot = fsMakeRot(w->angles[index]);
    if (w->isStatic[index]) {
        w->cachedRotations[index] = rot;
        w->rotationCached[index] = true;
    }
    return rot;
}

// Spatial hash grid functions
static inline uint32_t fsGridHash(int32_t x, int32_t y) {
    // Improved hash with better bit mixing
    uint32_t h = (uint32_t)x * 73856093 ^ (uint32_t)y * 83492791;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h & (GRID_TABLE_SIZE - 1);
}

static inline void fsGridClear(GridCell* grid) {
    memset(grid, 0, sizeof(GridCell) * GRID_TABLE_SIZE);
}

static inline void fsGridInsert(GridCell* grid, fsVec2 pos, float radius, uint16_t bodyIndex) {
    int32_t minX = (int32_t)floorf((pos.x - radius) / GRID_CELL_SIZE);
    int32_t maxX = (int32_t)floorf((pos.x + radius) / GRID_CELL_SIZE);
    int32_t minY = (int32_t)floorf((pos.y - radius) / GRID_CELL_SIZE);
    int32_t maxY = (int32_t)floorf((pos.y + radius) / GRID_CELL_SIZE);

    for (int32_t x = minX; x <= maxX; x++) {
        for (int32_t y = minY; y <= maxY; y++) {
            uint32_t hash = fsGridHash(x, y);
            GridCell* cell = &grid[hash];
            if (cell->count < MAX_GRID_BODIES) {
                cell->bodyIndices[cell->count++] = bodyIndex;
            }
        }
    }
}

// Query spatial grid for bodies in a given AABB region
// Returns true if callback returns true for any body
static inline bool fsGridQueryAABB(GridCell* grid, fsAABB query, uint32_t maskBits,
                                   bool (*callback)(fsWorld*, uint16_t, void*), void* userData, fsWorld* w) {
    int32_t minX = (int32_t)floorf(query.min.x / GRID_CELL_SIZE);
    int32_t maxX = (int32_t)floorf(query.max.x / GRID_CELL_SIZE);
    int32_t minY = (int32_t)floorf(query.min.y / GRID_CELL_SIZE);
    int32_t maxY = (int32_t)floorf(query.max.y / GRID_CELL_SIZE);

    uint16_t checked[MAX_BODIES];
    uint16_t checkedCount = 0;

    for (int32_t x = minX; x <= maxX; x++) {
        for (int32_t y = minY; y <= maxY; y++) {
            uint32_t hash = fsGridHash(x, y);
            GridCell* cell = &grid[hash];

            for (uint16_t i = 0; i < cell->count; i++) {
                uint16_t bodyIdx = cell->bodyIndices[i];

                // Skip if already checked
                bool alreadyChecked = false;
                for (uint16_t j = 0; j < checkedCount; j++) {
                    if (checked[j] == bodyIdx) {
                        alreadyChecked = true;
                        break;
                    }
                }
                if (alreadyChecked) continue;
                if (checkedCount < MAX_BODIES) {
                    checked[checkedCount++] = bodyIdx;
                }

                // Check mask bits
                if (!(w->categoryBits[bodyIdx] & maskBits)) continue;

                if (callback(w, bodyIdx, userData)) {
                    return true;
                }
            }
        }
    }
    return false;
}

static inline fsBodyIndex fsWorld_CreateBody(fsWorld* w) {
    if (w->freeListCount == 0) {
        return FS_BODY_INVALID;
    }
    uint16_t index = w->freeList[--w->freeListCount];
    // Initialize body at index
    w->positions[index] = fsVec2_zero;
    w->velocities[index] = fsVec2_zero;
    w->angles[index] = 0.0f;
    w->angularVelocities[index] = 0.0f;
    w->forces[index] = fsVec2_zero;
    w->torques[index] = 0.0f;
    w->masses[index] = 0.0f;
    w->invMasses[index] = 1.0f;
    w->inertias[index] = 0.0f;
    w->invInertias[index] = 0.0f;
    w->frictions[index] = 0.0f;
    w->restitutions[index] = 1.0f;
    memset(&w->shapes[index], 0, sizeof(fsShape));
    w->userDatas[index] = NULL;
    w->isActive[index] = true;
    w->isStatic[index] = false;
    w->isSensor[index] = false;
    w->categoryBits[index] = 0;
    w->maskBits[index] = 0;
    w->boundingRadii[index] = 0.0f;
    w->gridMinX[index] = 0; w->gridMaxX[index] = 0;
    w->gridMinY[index] = 0; w->gridMaxY[index] = 0;
    w->cachedRotations[index] = fsRot_identity;
    w->rotationCached[index] = false;
    return index;
}

static inline void fsWorld_DestroyBody(fsWorld* w, fsBodyIndex index) {
    if (index == FS_BODY_INVALID) return;
    w->isActive[index] = false;
    w->categoryBits[index] = 0;
    w->maskBits[index] = 0;
    // Return body to free list
    w->freeList[w->freeListCount++] = index;
}

// Integration
static inline void fsBody_Integrate(fsWorld* w, uint16_t index, float dt) {
    if (w->isStatic[index]) return;
    
    // Euler integration
    fsVec2 acceleration = fsMul(w->forces[index], w->invMasses[index]);
    w->velocities[index] = fsAdd(w->velocities[index], fsMul(acceleration, dt));
    w->positions[index] = fsAdd(w->positions[index], fsMul(w->velocities[index], dt));
    
    float angularAccel = w->torques[index] * w->invInertias[index];
    w->angularVelocities[index] += angularAccel * dt;
    w->angles[index] += w->angularVelocities[index] * dt;
    
    // Reset forces
    w->forces[index] = (fsVec2){0, 0};
    w->torques[index] = 0;
}

// Simple impulse resolution
static inline void fsResolveCollision(fsWorld* w, fsContact* c) {
    const uint16_t idxA = c->a;
    const uint16_t idxB = c->b;
    if (w->isStatic[idxA] && w->isStatic[idxB]) return;
    if (w->isSensor[idxA] || w->isSensor[idxB]) return;
    
    fsVec2 n = c->normal;
    
    // Relative velocity
    fsVec2 rv = fsSub(w->velocities[idxB], w->velocities[idxA]);
    
    // Relative velocity along normal
    float velAlongNormal = fsDot(rv, n);
    
    // Do not resolve if velocities are separating
    if (velAlongNormal > 0) return;
    
    // Restitution
    float e = fminf(w->restitutions[idxA], w->restitutions[idxB]);
    
    // Impulse magnitude
    float j = -(1.0f + e) * velAlongNormal;
    j /= w->invMasses[idxA] + w->invMasses[idxB];
    
    // Apply impulse
    fsVec2 impulse = fsMul(n, j);
    if (!w->isStatic[idxA]) w->velocities[idxA] = fsSub(w->velocities[idxA], fsMul(impulse, w->invMasses[idxA]));
    if (!w->isStatic[idxB]) w->velocities[idxB] = fsAdd(w->velocities[idxB], fsMul(impulse, w->invMasses[idxB]));
    
    // Positional correction (anti-sink)
    const float percent = 0.2f; // penetration percentage to correct
    const float slop = 0.01f; // penetration allowance
    fsVec2 correction = fsMul(n, fmaxf(c->depth - slop, 0.0f) / (w->invMasses[idxA] + w->invMasses[idxB]) * percent);
    if (!w->isStatic[idxA]) w->positions[idxA] = fsSub(w->positions[idxA], fsMul(correction, w->invMasses[idxA]));
    if (!w->isStatic[idxB]) w->positions[idxB] = fsAdd(w->positions[idxB], fsMul(correction, w->invMasses[idxB]));
}

static inline void fsWorld_Step(fsWorld* w, float dt) {
    uint16_t activeIndices[MAX_BODIES];
    uint16_t activeCount = 0;
    #pragma clang loop vectorize(enable)
    for (uint16_t i = 0; i < MAX_BODIES; i++) {
        if (w->isActive[i]) {
            activeIndices[activeCount++] = i;
        }
    }

    // 1. Integration & cache updates
    #pragma clang loop vectorize(enable)
    for (uint16_t i = 0; i < activeCount; i++) {
        uint16_t idx = activeIndices[i];
        fsBody_Integrate(w, idx, dt);
        // Cache bounding radius and grid cell bounds
        w->boundingRadii[idx] = fsShapeBoundingRadius(&w->shapes[idx]);
        const float r = w->boundingRadii[idx];
        w->gridMinX[idx] = (int32_t)floorf((w->positions[idx].x - r) / GRID_CELL_SIZE);
        w->gridMaxX[idx] = (int32_t)floorf((w->positions[idx].x + r) / GRID_CELL_SIZE);
        w->gridMinY[idx] = (int32_t)floorf((w->positions[idx].y - r) / GRID_CELL_SIZE);
        w->gridMaxY[idx] = (int32_t)floorf((w->positions[idx].y + r) / GRID_CELL_SIZE);
    }

    // 2. Build spatial hash grid for broadphase
    fsGridClear(w->grid);
    #pragma clang loop vectorize(enable)
    for (uint16_t i = 0; i < activeCount; i++) {
        uint16_t idx = activeIndices[i];
        fsGridInsert(w->grid, w->positions[idx], w->boundingRadii[idx], idx);
    }

    // 3. Collision detection & resolution using spatial grid
    w->currentTimestamp++;
    
    for (uint16_t i = 0; i < activeCount; i++) {
        uint16_t idxA = activeIndices[i];
        const float radius = w->boundingRadii[idxA];
        const uint32_t catA = w->categoryBits[idxA];
        const uint32_t maskA = w->maskBits[idxA];
        const bool staticA = w->isStatic[idxA];
        const bool sensorA = w->isSensor[idxA];
        const fsVec2 posA = w->positions[idxA];
        
        // Use cached grid cell bounds
        const int32_t minX = w->gridMinX[idxA];
        const int32_t maxX = w->gridMaxX[idxA];
        const int32_t minY = w->gridMinY[idxA];
        const int32_t maxY = w->gridMaxY[idxA];

        // Check collisions with bodies in neighboring cells
        for (int32_t x = minX; x <= maxX; x++) {
            for (int32_t y = minY; y <= maxY; y++) {
                uint32_t hash = fsGridHash(x, y);
                GridCell* cell = &w->grid[hash];
                
                for (uint16_t k = 0; k < cell->count; k++) {
                    uint16_t idxB = cell->bodyIndices[k];
                    if (__builtin_expect(idxB == idxA, 0)) continue;
                    
                    // Avoid checking the same pair twice using timestamp
                    if (__builtin_expect(w->pairCheckTimestamp[idxB] == w->currentTimestamp, 0)) continue;
                    w->pairCheckTimestamp[idxB] = w->currentTimestamp;
                    
                    // Early static-static and category/mask checks
                    if (__builtin_expect(staticA && w->isStatic[idxB], 0)) continue;
                    if (__builtin_expect(!(catA & w->maskBits[idxB]) && !(w->categoryBits[idxB] & maskA), 0)) continue;

                    // Broadphase cull: reject obviously far-apart pairs
                    const float rb = w->boundingRadii[idxB];
                    const float maxDist = radius + rb;
                    const fsVec2 diff = fsSub(posA, w->positions[idxB]);
                    const float distSq = diff.x * diff.x + diff.y * diff.y;
                    if (__builtin_expect(distSq > (maxDist * maxDist), 0)) continue;

                    fsContact contact;
                    bool hit = false;
                    
                    if (w->shapes[idxA].type == FS_CIRCLE && w->shapes[idxB].type == FS_CIRCLE) {
                        hit = fsTestCircleCircle(posA, w->shapes[idxA].circle.radius, w->positions[idxB], w->shapes[idxB].circle.radius, &contact);
                    } else if (w->shapes[idxA].type == FS_CIRCLE && w->shapes[idxB].type == FS_BOX) {
                        hit = fsTestCircleBox(posA, w->shapes[idxA].circle.radius, w->positions[idxB], w->shapes[idxB].box.halfExtents, fsGetRotation(w, idxB), &contact);
                    } else if (w->shapes[idxA].type == FS_BOX && w->shapes[idxB].type == FS_CIRCLE) {
                        hit = fsTestCircleBox(w->positions[idxB], w->shapes[idxB].circle.radius, posA, w->shapes[idxA].box.halfExtents, fsGetRotation(w, idxA), &contact);
                        // Flip normal
                        contact.normal = fsMul(contact.normal, -1.0f);
                    }
                    
                    if (__builtin_expect(hit, 0)) {
                        contact.a = idxA;
                        contact.b = idxB;
                        if (!sensorA && !w->isSensor[idxB]) {
                            fsResolveCollision(w, &contact);
                        }
                        
                        if (__builtin_expect(w->numEvents < MAX_EVENTS, 1)) {
                            w->events[w->numEvents++] = (fsContactEvent){
                                .type = FS_CONTACT_BEGIN,
                                .a = idxA,
                                .b = idxB,
                                .normal = contact.normal
                            };
                        }
                    }
                }
            }
        }
    }
}

typedef struct fsRayCastResult {
    fsBodyIndex bodyIndex;
    fsVec2 point;
    fsVec2 normal;
    float fraction;
} fsRayCastResult;

static inline bool fsRayCast(fsWorld* w, fsVec2 start, fsVec2 dir, float maxFraction, uint32_t mask, fsRayCastResult* out) {
    const float dirLen = fsLength(dir);
    if (dirLen <= FLT_EPSILON) {
        return false;
    }
    float minFraction = maxFraction;
    bool hit = false;
    
    for (uint16_t i = 0; i < MAX_BODIES; i++) {
        if (!w->isActive[i]) continue;
        if (!(w->categoryBits[i] & mask)) continue;
        
        // Simple ray-circle or ray-box
        if (w->shapes[i].type == FS_CIRCLE) {
            fsVec2 m = fsSub(start, w->positions[i]);
            float c = fsDot(m, m) - w->shapes[i].circle.radius * w->shapes[i].circle.radius;
            float b_dot = fsDot(m, dir);
            if (c > 0.0f && b_dot > 0.0f) continue;
            float discr = b_dot * b_dot - c;
            if (discr < 0.0f) continue;
            float t = -b_dot - sqrtf(discr);
            if (t < 0.0f) t = 0.0f;
            float f = t / dirLen;
            if (f < minFraction) {
                minFraction = f;
                out->bodyIndex = i;
                out->point = fsAdd(start, fsMul(dir, f));
                out->normal = fsNormalize(fsSub(out->point, w->positions[i]));
                out->fraction = f;
                hit = true;
            }
        }
        // TODO: Ray-Box if needed, but mostly walls are circles/boxes.
        // impulse wars uses AABBs for walls mostly.
    }
    return hit;
}

#endif
