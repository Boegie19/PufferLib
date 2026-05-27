#ifndef IMPULSE_WARS_TYPES_H
#define IMPULSE_WARS_TYPES_H

#include "fast_sim.h"
#include "fs_compat.h"
#include "id_pool.h"
#include "raylib.h"
#include "rlights.h"

#include "include/cc_array.h"

#include "settings.h"

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

typedef struct entityID {
    int32_t id;
    uint16_t generation;
} entityID;

// general purpose entity object
typedef struct entity {
    entityID *id;
    uint32_t generation;
    enum entityType type;
    void *entity;
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
    fsBody *body;
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
    fsBody *body;
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

    fsBody *body;
    fsBody *sensor;
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

    fsBody *body;
    fsVec2 pos;
    float health;
    float duration;

    entity *ent;
} shieldEntity;

typedef struct dronePieceEntity {
    uint8_t droneIdx;

    fsBody *body;
    fsVec2 pos;
    fsRot rot;
    fsVec2 vertices[3];
    bool isShieldPiece;

    entity *ent;

    uint16_t lifetime;
} dronePieceEntity;



typedef struct droneEntity {
    fsBody *body;
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
    uint8_t spawnedWeaponPickups[_NUM_WEAPONS];
    weaponInformation *defaultWeapon;
    CC_Array *entities;
    CC_Array *cells;
    CC_Array *walls;
    CC_Array *floatingWalls;
    CC_Array *drones;
    CC_Array *pickups;
    CC_Array *projectiles;
    CC_Array *explodingProjectiles;
    CC_Array *dronePieces;

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
    CC_Array *explosions;
    CC_Array *projectilePool;
    CC_Array *dronePiecePool;
    CC_Array *explosionPool;
    CC_Array *brakeTrailPointPool;
    CC_Array *entityIdPool;
    CC_Array *debugPoints;

} iwEnv;

#endif
