#ifndef IMPULSE_WARS_TYPES_H
#define IMPULSE_WARS_TYPES_H

#include "fast_sim.h"
#include "fs_compat.h"
#include "raylib.h"
#include "rlights.h"

#include "include/cc_array.h"

// Define constants needed for SoA arrays to avoid circular dependency
#define MAX_WEAPON_PICKUPS 12
#define MAX_FLOATING_WALLS 18

// Body index type (used instead of pointer for SoA layout)
// Must be defined here since types.h is included by fast_sim.h consumers
typedef uint16_t fsBodyIndex;
#define FS_BODY_INVALID 0xFFFF

#define _MAX_DRONES 4

const uint8_t NUM_WALL_TYPES = 3;

#define MAX_TRAIL_POINTS 20
#define MAX_DRONE_TRAIL_POINTS 20
#define MAX_PROJECTLE_TRAIL_POINTS 10

enum entityType {
    STANDARD_WALL_ENTITY,
    BOUNCY_WALL_ENTITY,
    DEATH_WALL_ENTITY,
    WEAPON_PICKUP_ENTITY,
    PROJECTILE_ENTITY,
    DRONE_ENTITY,
    SHIELD_ENTITY,
    DRONE_PIECE_ENTITY,
};

// the category bit that will be set on each entity's shape; this is
// used to control what entities can collide with each other
enum shapeCategory {
    WALL_SHAPE = 1,
    FLOATING_WALL_SHAPE = 2,
    PROJECTILE_SHAPE = 4,
    WEAPON_PICKUP_SHAPE = 8,
    DRONE_SHAPE = 16,
    SHIELD_SHAPE = 32,
    DRONE_PIECE_SHAPE = 64,
};

// Simplified entity ID - just the array index
typedef int32_t entityID;

// general purpose entity object
typedef struct entity {
    entityID id;
    enum entityType type;
    void *entity;
    uint16_t soaIndex;  // Index in SoA array for fast removal
} entity;

#define _NUM_WEAPONS 10
const uint8_t NUM_WEAPONS = _NUM_WEAPONS;

enum weaponType {
    STANDARD_WEAPON,
    MACHINEGUN_WEAPON,
    SNIPER_WEAPON,
    SHOTGUN_WEAPON,
    IMPLODER_WEAPON,
    ACCELERATOR_WEAPON,
    FLAK_CANNON_WEAPON,
    MINE_LAUNCHER_WEAPON,
    BLACK_HOLE_WEAPON,
    NUKE_WEAPON,
};

typedef struct mapBounds {
    fsVec2 min;
    fsVec2 max;
} mapBounds;

// used for N near entities observations
typedef struct nearEntity {
    uint16_t idx;
    void *entity;
    float distanceSquared;
} nearEntity;

typedef struct mapEntry {
    const char *layout;
    const uint8_t columns;
    const uint8_t rows;
    const uint8_t randFloatingStandardWalls;
    const uint8_t randFloatingBouncyWalls;
    const uint8_t randFloatingDeathWalls;
    // are there any floating walls that have consistent starting positions
    const bool hasSetFloatingWalls;
    const uint16_t weaponPickups;
    const enum weaponType defaultWeapon;
    const uint8_t maxSuddenDeathWalls;

    mapBounds bounds;
    mapBounds spawnQuads[4];
    bool *droneSpawns;
    uint8_t *packedLayout;
    nearEntity *nearestWalls;
} mapEntry;

// a cell in the map; ent will be NULL if the cell is empty
typedef struct mapCell {
    entity *ent;
    fsVec2 pos;
} mapCell;

typedef struct wallEntity {
    fsBodyIndex body;
    fsVec2 pos;
    fsRot rot;
    fsVec2 velocity;
    fsVec2 extent;
    int16_t mapCellIdx;
    bool isFloating;
    enum entityType type;
    bool isSuddenDeath;
    fsVec2 contributions[_MAX_DRONES];

    entity *ent;
} wallEntity;

typedef struct weaponInformation {
    const enum weaponType type;
    // should the body be treated as a bullet by box2d; if so CCD
    // (continuous collision detection) will be enabled to prevent
    // tunneling through static bodies which is expensive so it's
    // only enabled for fast moving projectiles
    const bool isPhysicsBullet;
    // can the projectile ever be stationary? if so, it should be
    // allowed to sleep to save on physics updates
    const bool canSleep;
    const uint8_t numProjectiles;
    const float fireMagnitude;
    const float recoilMagnitude;
    const float damping;
    const float charge;
    const float coolDown;
    const float maxDistance;
    const float radius;
    const float density;
    const float mass;
    const float invMass;
    const float initialSpeed;
    const uint8_t maxBounces;
    const bool explosive;
    const bool destroyedOnDroneHit;
    const bool explodesOnDroneHit;
    const bool hasSensor;
    const float energyRefillCoef;
    const float spawnWeight;
} weaponInformation;

typedef struct weaponPickupEntity {
    fsBodyIndex body;
    enum weaponType weapon;
    float respawnWait;
    // how many floating walls are touching this pickup
    uint8_t floatingWallsTouching;
    fsVec2 pos;
    int16_t mapCellIdx;

    entity *ent;
    bool bodyDestroyed;
} weaponPickupEntity;

typedef struct droneEntity droneEntity;

typedef struct trailPoints {
    Vector2 points[MAX_TRAIL_POINTS];
    uint8_t length;
} trailPoints;

typedef struct projectileEntity {
    uint8_t droneIdx;

    fsBodyIndex body;
    fsBodyIndex sensor;
    weaponInformation *weaponInfo;
    fsVec2 pos;
    int16_t mapCellIdx;
    fsVec2 lastPos;
    fsVec2 velocity;
    fsVec2 lastVelocity;
    float speed;
    float lastSpeed;
    float distance;
    uint8_t bounces;
    uint8_t contacts;
    bool setMine;
    uint8_t numDronesBehindWalls;
    uint8_t dronesBehindWalls[_MAX_DRONES];
    CC_Array *entsInBlackHole;
    bool needsToBeDestroyed;

    entity *ent;

    // for rendering
    trailPoints trailPoints;
} projectileEntity;

// used to keep track of what happened each step for reward purposes
typedef struct droneStepInfo {
    bool firedShot;
    bool pickedUpWeapon;
    enum weaponType prevWeapon;
    float shotHit[_MAX_DRONES];
    float explosionHit[_MAX_DRONES];
    float shotTaken[_MAX_DRONES];
    float explosionTaken[_MAX_DRONES];
    bool brokeShield[_MAX_DRONES];
    bool ownShotTaken;
} droneStepInfo;

typedef struct shieldEntity {
    droneEntity *drone;

    fsBodyIndex body;
    fsVec2 pos;
    float health;
    float duration;

    entity *ent;
} shieldEntity;

typedef struct dronePieceEntity {
    uint8_t droneIdx;

    fsBodyIndex body;
    fsVec2 pos;
    fsRot rot;
    fsVec2 vertices[3];
    bool isShieldPiece;

    entity *ent;

    uint16_t lifetime;
} dronePieceEntity;



typedef struct droneEntity {
    fsBodyIndex body;
    weaponInformation *weaponInfo;
    int8_t ammo;
    float weaponCooldown;
    uint16_t heat;
    bool chargingWeapon;
    float weaponCharge;
    float energyLeft;
    bool braking;
    bool chargingBurst;
    float burstCharge;
    float burstCooldown;
    bool energyFullyDepleted;
    bool energyFullyDepletedThisStep;
    float energyRefillWait;
    bool shotThisStep;
    bool diedThisStep;

    uint8_t idx;
    uint8_t team;
    fsVec2 initalPos;
    fsVec2 pos;
    int16_t mapCellIdx;
    fsVec2 lastPos;
    fsVec2 lastMove;
    fsVec2 lastAim;
    fsVec2 velocity;
    fsVec2 lastVelocity;
    droneStepInfo stepInfo;
    float respawnWait;
    uint8_t livesLeft;
    bool dead;

    fsVec2 contributions[_MAX_DRONES];
    int8_t killedBy;
    bool killed[_MAX_DRONES];

    shieldEntity *shield;
    entity *ent;

    // for rendering
    trailPoints trailPoints;
    CC_Array *brakeTrailPoints;
    uint16_t respawnGuideLifetime;
} droneEntity;

// stats for the whole episode
typedef struct droneStats {
    float returns;
    float distanceTraveled;
    float absDistanceTraveled;
    float brakeTime;
    float totalBursts;
    float burstsHit;
    float energyEmptied;
    float shieldsBroken;
    float ownShieldBroken;
    float selfKills;
    float kills;
    float unknownKills;
    float wins;

    float shotsFired[_NUM_WEAPONS];
    float shotsHit[_NUM_WEAPONS];
    float shotsTaken[_NUM_WEAPONS];
    float ownShotsTaken[_NUM_WEAPONS];
    float weaponsPickedUp[_NUM_WEAPONS];
    float shotDistances[_NUM_WEAPONS];

    float totalShotsFired;
    float totalShotsHit;
    float totalShotsTaken;
    float totalOwnShotsTaken;
    float totalWeaponsPickedUp;
    float totalShotDistances;
} droneStats;

typedef struct Log {
    float length;
    float ties;
    droneStats stats[_MAX_DRONES];

    float n;
} Log;

typedef struct gameCamera {
    Camera3D camera3D;
    Camera2D camera2D;
    Vector2 targetPos;
    float maxZoom;
    bool orthographic;
} gameCamera;

typedef struct rayClient {
    float scale;
    uint16_t width;
    uint16_t height;
    uint16_t halfWidth;
    uint16_t halfHeight;

    gameCamera *camera;

    Shader blurShader;
    int32_t blurShaderDirLoc;
    Shader bloomShader;
    int32_t bloomIntensityLoc;
    int32_t bloomTexColorLoc;
    int32_t bloomTexBloomBlurLoc;
    Shader gridShader;
    int32_t gridShaderPosLoc[4];
    int32_t gridShaderColorLoc[4];
    Texture2D wallTexture;
    RenderTexture2D blurSrcTexture;
    RenderTexture2D blurDstTexture;
    RenderTexture2D projRawTex;
    RenderTexture2D projBloomTex;
    RenderTexture2D droneRawTex;
    RenderTexture2D droneBloomTex;
} rayClient;

typedef struct brakeTrailPoint {
    fsVec2 pos;
    uint16_t lifetime;
    bool isEnd;
} brakeTrailPoint;

typedef struct explosionInfo {
    fsVec2 pos;
    float radius;
    float impulsePerLength;
    bool isBurst;
    uint8_t droneIdx;
    uint16_t renderSteps;
} explosionInfo;

typedef struct agentActions {
    fsVec2 move;
    fsVec2 aim;
    bool chargingWeapon;
    bool shoot;
    bool brake;
    bool chargingBurst;
    bool discardWeapon;
} agentActions;

typedef struct pathingInfo {
    uint8_t *paths;
    int8_t *pathBuffer;
} pathingInfo;

typedef struct debugPoint {
    fsVec2 pos;
    float size;
    Color color;
} debugPoint;

// True Structure of Arrays (SoA) for maximum performance
// Elements are now stored contiguously in value arrays, not pointers.

typedef struct {
    fsBodyIndex bodies[_MAX_DRONES];
    weaponInformation *weaponInfos[_MAX_DRONES];
    int8_t ammos[_MAX_DRONES];
    float weaponCooldowns[_MAX_DRONES];
    uint16_t heats[_MAX_DRONES];
    bool chargingWeapons[_MAX_DRONES];
    float weaponCharges[_MAX_DRONES];
    float energyLefts[_MAX_DRONES];
    bool brakings[_MAX_DRONES];
    bool chargingBursts[_MAX_DRONES];
    float burstCharges[_MAX_DRONES];
    float burstCooldowns[_MAX_DRONES];
    bool energyFullyDepleteds[_MAX_DRONES];
    bool energyFullyDepletedThisSteps[_MAX_DRONES];
    float energyRefillWaits[_MAX_DRONES];
    bool shotThisSteps[_MAX_DRONES];
    bool diedThisSteps[_MAX_DRONES];

    uint8_t idxs[_MAX_DRONES];
    uint8_t teams[_MAX_DRONES];
    fsVec2 initalPoses[_MAX_DRONES];
    fsVec2 poses[_MAX_DRONES];
    int16_t mapCellIdxs[_MAX_DRONES];
    fsVec2 lastPoses[_MAX_DRONES];
    fsVec2 lastMoves[_MAX_DRONES];
    fsVec2 lastAims[_MAX_DRONES];
    fsVec2 velocities[_MAX_DRONES];
    fsVec2 lastVelocities[_MAX_DRONES];
    droneStepInfo stepInfos[_MAX_DRONES];
    float respawnWaits[_MAX_DRONES];
    uint8_t livesLefts[_MAX_DRONES];
    bool deads[_MAX_DRONES];

    fsVec2 contributions[_MAX_DRONES][_MAX_DRONES];
    int8_t killedBys[_MAX_DRONES];
    bool killeds[_MAX_DRONES][_MAX_DRONES];

    shieldEntity *shields[_MAX_DRONES];
    entityID entityIDs[_MAX_DRONES];
    
    trailPoints trailPoints[_MAX_DRONES];
    CC_Array *brakeTrailPoints[_MAX_DRONES];
    uint16_t respawnGuideLifetimes[_MAX_DRONES];
    
    // Back-pointers for compatibility during bridging
    droneEntity* entities[_MAX_DRONES];
    
    uint8_t size;
} DroneSoA;

typedef struct {
    uint8_t droneIdxs[128];
    fsBodyIndex bodies[128];
    fsBodyIndex sensors[128];
    weaponInformation *weaponInfos[128];
    fsVec2 poses[128];
    int16_t mapCellIdxs[128];
    fsVec2 lastPoses[128];
    fsVec2 velocities[128];
    fsVec2 lastVelocities[128];
    float speeds[128];
    float lastSpeeds[128];
    float distances[128];
    uint8_t bounces[128];
    uint8_t contacts[128];
    bool setMines[128];
    uint8_t numDronesBehindWalls[128];
    uint8_t dronesBehindWalls[128][_MAX_DRONES];
    CC_Array *entsInBlackHoles[128];
    bool needsToBeDestroyeds[128];
    entityID entityIDs[128];
    trailPoints trailPoints[128];
    
    projectileEntity* entities[128];
    uint8_t size;
} ProjectileSoA;

typedef struct {
    fsBodyIndex bodies[256];
    fsVec2 poses[256];
    fsRot rots[256];
    fsVec2 velocities[256];
    fsVec2 extents[256];
    int16_t mapCellIdxs[256];
    bool isFloatings[256];
    enum entityType types[256];
    bool isSuddenDeaths[256];
    fsVec2 contributions[256][_MAX_DRONES];
    entityID entityIDs[256];

    wallEntity* entities[256];
    uint16_t size;
} WallSoA;

typedef struct {
    fsBodyIndex bodies[MAX_WEAPON_PICKUPS];
    enum weaponType weapons[MAX_WEAPON_PICKUPS];
    float respawnWaits[MAX_WEAPON_PICKUPS];
    uint8_t floatingWallsTouchings[MAX_WEAPON_PICKUPS];
    fsVec2 poses[MAX_WEAPON_PICKUPS];
    int16_t mapCellIdxs[MAX_WEAPON_PICKUPS];
    entityID entityIDs[MAX_WEAPON_PICKUPS];
    bool bodyDestroyeds[MAX_WEAPON_PICKUPS];

    weaponPickupEntity* entities[MAX_WEAPON_PICKUPS];
    uint8_t size;
} PickupSoA;

typedef struct {
    fsVec2 poses[32];
    float radiuses[32];
    float impulsePerLengths[32];
    bool isBursts[32];
    uint8_t droneIdxs[32];
    uint16_t renderSteps[32];

    explosionInfo* entities[32];
    uint8_t size;
} ExplosionSoA;

typedef struct {
    uint8_t droneIdxs[32];
    fsBodyIndex bodies[32];
    fsVec2 poses[32];
    fsRot rots[32];
    fsVec2 vertices[32][3];
    bool isShieldPieces[32];
    entityID entityIDs[32];
    uint16_t lifetimes[32];

    dronePieceEntity* entities[32];
    uint8_t size;
} DronePieceSoA;

typedef struct {
    entityID entityIDs[512];
    fsVec2 poses[512];
    
    mapCell* entities[512];
    uint16_t size;
} CellSoA;

// Size check to ensure we don't overflow the fixed buffers
static inline uint8_t drone_soa_size(DroneSoA *a) { return a->size; }
static inline uint8_t projectile_soa_size(ProjectileSoA *a) { return a->size; }
static inline uint16_t wall_soa_size(WallSoA *a) { return a->size; }
static inline uint8_t pickup_soa_size(PickupSoA *a) { return a->size; }
static inline uint8_t explosion_soa_size(ExplosionSoA *a) { return a->size; }
static inline uint8_t drone_piece_soa_size(DronePieceSoA *a) { return a->size; }
static inline uint16_t cell_soa_size(CellSoA *a) { return a->size; }

// --- SOA Helper Functions ---
// These helpers manage the contiguous value arrays in the SoA structs.
// Removal uses a swap-with-last strategy for O(1) performance.

static inline droneEntity* drone_soa_get(DroneSoA *a, uint8_t idx) {
    if (idx >= a->size) return NULL;
    return a->entities[idx];
}

static inline uint8_t drone_soa_add(DroneSoA *a, droneEntity *d) {
    if (a->size >= _MAX_DRONES) return 0xFF;
    uint8_t idx = a->size++;
    a->entities[idx] = d;
    a->bodies[idx] = d->body;
    a->weaponInfos[idx] = d->weaponInfo;
    a->ammos[idx] = d->ammo;
    a->weaponCooldowns[idx] = d->weaponCooldown;
    a->heats[idx] = d->heat;
    a->chargingWeapons[idx] = d->chargingWeapon;
    a->weaponCharges[idx] = d->weaponCharge;
    a->energyLefts[idx] = d->energyLeft;
    a->brakings[idx] = d->braking;
    a->chargingBursts[idx] = d->chargingBurst;
    a->burstCharges[idx] = d->burstCharge;
    a->burstCooldowns[idx] = d->burstCooldown;
    a->energyFullyDepleteds[idx] = d->energyFullyDepleted;
    a->energyFullyDepletedThisSteps[idx] = d->energyFullyDepletedThisStep;
    a->energyRefillWaits[idx] = d->energyRefillWait;
    a->shotThisSteps[idx] = d->shotThisStep;
    a->diedThisSteps[idx] = d->diedThisStep;
    a->idxs[idx] = d->idx;
    a->teams[idx] = d->team;
    a->initalPoses[idx] = d->initalPos;
    a->poses[idx] = d->pos;
    a->mapCellIdxs[idx] = d->mapCellIdx;
    a->lastPoses[idx] = d->lastPos;
    a->lastMoves[idx] = d->lastMove;
    a->lastAims[idx] = d->lastAim;
    a->velocities[idx] = d->velocity;
    a->lastVelocities[idx] = d->lastVelocity;
    a->stepInfos[idx] = d->stepInfo;
    a->respawnWaits[idx] = d->respawnWait;
    a->livesLefts[idx] = d->livesLeft;
    a->deads[idx] = d->dead;
    a->killedBys[idx] = d->killedBy;
    for (int i=0; i<_MAX_DRONES; ++i) {
        a->contributions[idx][i] = d->contributions[i];
        a->killeds[idx][i] = d->killed[i];
    }
    a->shields[idx] = d->shield;
    a->entityIDs[idx] = d->ent->id;
    a->trailPoints[idx] = d->trailPoints;
    a->brakeTrailPoints[idx] = d->brakeTrailPoints;
    a->respawnGuideLifetimes[idx] = d->respawnGuideLifetime;
    return idx;
}

static inline void drone_soa_remove(DroneSoA *a, uint8_t idx) {
    if (idx >= a->size) return;
    a->size--;
    if (idx < a->size) {
        uint8_t last = a->size;
        a->entities[idx] = a->entities[last];
        a->entities[idx]->ent->soaIndex = idx;
        a->bodies[idx] = a->bodies[last];
        a->weaponInfos[idx] = a->weaponInfos[last];
        a->ammos[idx] = a->ammos[last];
        a->weaponCooldowns[idx] = a->weaponCooldowns[last];
        a->heats[idx] = a->heats[last];
        a->chargingWeapons[idx] = a->chargingWeapons[last];
        a->weaponCharges[idx] = a->weaponCharges[last];
        a->energyLefts[idx] = a->energyLefts[last];
        a->brakings[idx] = a->brakings[last];
        a->chargingBursts[idx] = a->chargingBursts[last];
        a->burstCharges[idx] = a->burstCharges[last];
        a->burstCooldowns[idx] = a->burstCooldowns[last];
        a->energyFullyDepleteds[idx] = a->energyFullyDepleteds[last];
        a->energyFullyDepletedThisSteps[idx] = a->energyFullyDepletedThisSteps[last];
        a->energyRefillWaits[idx] = a->energyRefillWaits[last];
        a->shotThisSteps[idx] = a->shotThisSteps[last];
        a->diedThisSteps[idx] = a->diedThisSteps[last];
        a->idxs[idx] = a->idxs[last];
        a->teams[idx] = a->teams[last];
        a->initalPoses[idx] = a->initalPoses[last];
        a->poses[idx] = a->poses[last];
        a->mapCellIdxs[idx] = a->mapCellIdxs[last];
        a->lastPoses[idx] = a->lastPoses[last];
        a->lastMoves[idx] = a->lastMoves[last];
        a->lastAims[idx] = a->lastAims[last];
        a->velocities[idx] = a->velocities[last];
        a->lastVelocities[idx] = a->lastVelocities[last];
        a->stepInfos[idx] = a->stepInfos[last];
        a->respawnWaits[idx] = a->respawnWaits[last];
        a->livesLefts[idx] = a->livesLefts[last];
        a->deads[idx] = a->deads[last];
        a->killedBys[idx] = a->killedBys[last];
        for (int i=0; i<_MAX_DRONES; ++i) {
            a->contributions[idx][i] = a->contributions[last][i];
            a->killeds[idx][i] = a->killeds[last][i];
        }
        a->shields[idx] = a->shields[last];
        a->entityIDs[idx] = a->entityIDs[last];
        a->trailPoints[idx] = a->trailPoints[last];
        a->brakeTrailPoints[idx] = a->brakeTrailPoints[last];
        a->respawnGuideLifetimes[idx] = a->respawnGuideLifetimes[last];
    }
}

static inline projectileEntity* projectile_soa_get(ProjectileSoA *a, uint8_t idx) {
    if (idx >= a->size) return NULL;
    return a->entities[idx];
}

static inline uint8_t projectile_soa_add(ProjectileSoA *a, projectileEntity *item) {
    if (a->size >= 128) return 0xFF;
    uint8_t idx = a->size++;
    a->entities[idx] = item;
    a->droneIdxs[idx] = item->droneIdx;
    a->bodies[idx] = item->body;
    a->sensors[idx] = item->sensor;
    a->weaponInfos[idx] = item->weaponInfo;
    a->poses[idx] = item->pos;
    a->mapCellIdxs[idx] = item->mapCellIdx;
    a->lastPoses[idx] = item->lastPos;
    a->velocities[idx] = item->velocity;
    a->lastVelocities[idx] = item->lastVelocity;
    a->speeds[idx] = item->speed;
    a->lastSpeeds[idx] = item->lastSpeed;
    a->distances[idx] = item->distance;
    a->bounces[idx] = item->bounces;
    a->contacts[idx] = item->contacts;
    a->setMines[idx] = item->setMine;
    a->numDronesBehindWalls[idx] = item->numDronesBehindWalls;
    memcpy(a->dronesBehindWalls[idx], item->dronesBehindWalls, _MAX_DRONES);
    a->entsInBlackHoles[idx] = item->entsInBlackHole;
    a->needsToBeDestroyeds[idx] = item->needsToBeDestroyed;
    a->entityIDs[idx] = item->ent->id;
    a->trailPoints[idx] = item->trailPoints;
    return idx;
}

static inline void projectile_soa_remove(ProjectileSoA *a, uint8_t idx) {
    if (idx >= a->size) return;
    a->size--;
    if (idx < a->size) {
        uint8_t last = a->size;
        a->entities[idx] = a->entities[last];
        a->entities[idx]->ent->soaIndex = idx;
        a->droneIdxs[idx] = a->droneIdxs[last];
        a->bodies[idx] = a->bodies[last];
        a->sensors[idx] = a->sensors[last];
        a->weaponInfos[idx] = a->weaponInfos[last];
        a->poses[idx] = a->poses[last];
        a->mapCellIdxs[idx] = a->mapCellIdxs[last];
        a->lastPoses[idx] = a->lastPoses[last];
        a->velocities[idx] = a->velocities[last];
        a->lastVelocities[idx] = a->lastVelocities[last];
        a->speeds[idx] = a->speeds[last];
        a->lastSpeeds[idx] = a->lastSpeeds[last];
        a->distances[idx] = a->distances[last];
        a->bounces[idx] = a->bounces[last];
        a->contacts[idx] = a->contacts[last];
        a->setMines[idx] = a->setMines[last];
        a->numDronesBehindWalls[idx] = a->numDronesBehindWalls[last];
        memcpy(a->dronesBehindWalls[idx], a->dronesBehindWalls[last], _MAX_DRONES);
        a->entsInBlackHoles[idx] = a->entsInBlackHoles[last];
        a->needsToBeDestroyeds[idx] = a->needsToBeDestroyeds[last];
        a->entityIDs[idx] = a->entityIDs[last];
        a->trailPoints[idx] = a->trailPoints[last];
    }
}

static inline wallEntity* wall_soa_get(WallSoA *a, uint16_t idx) {
    if (idx >= a->size) return NULL;
    return a->entities[idx];
}

static inline uint16_t wall_soa_add(WallSoA *a, wallEntity *item) {
    if (a->size >= 256) return 0xFFFF;
    uint16_t idx = a->size++;
    a->entities[idx] = item;
    a->bodies[idx] = item->body;
    a->poses[idx] = item->pos;
    a->rots[idx] = item->rot;
    a->velocities[idx] = item->velocity;
    a->extents[idx] = item->extent;
    a->mapCellIdxs[idx] = item->mapCellIdx;
    a->isFloatings[idx] = item->isFloating;
    a->types[idx] = item->type;
    a->isSuddenDeaths[idx] = item->isSuddenDeath;
    memcpy(a->contributions[idx], item->contributions, _MAX_DRONES * sizeof(fsVec2));
    a->entityIDs[idx] = item->ent->id;
    return idx;
}

static inline void wall_soa_remove(WallSoA *a, uint16_t idx) {
    if (idx >= a->size) return;
    a->size--;
    if (idx < a->size) {
        uint16_t last = a->size;
        a->entities[idx] = a->entities[last];
        a->entities[idx]->ent->soaIndex = idx;
        a->bodies[idx] = a->bodies[last];
        a->poses[idx] = a->poses[last];
        a->rots[idx] = a->rots[last];
        a->velocities[idx] = a->velocities[last];
        a->extents[idx] = a->extents[last];
        a->mapCellIdxs[idx] = a->mapCellIdxs[last];
        a->isFloatings[idx] = a->isFloatings[last];
        a->types[idx] = a->types[last];
        a->isSuddenDeaths[idx] = a->isSuddenDeaths[last];
        memcpy(a->contributions[idx], a->contributions[last], _MAX_DRONES * sizeof(fsVec2));
        a->entityIDs[idx] = a->entityIDs[last];
    }
}

static inline weaponPickupEntity* pickup_soa_get(PickupSoA *a, uint8_t idx) {
    if (idx >= a->size) return NULL;
    return a->entities[idx];
}

static inline uint8_t pickup_soa_add(PickupSoA *a, weaponPickupEntity *item) {
    if (a->size >= MAX_WEAPON_PICKUPS) return 0xFF;
    uint8_t idx = a->size++;
    a->entities[idx] = item;
    a->bodies[idx] = item->body;
    a->weapons[idx] = item->weapon;
    a->respawnWaits[idx] = item->respawnWait;
    a->floatingWallsTouchings[idx] = item->floatingWallsTouching;
    a->poses[idx] = item->pos;
    a->mapCellIdxs[idx] = item->mapCellIdx;
    a->entityIDs[idx] = item->ent->id;
    a->bodyDestroyeds[idx] = item->bodyDestroyed;
    return idx;
}

static inline void pickup_soa_remove(PickupSoA *a, uint8_t idx) {
    if (idx >= a->size) return;
    a->size--;
    if (idx < a->size) {
        uint8_t last = a->size;
        a->entities[idx] = a->entities[last];
        a->entities[idx]->ent->soaIndex = idx;
        a->bodies[idx] = a->bodies[last];
        a->weapons[idx] = a->weapons[last];
        a->respawnWaits[idx] = a->respawnWaits[last];
        a->floatingWallsTouchings[idx] = a->floatingWallsTouchings[last];
        a->poses[idx] = a->poses[last];
        a->mapCellIdxs[idx] = a->mapCellIdxs[last];
        a->entityIDs[idx] = a->entityIDs[last];
        a->bodyDestroyeds[idx] = a->bodyDestroyeds[last];
    }
}

static inline explosionInfo* explosion_soa_get(ExplosionSoA *a, uint8_t idx) {
    if (idx >= a->size) return NULL;
    return a->entities[idx];
}

static inline uint8_t explosion_soa_add(ExplosionSoA *a, explosionInfo *item) {
    if (a->size >= 32) return 0xFF;
    uint8_t idx = a->size++;
    a->entities[idx] = item;
    a->poses[idx] = item->pos;
    a->radiuses[idx] = item->radius;
    a->impulsePerLengths[idx] = item->impulsePerLength;
    a->isBursts[idx] = item->isBurst;
    a->droneIdxs[idx] = item->droneIdx;
    a->renderSteps[idx] = item->renderSteps;
    return idx;
}

static inline void explosion_soa_remove(ExplosionSoA *a, uint8_t idx) {
    if (idx >= a->size) return;
    a->size--;
    if (idx < a->size) {
        uint8_t last = a->size;
        a->entities[idx] = a->entities[last];
        a->poses[idx] = a->poses[last];
        a->radiuses[idx] = a->radiuses[last];
        a->impulsePerLengths[idx] = a->impulsePerLengths[last];
        a->isBursts[idx] = a->isBursts[last];
        a->droneIdxs[idx] = a->droneIdxs[last];
        a->renderSteps[idx] = a->renderSteps[last];
    }
}

static inline dronePieceEntity* drone_piece_soa_get(DronePieceSoA *a, uint8_t idx) {
    if (idx >= a->size) return NULL;
    return a->entities[idx];
}

static inline uint8_t drone_piece_soa_add(DronePieceSoA *a, dronePieceEntity *item) {
    if (a->size >= 32) return 0xFF;
    uint8_t idx = a->size++;
    a->entities[idx] = item;
    a->droneIdxs[idx] = item->droneIdx;
    a->bodies[idx] = item->body;
    a->poses[idx] = item->pos;
    a->rots[idx] = item->rot;
    memcpy(a->vertices[idx], item->vertices, 3 * sizeof(fsVec2));
    a->isShieldPieces[idx] = item->isShieldPiece;
    a->entityIDs[idx] = item->ent->id;
    a->lifetimes[idx] = item->lifetime;
    return idx;
}

static inline void drone_piece_soa_remove(DronePieceSoA *a, uint8_t idx) {
    if (idx >= a->size) return;
    a->size--;
    if (idx < a->size) {
        uint8_t last = a->size;
        a->entities[idx] = a->entities[last];
        a->entities[idx]->ent->soaIndex = idx;
        a->droneIdxs[idx] = a->droneIdxs[last];
        a->bodies[idx] = a->bodies[last];
        a->poses[idx] = a->poses[last];
        a->rots[idx] = a->rots[last];
        memcpy(a->vertices[idx], a->vertices[last], 3 * sizeof(fsVec2));
        a->isShieldPieces[idx] = a->isShieldPieces[last];
        a->entityIDs[idx] = a->entityIDs[last];
        a->lifetimes[idx] = a->lifetimes[last];
    }
}

static inline mapCell* cell_soa_get(CellSoA *a, uint16_t idx) {
    if (idx >= a->size) return NULL;
    return a->entities[idx];
}

static inline uint16_t cell_soa_add(CellSoA *a, mapCell *item) {
    if (a->size >= 512) return 0xFFFF;
    uint16_t idx = a->size++;
    a->entities[idx] = item;
    a->entityIDs[idx] = item->ent ? item->ent->id : -1;
    a->poses[idx] = item->pos;
    return idx;
}

static inline void cell_soa_remove(CellSoA *a, uint16_t idx) {
    if (idx >= a->size) return;
    a->size--;
    if (idx < a->size) {
        uint16_t last = a->size;
        a->entities[idx] = a->entities[last];
        a->entityIDs[idx] = a->entityIDs[last];
        a->poses[idx] = a->poses[last];
    }
}

static inline void drone_soa_remove_last(DroneSoA *a) { if (a->size > 0) drone_soa_remove(a, a->size - 1); }
static inline void drone_soa_remove_all(DroneSoA *a) { a->size = 0; }
static inline void projectile_soa_remove_all(ProjectileSoA *a) { a->size = 0; }
static inline void wall_soa_remove_all(WallSoA *a) { a->size = 0; }
static inline void pickup_soa_remove_all(PickupSoA *a) { a->size = 0; }
static inline void explosion_soa_remove_all(ExplosionSoA *a) { a->size = 0; }
static inline void drone_piece_soa_remove_all(DronePieceSoA *a) { a->size = 0; }
static inline void cell_soa_remove_all(CellSoA *a) { a->size = 0; }

typedef struct iwEnv {
    uint8_t numDrones;
    uint8_t numAgents;
    uint8_t numTeams;
    bool teamsEnabled;
    bool sittingDuck;
    bool isTraining;

    float winReward;
    float selfKillPunishment;
    float enemyDeathReward;
    float enemyKillReward;
    float teammateDeathPunishment;
    float teammateKillPunishment;
    float deathPunishment;
    float energyEmptiedPunishment;
    float weaponPickupReward;
    float shieldBreakReward;
    float shotHitRewardCoef;
    float explosionHitRewardCoef;

    uint16_t obsBytes;
    uint16_t discreteObsBytes;
    bool continuousActions;
    bool obsDirty;
    uint8_t obsDirtyPerAgent[_MAX_DRONES]; // Per-agent observation dirty flags

    uint8_t *observations;
    float *rewards;
    float *actions;
    uint8_t *masks;
    float *terminals;
    uint8_t *truncations;

    uint8_t frameRate;
    float deltaTime;
    uint8_t frameSkip;
    uint8_t physicsSubSteps;
    uint64_t randState;
    bool needsReset;

    uint16_t episodeLength;
    Log log;
    droneStats stats[_MAX_DRONES];

    fsWorld world;
    int8_t pinnedMapIdx;
    int8_t mapIdx;
    mapEntry *map;
    int8_t lastSpawnQuad;

    // Per-environment map data (computed during setupEnv)
    bool *droneSpawns;
    uint8_t *packedLayout;
    nearEntity *nearestWalls;
    uint8_t spawnedWeaponPickups[_NUM_WEAPONS];
    weaponInformation *defaultWeapon;
    CC_Array *entities;
    CellSoA cells;
    WallSoA walls;
    WallSoA floatingWalls;
    DroneSoA drones;
    PickupSoA pickups;
    ProjectileSoA projectiles;
    ProjectileSoA explodingProjectiles;
    DronePieceSoA dronePieces;

    pathingInfo *mapPathing;

    uint16_t totalSteps;
    uint16_t totalSuddenDeathSteps;
    // steps left until sudden death
    uint16_t stepsLeft;
    // steps left until the next set of sudden death walls are spawned
    uint16_t suddenDeathSteps;
    // the amount of sudden death walls that have been spawned
    uint8_t suddenDeathWallCounter;
    bool suddenDeathWallsPlaced;

    bool humanInput;
    uint8_t humanDroneInput;
    uint8_t connectedControllers;

    // used for rendering
    rayClient *client;
    float renderScale;
    ExplosionSoA explosions;
    CC_Array *projectilePool;
    CC_Array *dronePiecePool;
    CC_Array *explosionPool;
    CC_Array *wallPool;
    CC_Array *pickupPool;
    CC_Array *dronePool;
    CC_Array *debugPoints;

    // Distance cache for drone-to-drone distances (max 4x4 = 16 pairs)
    float droneDistanceCache[_MAX_DRONES][_MAX_DRONES];
    uint32_t droneDistanceTimestamp[_MAX_DRONES][_MAX_DRONES];
    uint32_t currentDistanceTimestamp;

    // Pre-calculated map coordinate transforms
    float mapOriginX, mapOriginY;
    float invWallThickness;
} iwEnv;

// Initialize SoA arrays (zero allocation, just zero active flags)
static inline void init_soa_arrays(iwEnv *e) {
    memset(&e->cells, 0, sizeof(CellSoA));
    memset(&e->walls, 0, sizeof(WallSoA));
    memset(&e->floatingWalls, 0, sizeof(WallSoA));
    memset(&e->drones, 0, sizeof(DroneSoA));
    memset(&e->pickups, 0, sizeof(PickupSoA));
    memset(&e->projectiles, 0, sizeof(ProjectileSoA));
    memset(&e->explodingProjectiles, 0, sizeof(ProjectileSoA));
    memset(&e->dronePieces, 0, sizeof(DronePieceSoA));
    memset(&e->explosions, 0, sizeof(ExplosionSoA));
}

#endif
