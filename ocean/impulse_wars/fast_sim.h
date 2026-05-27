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

typedef struct fsBody {
    fsVec2 pos;
    fsVec2 vel;
    float angle;
    float angularVel;
    fsVec2 force;
    float torque;
    float mass;
    float invMass;
    float inertia;
    float invInertia;
    float friction;
    float restitution;
    fsShape shape;
    void* userData;
    bool isActive;
    bool isStatic;
    bool isSensor;
    uint32_t categoryBits;
    uint32_t maskBits;
} fsBody;

static inline void fsWorld_DestroyBody(fsBody* b) {
    if (!b) return;
    b->isActive = false;
    b->categoryBits = 0;
    b->maskBits = 0;
}

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

// Integration
static inline void fsBody_Integrate(fsBody* b, float dt) {
    if (b->isStatic) return;
    
    // Euler integration
    fsVec2 acceleration = fsMul(b->force, b->invMass);
    b->vel = fsAdd(b->vel, fsMul(acceleration, dt));
    b->pos = fsAdd(b->pos, fsMul(b->vel, dt));
    
    float angularAccel = b->torque * b->invInertia;
    b->angularVel += angularAccel * dt;
    b->angle += b->angularVel * dt;
    
    // Reset forces
    b->force = (fsVec2){0, 0};
    b->torque = 0;
}

// Collision Resolution Primitives
typedef struct fsContact {
    fsVec2 normal;
    float depth;
    fsVec2 point;
    fsBody* a;
    fsBody* b;
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

// Simple impulse resolution
static inline void fsResolveCollision(fsContact* c) {
    if (c->a->isStatic && c->b->isStatic) return;
    if (c->a->isSensor || c->b->isSensor) return;
    
    fsBody* a = c->a;
    fsBody* b = c->b;
    fsVec2 n = c->normal;
    
    // Relative velocity
    fsVec2 rv = fsSub(b->vel, a->vel);
    
    // Relative velocity along normal
    float velAlongNormal = fsDot(rv, n);
    
    // Do not resolve if velocities are separating
    if (velAlongNormal > 0) return;
    
    // Restitution
    float e = fminf(a->restitution, b->restitution);
    
    // Impulse magnitude
    float j = -(1.0f + e) * velAlongNormal;
    j /= a->invMass + b->invMass;
    
    // Apply impulse
    fsVec2 impulse = fsMul(n, j);
    if (!a->isStatic) a->vel = fsSub(a->vel, fsMul(impulse, a->invMass));
    if (!b->isStatic) b->vel = fsAdd(b->vel, fsMul(impulse, b->invMass));
    
    // Positional correction (anti-sink)
    const float percent = 0.2f; // penetration percentage to correct
    const float slop = 0.01f; // penetration allowance
    fsVec2 correction = fsMul(n, fmaxf(c->depth - slop, 0.0f) / (a->invMass + b->invMass) * percent);
    if (!a->isStatic) a->pos = fsSub(a->pos, fsMul(correction, a->invMass));
    if (!b->isStatic) b->pos = fsAdd(b->pos, fsMul(correction, b->invMass));
}

#define MAX_BODIES 1024
typedef enum fsContactType {
    FS_CONTACT_BEGIN,
    FS_CONTACT_END
} fsContactType;

typedef struct fsContactEvent {
    fsContactType type;
    fsBody *a;
    fsBody *b;
    fsVec2 normal;
} fsContactEvent;

#define MAX_EVENTS 2048
typedef struct fsWorld {
    fsBody bodies[MAX_BODIES];
    fsVec2 gravity;
    fsContactEvent events[MAX_EVENTS];
    uint16_t numEvents;
} fsWorld;

static inline void fsWorld_Init(fsWorld* w, fsVec2 gravity) {
    memset(w, 0, sizeof(fsWorld));
    w->gravity = gravity;
}

static inline fsBody* fsWorld_CreateBody(fsWorld* w) {
    for (uint16_t i = 0; i < MAX_BODIES; i++) {
        if (!w->bodies[i].isActive) {
            fsBody* b = &w->bodies[i];
            memset(b, 0, sizeof(fsBody));
            b->isActive = true;
            b->invMass = 1.0f;
            b->restitution = 1.0f;
            return b;
        }
    }
    return NULL;
}

static inline void fsWorld_Step(fsWorld* w, float dt) {
    // 1. Integration
    for (uint16_t i = 0; i < MAX_BODIES; i++) {
        if (w->bodies[i].isActive) fsBody_Integrate(&w->bodies[i], dt);
    }

    // 2. Collision detection & resolution
    for (uint16_t i = 0; i < MAX_BODIES; i++) {
        if (!w->bodies[i].isActive) continue;
        for (uint16_t j = i + 1; j < MAX_BODIES; j++) {
            if (!w->bodies[j].isActive) continue;
            fsBody* a = &w->bodies[i];
            fsBody* b = &w->bodies[j];
            
            if (a->isStatic && b->isStatic) continue;
            if (!(a->categoryBits & b->maskBits) && !(b->categoryBits & a->maskBits)) continue;

            fsContact contact;
            bool hit = false;
            
            if (a->shape.type == FS_CIRCLE && b->shape.type == FS_CIRCLE) {
                hit = fsTestCircleCircle(a->pos, a->shape.circle.radius, b->pos, b->shape.circle.radius, &contact);
            } else if (a->shape.type == FS_CIRCLE && b->shape.type == FS_BOX) {
                hit = fsTestCircleBox(a->pos, a->shape.circle.radius, b->pos, b->shape.box.halfExtents, fsMakeRot(b->angle), &contact);
            } else if (a->shape.type == FS_BOX && b->shape.type == FS_CIRCLE) {
                hit = fsTestCircleBox(b->pos, b->shape.circle.radius, a->pos, a->shape.box.halfExtents, fsMakeRot(a->angle), &contact);
                // Flip normal
                contact.normal = fsMul(contact.normal, -1.0f);
            }
            
            if (hit) {
                contact.a = a;
                contact.b = b;
                if (!a->isSensor && !b->isSensor) {
                    fsResolveCollision(&contact);
                }
                
                if (w->numEvents < MAX_EVENTS) {
                    w->events[w->numEvents++] = (fsContactEvent){
                        .type = FS_CONTACT_BEGIN,
                        .a = a,
                        .b = b,
                        .normal = contact.normal
                    };
                }
            }
        }
    }
}

typedef struct fsRayCastResult {
    fsBody* body;
    fsVec2 point;
    fsVec2 normal;
    float fraction;
} fsRayCastResult;

static inline bool fsRayCast(fsWorld* w, fsVec2 start, fsVec2 dir, float maxFraction, uint32_t mask, fsRayCastResult* out) {
    float minFraction = maxFraction;
    bool hit = false;
    
    for (uint16_t i = 0; i < MAX_BODIES; i++) {
        fsBody* b = &w->bodies[i];
        if (!b->isActive) continue;
        if (!(b->categoryBits & mask)) continue;
        
        // Simple ray-circle or ray-box
        if (b->shape.type == FS_CIRCLE) {
            fsVec2 m = fsSub(start, b->pos);
            float c = fsDot(m, m) - b->shape.circle.radius * b->shape.circle.radius;
            float b_dot = fsDot(m, dir);
            if (c > 0.0f && b_dot > 0.0f) continue;
            float discr = b_dot * b_dot - c;
            if (discr < 0.0f) continue;
            float t = -b_dot - sqrtf(discr);
            if (t < 0.0f) t = 0.0f;
            float f = t / fsLength(dir);
            if (f < minFraction) {
                minFraction = f;
                out->body = b;
                out->point = fsAdd(start, fsMul(dir, f));
                out->normal = fsNormalize(fsSub(out->point, b->pos));
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
