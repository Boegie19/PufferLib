#ifndef IMPULSE_WARS_GAME_H
#define IMPULSE_WARS_GAME_H

#include "helpers.h"
#include "settings.h"
#include "types.h"
#include "fs_compat.h"

// these functions call each other so need to be forward declared
void destroyProjectile(iwEnv *e, projectileEntity *projectile, const bool processExplosions, const bool full);
void createExplosion(iwEnv *e, droneEntity *drone, const projectileEntity *projectile, const fsVec2 pos, float radius, float magnitude, uint32_t maskBits);
void handleProjectileEndTouch(iwEnv *e, const entity *sensor, entity *visitor);

void updateTrailPoints(trailPoints *tp, const uint8_t maxLen, const fsVec2 pos);

entity *createEntity(iwEnv *e, enum entityType type, void *entityData) {
    entity *ent = fastCalloc(1, sizeof(entity));
    ent->id = fastCalloc(1, sizeof(entityID));
    ent->generation += 1;
    ent->type = type;
    ent->entity = entityData;
    ent->id->id = cc_array_size(e->entities) + 1;
    ent->id->generation = ent->generation;
    cc_array_add(e->entities, ent);
    return ent;
}

void destroyEntity(iwEnv *e, entity *ent) {
    ent->id->id = 0;
    // In a real pool we'd add to freelist
}

// will return a pointer to an entity or NULL if the given entity ID is
// invalid or orphaned
entity *getEntityByID(const iwEnv *e, const entityID *id) {
    if (id->id < 1 || (int64_t)cc_array_size(e->entities) < id->id) {
        // invalid index
        return NULL;
    }
    entity *ent = safe_array_get_at(e->entities, id->id - 1);
    if (ent->id->id == 0 || ent->generation != id->generation) {
        // orphaned entity
        return NULL;
    }
    return ent;
}

static inline bool entityTypeIsWall(const enum entityType type) {
    // walls are the first 3 entity types
    return type <= DEATH_WALL_ENTITY;
}

static inline int16_t cellIndex(const iwEnv *e, const int8_t col, const int8_t row) {
    return col + (row * e->map->columns);
}

// discretizes an entity's position into a cell index; -1 is returned if
// the position is out of bounds of the map
static inline int16_t entityPosToCellIdx(const iwEnv *e, const fsVec2 pos) {
    const float cellX = pos.x + (((float)e->map->columns * WALL_THICKNESS) / 2.0f);
    const float cellY = pos.y + (((float)e->map->rows * WALL_THICKNESS) / 2.0f);
    const int8_t cellCol = cellX / WALL_THICKNESS;
    const int8_t cellRow = cellY / WALL_THICKNESS;
    const int16_t cellIdx = cellIndex(e, cellCol, cellRow);
    // set the cell to -1 if it's out of bounds
    if (cellIdx < 0 || (uint16_t)cellIdx >= cc_array_size(e->cells)) {
        DEBUG_LOGF("invalid cell index: %d from position: (%f, %f)", cellIdx, pos.x, pos.y);
        return -1;
    }
    return cellIdx;
}

// returns true if the given position overlaps with shapes in a bounding
// box with a height and width of distance
bool isOverlappingAABB(const iwEnv *e, const fsVec2 pos, const float distance, const uint32_t categoryBits, const uint32_t maskBits) {
    fsAABB query = {
        .min = {.x = pos.x - distance, .y = pos.y - distance},
        .max = {.x = pos.x + distance, .y = pos.y + distance},
    };
    
    for (uint16_t i = 0; i < MAX_BODIES; i++) {
        const fsBody *b = &e->world.bodies[i];
        if (!b->isActive) continue;
        if (!(b->categoryBits & maskBits)) continue;
        
        // Simple AABB vs Body check
        if (b->shape.type == FS_CIRCLE) {
            float r = b->shape.circle.radius;
            if (b->pos.x + r < query.min.x || b->pos.x - r > query.max.x ||
                b->pos.y + r < query.min.y || b->pos.y - r > query.max.y) continue;
            return true;
        } else if (b->shape.type == FS_BOX) {
            float hx = b->shape.box.halfExtents.x;
            float hy = b->shape.box.halfExtents.y;
            if (b->pos.x + hx < query.min.x || b->pos.x - hx > query.max.x ||
                b->pos.y + hy < query.min.y || b->pos.y - hy > query.max.y) continue;
            return true;
        }
    }
    return false;
}


    droneEntity *drone;

    switch (ent->type) {
    case STANDARD_WALL_ENTITY:
    case BOUNCY_WALL_ENTITY:
    case DEATH_WALL_ENTITY:
        wall = ent->entity;
        transform.p = wall->pos;
        transform.q = wall->rot;
        return transform;
    case PROJECTILE_ENTITY:
        proj = ent->entity;
        transform.p = proj->pos;
        transform.q = fsRot_identity;
        return transform;
    case DRONE_ENTITY:
        drone = ent->entity;
        transform.p = drone->pos;
        transform.q = fsRot_identity;
        return transform;
    default:
        ERRORF("unknown entity type: %d", ent->type);
    }
}

// returns the closest points between two entities
typedef struct {
    float distance;
    fsVec2 normal;
} fsDistanceOutput;

fsDistanceOutput closestPoint(const entity *srcEnt, const entity *dstEnt) {
    fsVec2 p1 = fsVec2_zero;
    fsVec2 p2 = fsVec2_zero;
    float r1 = 0;
    float r2 = 0;

    switch (srcEnt->type) {
        case DRONE_ENTITY: p1 = ((droneEntity*)srcEnt->entity)->pos; r1 = DRONE_RADIUS; break;
        case SHIELD_ENTITY: p1 = ((shieldEntity*)srcEnt->entity)->pos; r1 = DRONE_SHIELD_RADIUS; break;
        case PROJECTILE_ENTITY: p1 = ((projectileEntity*)srcEnt->entity)->pos; r1 = ((projectileEntity*)srcEnt->entity)->weaponInfo->radius; break;
        default: break;
    }

    switch (dstEnt->type) {
        case DRONE_ENTITY: p2 = ((droneEntity*)dstEnt->entity)->pos; r2 = DRONE_RADIUS; break;
        case SHIELD_ENTITY: p2 = ((shieldEntity*)dstEnt->entity)->pos; r2 = DRONE_SHIELD_RADIUS; break;
        case PROJECTILE_ENTITY: p2 = ((projectileEntity*)dstEnt->entity)->pos; r2 = ((projectileEntity*)dstEnt->entity)->weaponInfo->radius; break;
        case STANDARD_WALL_ENTITY:
        case BOUNCY_WALL_ENTITY:
        case DEATH_WALL_ENTITY: p2 = ((wallEntity*)dstEnt->entity)->pos; r2 = 1.0f; break; // Approximation
        default: break;
    }

    fsDistanceOutput output;
    fsVec2 delta = fsSub(p2, p1);
    float distSq = fsLengthSq(delta);
    if (distSq < 0.0001f) {
        output.distance = 0;
        output.normal = (fsVec2){0, 1};
    } else {
        float dist = sqrtf(distSq);
        output.distance = dist - r1 - r2;
        output.normal = fsMul(delta, 1.0f/dist);
    }
    return output;
}

// returns true if there are shapes that match filter between startPos and endPos
bool posBehindWall(const iwEnv *e, const fsVec2 srcPos, const fsVec2 dstPos, const entity *dstEnt, const uint32_t categoryBits, const uint32_t maskBits, const enum entityType *targetType) {
    const float rayDistance = fsLength(fsSub(dstPos, srcPos));
    if (rayDistance <= 1.0f) {
        return false;
    }

    fsVec2 dir = fsSub(dstPos, srcPos);
    fsRayCastResult res;
    if (fsRayCast((fsWorld*)&e->world, srcPos, dir, 1.0f, maskBits, &res)) {
        if (res.body->userData == dstEnt) return false;
        if (targetType != NULL) {
            entity *hitEnt = res.body->userData;
            if (hitEnt->type == *targetType) return false;
        }
        return true;
    }
    return false;
}

bool isOverlappingCircleInLineOfSight(const iwEnv *e, const entity *ent, const fsVec2 startPos, const float radius, const uint32_t categoryBits, const uint32_t maskBits, const enum entityType *targetType) {
    for (uint16_t i = 0; i < MAX_BODIES; i++) {
        fsBody *b = (fsBody*)&e->world.bodies[i];
        if (!b->isActive) continue;
        if (!(b->categoryBits & maskBits)) continue;
        if (targetType != NULL && ((entity*)b->userData)->type != *targetType) continue;

        float dist = fsLength(fsSub(b->pos, startPos));
        float combinedRadius = radius + (b->shape.type == FS_CIRCLE ? b->shape.circle.radius : 0.0f); // simplified
        if (dist < combinedRadius) {
            // Check line of sight
            if (!posBehindWall(e, startPos, b->pos, ent, 0, WALL_SHAPE | FLOATING_WALL_SHAPE, targetType)) {
                return true;
            }
        }
    }
    return false;
}

uint8_t cellOffsets[8][2] = {
    {-1, 0},  // left
    {1, 0},   // right
    {0, -1},  // up
    {0, 1},   // down
    {-1, -1}, // top-left
    {1, -1},  // top-right
    {-1, 1},  // bottom-left
    {1, 1},   // bottom-right
};

// returns true and sets emptyPos to the position of an empty cell
// that is an appropriate distance away from other entities if one exists;
// if quad is set to -1 a random valid position from anywhere on the map
// will be returned, otherwise a position within the specified quadrant
// will be returned
bool findOpenPos(iwEnv *e, const enum shapeCategory shapeType, fsVec2 *emptyPos, int8_t quad) {
    uint8_t checkedCells[BITNSLOTS(MAX_CELLS)] = {0};
    const size_t nCells = cc_array_size(e->cells) - 1;
    uint16_t attempts = 0;
    bool laxDroneDistanceChecks = false;
    float minSpawnDistance = MIN_SPAWN_DISTANCE;

    while (true) {
        if (attempts == nCells) {
            // if we're trying to find a position for a drone and sudden
            // death walls have been placed, try again this time ignoring
            // distance checks; the drone must be spawned next
            // to a death wall in this case
            if (shapeType == DRONE_SHAPE && e->suddenDeathWallsPlaced && !laxDroneDistanceChecks) {
                attempts = 0;
                memset(checkedCells, 0x0, BITNSLOTS(MAX_CELLS));
                laxDroneDistanceChecks = true;
                minSpawnDistance = MIN_SD_SPAWN_DISTANCE;
                continue;
            }
            return false;
        }

        uint16_t cellIdx;
        if (quad == -1) {
            cellIdx = randInt(&e->randState, 0, nCells);
        } else {
            const float minX = e->map->spawnQuads[quad].min.x;
            const float minY = e->map->spawnQuads[quad].min.y;
            const float maxX = e->map->spawnQuads[quad].max.x;
            const float maxY = e->map->spawnQuads[quad].max.y;

            fsVec2 randPos = {.x = randFloat(&e->randState, minX, maxX), .y = randFloat(&e->randState, minY, maxY)};
            cellIdx = entityPosToCellIdx(e, randPos);
        }
        if (bitTest(checkedCells, cellIdx)) {
            continue;
        }
        bitSet(checkedCells, cellIdx);
        attempts++;

        const mapCell *cell = safe_array_get_at(e->cells, cellIdx);
        if (cell->ent != NULL) {
            continue;
        }

        bool tooClose = false;
        switch (shapeType) {
        case WEAPON_PICKUP_SHAPE:
            // ensure pickups don't spawn too close to other pickups
            for (uint8_t i = 0; i < cc_array_size(e->pickups); i++) {
                const weaponPickupEntity *pickup = safe_array_get_at(e->pickups, i);
                if (fsDistanceSq(cell->pos, pickup->pos) < PICKUP_SPAWN_DISTANCE_SQUARED) {
                    tooClose = true;
                    break;
                }
            }
            if (tooClose) {
                continue;
            }
            break;
        case DRONE_SHAPE:
            if (e->suddenDeathWallsPlaced) {
                if (!laxDroneDistanceChecks) {
                    // if sudden death walls have been placed, ignore the
                    // spawn points as they may be covered by death walls;
                    // instead just try and find a cell that doesn't neighbor
                    // a death wall
                    const uint8_t cellCol = cellIdx / e->map->columns;
                    const uint8_t cellRow = cellIdx % e->map->columns;
                    bool deathWallNeighboring = false;
                    for (uint8_t i = 0; i < 8; i++) {
                        const int8_t col = cellCol + cellOffsets[i][0];
                        const int8_t row = cellRow + cellOffsets[i][1];
                        if (row < 0 || row >= e->map->rows || col < 0 || col >= e->map->columns) {
                            continue;
                        }
                        const int16_t testCellIdx = cellIndex(e, col, row);
                        const mapCell *testCell = safe_array_get_at(e->cells, testCellIdx);
                        if (testCell->ent != NULL && testCell->ent->type == DEATH_WALL_ENTITY) {
                            deathWallNeighboring = true;
                            break;
                        }
                    }
                    if (deathWallNeighboring) {
                        continue;
                    }
                }

                // ensure drones aren't spawning on top of each other
                for (uint8_t i = 0; i < cc_array_size(e->drones); i++) {
                    const droneEntity *drone = safe_array_get_at(e->drones, i);
                    if (drone->dead) {
                        continue;
                    }
                    if (fsDistanceSq(cell->pos, drone->pos) == 0.0f) {
                        tooClose = true;
                        break;
                    }
                }
                if (tooClose) {
                    continue;
                }
            } else {
                if (!e->map->droneSpawns[cellIdx]) {
                    continue;
                }

                // ensure drones don't spawn too close to other drones
                for (uint8_t i = 0; i < cc_array_size(e->drones); i++) {
                    const droneEntity *drone = safe_array_get_at(e->drones, i);
                    if (drone->dead) {
                        continue;
                    }
                    if (fsDistanceSq(cell->pos, drone->pos) < DRONE_DRONE_SPAWN_DISTANCE_SQUARED) {
                        tooClose = true;
                        break;
                    }
                }
                if (tooClose) {
                    continue;
                }
            }
            break;
        default:
            break;
        }

        uint32_t maskBits = FLOATING_WALL_SHAPE | WEAPON_PICKUP_SHAPE | DRONE_SHAPE;
        if (shapeType != FLOATING_WALL_SHAPE && !laxDroneDistanceChecks) {
            maskBits &= ~shapeType;
        }

        if (!isOverlappingAABB(e, cell->pos, minSpawnDistance, shapeType, maskBits)) {
            *emptyPos = cell->pos;
            return true;
        }
    }
}

entity *createWall(iwEnv *e, const fsVec2 pos, const float width, const float height, int16_t cellIdx, const enum entityType type, const bool floating) {
    ASSERT(cellIdx != -1);
    ASSERT(entityTypeIsWall(type));

    fsBody *b = fsWorld_CreateBody(&e->world);
    b->pos = pos;
    b->isStatic = !floating;
    b->friction = STANDARD_WALL_FRICTION;
    b->restitution = (type == BOUNCY_WALL_ENTITY) ? BOUNCY_WALL_RESTITUTION : STANDARD_WALL_RESTITUTION;
    b->categoryBits = floating ? FLOATING_WALL_SHAPE : WALL_SHAPE;
    b->maskBits = FLOATING_WALL_SHAPE | PROJECTILE_SHAPE | DRONE_SHAPE | SHIELD_SHAPE | DRONE_PIECE_SHAPE;
    if (floating) {
        b->maskBits |= WALL_SHAPE | WEAPON_PICKUP_SHAPE;
    }

    fsVec2 extent = {.x = width / 2.0f, .y = height / 2.0f};
    b->shape.type = FS_BOX;
    b->shape.box.halfExtents = extent;

    wallEntity *wall = fastCalloc(1, sizeof(wallEntity));
    wall->body = b;
    wall->pos = pos;
    wall->rot = fsRot_identity;
    wall->velocity = fsVec2_zero;
    wall->extent = extent;
    wall->mapCellIdx = cellIdx;
    wall->isFloating = floating;
    wall->type = type;
    wall->isSuddenDeath = e->suddenDeathWallsPlaced;

    entity *ent = createEntity(e, type, wall);
    wall->ent = ent;
    b->userData = ent;

    if (floating) {
        cc_array_add(e->floatingWalls, wall);
        memset(wall->contributions, 0, _MAX_DRONES * sizeof(fsVec2));
    } else {
        cc_array_add(e->walls, wall);
    }

    return ent;
}

void destroyWall(iwEnv *e, wallEntity *wall, const bool full) {
    destroyEntity(e, wall->ent);

    if (full) {
        mapCell *cell = safe_array_get_at(e->cells, wall->mapCellIdx);
        cell->ent = NULL;
    }

    if (wall->isFloating) {
        for (int i = 0; i < _MAX_DRONES; i++) {
            wall->contributions[i] = fsVec2_zero;
        }
    }

    fsWorld_DestroyBody(wall->body);
    fastFree(wall);
}

enum weaponType randWeaponPickupType(iwEnv *e) {
    // spawn weapon pickups according to their spawn weights and how many
    // pickups are currently spawned with different weapons
    float totalWeight = 0.0f;
    float spawnWeights[_NUM_WEAPONS - 1] = {0};
    for (uint8_t i = 1; i < NUM_WEAPONS; i++) {
        if (i == e->defaultWeapon->type) {
            continue;
        }
        spawnWeights[i - 1] = weaponInfos[i]->spawnWeight / ((e->spawnedWeaponPickups[i] + 1) * 2.0f);
        totalWeight += spawnWeights[i - 1];
    }

    const float randPick = randFloat(&e->randState, 0.0f, totalWeight);
    float cumulativeWeight = 0.0f;
    enum weaponType type = STANDARD_WEAPON;
    for (uint8_t i = 1; i < NUM_WEAPONS; i++) {
        if (i == e->defaultWeapon->type) {
            continue;
        }
        cumulativeWeight += spawnWeights[i - 1];
        if (randPick < cumulativeWeight) {
            type = i;
            break;
        }
    }
    ASSERT(type != STANDARD_WEAPON && type != e->defaultWeapon->type);
    e->spawnedWeaponPickups[type]++;

    return type;
}

void createWeaponPickupBodyShape(const iwEnv *e, weaponPickupEntity *pickup) {
    pickup->bodyDestroyed = false;

    fsBody *b = fsWorld_CreateBody((fsWorld*)&e->world);
    b->pos = pickup->pos;
    b->isStatic = true;
    b->isSensor = true;
    b->categoryBits = WEAPON_PICKUP_SHAPE;
    b->maskBits = FLOATING_WALL_SHAPE | DRONE_SHAPE;
    b->userData = pickup->ent;
    b->shape.type = FS_BOX;
    b->shape.box.halfExtents = (fsVec2){PICKUP_THICKNESS / 2.0f, PICKUP_THICKNESS / 2.0f};
    pickup->body = b;
}

void createWeaponPickup(iwEnv *e) {
    // ensure weapon pickups are initially spawned somewhat uniformly
    fsVec2 pos;
    e->lastSpawnQuad = (e->lastSpawnQuad + 1) % 4;
    if (!findOpenPos(e, WEAPON_PICKUP_SHAPE, &pos, e->lastSpawnQuad)) {
        ERROR("no open position for weapon pickup");
    }

    weaponPickupEntity *pickup = fastCalloc(1, sizeof(weaponPickupEntity));
    pickup->weapon = randWeaponPickupType(e);
    pickup->respawnWait = 0.0f;
    pickup->floatingWallsTouching = 0;
    pickup->pos = pos;

    entity *ent = createEntity(e, WEAPON_PICKUP_ENTITY, pickup);
    pickup->ent = ent;

    const int16_t cellIdx = entityPosToCellIdx(e, pos);
    if (cellIdx == -1) {
        ERRORF("invalid position for weapon pickup spawn: (%f, %f)", pos.x, pos.y);
    }
    pickup->mapCellIdx = cellIdx;
    mapCell *cell = safe_array_get_at(e->cells, cellIdx);
    cell->ent = ent;

    createWeaponPickupBodyShape(e, pickup);

    cc_array_add(e->pickups, pickup);
}

void destroyWeaponPickup(iwEnv *e, weaponPickupEntity *pickup) {
    destroyEntity(e, pickup->ent);

    mapCell *cell = safe_array_get_at(e->cells, pickup->mapCellIdx);
    cell->ent = NULL;

    if (!pickup->bodyDestroyed) {
        fsWorld_DestroyBody(pickup->body);
    }

    fastFree(pickup);
}

// destroys the pickup body and shape while the pickup is waiting to
// respawn to avoid spurious sensor overlap checks; enabling/disabling
// a body is almost as expensive and creating a new body in box2d, and
// manually moving (teleporting) it is expensive as well so destroying
// the body now and re-creating it later is the fastest
void disableWeaponPickup(iwEnv *e, weaponPickupEntity *pickup) {
    DEBUG_LOGF("disabling weapon pickup at cell %d (%f, %f)", pickup->mapCellIdx, pickup->pos.x, pickup->pos.y);

    pickup->respawnWait = PICKUP_RESPAWN_WAIT;
    if (e->suddenDeathWallsPlaced) {
        pickup->respawnWait = SUDDEN_DEATH_PICKUP_RESPAWN_WAIT;
    }
    fsWorld_DestroyBody(pickup->body);
    pickup->bodyDestroyed = true;

    mapCell *cell = safe_array_get_at(e->cells, pickup->mapCellIdx);
    ASSERT(cell->ent != NULL);
    cell->ent = NULL;

    e->spawnedWeaponPickups[pickup->weapon]--;
}

void createDroneShield(iwEnv *e, droneEntity *drone, const int8_t groupIdx) {
    fsBody *b = fsWorld_CreateBody(&e->world);
    b->pos = drone->pos;
    b->isStatic = false; // It follows the drone, but for now let's just make it a body.
    // In Impulse Wars, the shield is kinematic and follows the drone.
    // I'll need to manually update its position in the step.
    b->categoryBits = SHIELD_SHAPE;
    b->maskBits = PROJECTILE_SHAPE | DRONE_SHAPE | WALL_SHAPE | FLOATING_WALL_SHAPE | SHIELD_SHAPE;
    b->shape.type = FS_CIRCLE;
    b->shape.circle.radius = DRONE_SHIELD_RADIUS;

    shieldEntity *shield = fastCalloc(1, sizeof(shieldEntity));
    shield->drone = drone;
    shield->body = b;
    shield->pos = drone->pos;
    shield->health = DRONE_SHIELD_MAX_HEALTH;
    float duration = DRONE_SHIELD_START_DURATION;
    if (drone->livesLeft != DRONE_LIVES) {
        duration = DRONE_SHIELD_RESPAWN_DURATION;
    }
    shield->duration = duration;

    entity *ent = createEntity(e, SHIELD_ENTITY, shield);
    shield->ent = ent;
    b->userData = ent;

    drone->shield = shield;
}

void createDrone(iwEnv *e, const uint8_t idx) {
    fsVec2 spawnPos;
    int8_t spawnQuad = -1;
    if (!e->isTraining) {
        if (e->lastSpawnQuad == -1) {
            spawnQuad = randInt(&e->randState, 0, 3);
        } else if (e->numDrones == 2) {
            spawnQuad = 3 - e->lastSpawnQuad;
        } else {
            spawnQuad = (e->lastSpawnQuad + 1) % 4;
        }
        e->lastSpawnQuad = spawnQuad;
    }
    if (!findOpenPos(e, DRONE_SHAPE, &spawnPos, spawnQuad)) {
        ERROR("no open position for drone");
    }

    fsBody *b = fsWorld_CreateBody(&e->world);
    b->pos = spawnPos;
    b->isStatic = false;
    b->friction = DRONE_FRICTION;
    b->restitution = DRONE_RESTITUTION;
    b->invMass = DRONE_INV_MASS;
    b->categoryBits = DRONE_SHAPE;
    b->maskBits = WALL_SHAPE | FLOATING_WALL_SHAPE | WEAPON_PICKUP_SHAPE | PROJECTILE_SHAPE | DRONE_SHAPE | SHIELD_SHAPE;
    b->shape.type = FS_CIRCLE;
    b->shape.circle.radius = DRONE_RADIUS;

    droneEntity *drone = fastCalloc(1, sizeof(droneEntity));
    drone->body = b;
    drone->weaponInfo = e->defaultWeapon;
    drone->ammo = weaponAmmo(e->defaultWeapon->type, drone->weaponInfo->type);
    drone->energyLeft = DRONE_ENERGY_MAX;
    drone->idx = idx;
    drone->team = idx;
    if (e->teamsEnabled) {
        drone->team = idx / (e->numDrones / 2);
    }
    drone->initalPos = spawnPos;
    drone->pos = spawnPos;
    drone->mapCellIdx = entityPosToCellIdx(e, spawnPos);
    drone->lastAim = (fsVec2){.x = 0.0f, .y = -1.0f};
    drone->livesLeft = DRONE_LIVES;
    create_array(&drone->brakeTrailPoints, 64);
    drone->respawnGuideLifetime = UINT16_MAX;
    memset(&drone->stepInfo, 0x0, sizeof(droneStepInfo));
    drone->killedBy = -1;

    entity *ent = createEntity(e, DRONE_ENTITY, drone);
    drone->ent = ent;
    b->userData = ent;

    cc_array_add(e->drones, drone);

    createDroneShield(e, drone, -(idx + 1));
}

void droneAddEnergy(droneEntity *drone, float energy) {
    // if a burst is charging, add the energy to the burst charge
    if (drone->chargingBurst) {
        drone->burstCharge = clamp(drone->burstCharge + energy);
    } else {
        drone->energyLeft = clamp(drone->energyLeft + energy);
    }
}

void createDronePiece(iwEnv *e, droneEntity *drone, const bool fromShield) {
    const float distance = randFloat(&e->randState, DRONE_PIECE_MIN_DISTANCE, DRONE_PIECE_MAX_DISTANCE);
    const fsVec2 direction = fsNormalize((fsVec2){.x = randFloat(&e->randState, -1.0f, 1.0f), .y = randFloat(&e->randState, -1.0f, 1.0f)});
    const fsVec2 pos = fsAdd(drone->pos, fsMul(direction, distance));
    const float angle = randFloat(&e->randState, -PI, PI);

    dronePieceEntity *piece;
    if (cc_array_size(e->dronePiecePool) > 0) {
        cc_array_remove_last(e->dronePiecePool, (void **)&piece);
        memset(piece, 0, sizeof(dronePieceEntity));
    } else {
        piece = fastCalloc(1, sizeof(dronePieceEntity));
    }

    fsBody *b = fsWorld_CreateBody(&e->world);
    b->pos = pos;
    b->angle = angle;
    b->isStatic = false;
    b->invMass = 1.0f; // Unit mass for pieces
    
    const float bonus = 1.0f + min(fsLength(drone->velocity) / 15.0f, 5.0f);
    const float speed = randFloat(&e->randState, DRONE_PIECE_MIN_SPEED, DRONE_PIECE_MAX_SPEED) * bonus;
    b->vel = fsMul(direction, speed);
    
    b->categoryBits = DRONE_PIECE_SHAPE;
    b->maskBits = WALL_SHAPE | FLOATING_WALL_SHAPE | DRONE_PIECE_SHAPE;
    b->shape.type = FS_CIRCLE;
    b->shape.circle.radius = fromShield ? 0.2f : 0.4f;

    piece->droneIdx = drone->idx;
    piece->pos = pos;
    piece->rot = fsMakeRot(angle);
    piece->isShieldPiece = fromShield;
    piece->lifetime = randInt(&e->randState, 5 * e->frameRate, 10 * e->frameRate);
    piece->body = b;

    entity *ent = createEntity(e, DRONE_PIECE_ENTITY, piece);
    piece->ent = ent;
    b->userData = ent;

    cc_array_add(e->dronePieces, piece);
}


void destroyDronePiece(iwEnv *e, dronePieceEntity *piece) {
    fsWorld_DestroyBody(piece->body);
    destroyEntity(e, piece->ent);
    cc_array_add(e->dronePiecePool, piece);
}

void destroyDroneShield(iwEnv *e, shieldEntity *shield, const bool createPieces) {
    droneEntity *drone = shield->drone;
    const float health = shield->health;
    if (health <= 0.0f) {
        droneAddEnergy(drone, DRONE_SHIELD_BREAK_ENERGY_COST);
    }
    drone->shield = NULL;
    e->stats[drone->idx].ownShieldBroken++;

    fsWorld_DestroyBody(shield->body);
    destroyEntity(e, shield->ent);
    fastFree(shield);

    if (!createPieces || health > 0.0f) {
        return;
    }

    // only create pieces if the shield was broken early
    for (uint8_t i = 0; i < DRONE_PIECE_COUNT; i++) {
        createDronePiece(e, drone, true);
    }
}

void destroyDrone(iwEnv *e, droneEntity *drone) {
    for (size_t i = 0; i < cc_array_size(drone->brakeTrailPoints); i++) {
        brakeTrailPoint *trailPoint = safe_array_get_at(drone->brakeTrailPoints, i);
        cc_array_add(e->brakeTrailPointPool, trailPoint);
    }
    cc_array_destroy(drone->brakeTrailPoints);
    for (int i = 0; i < _MAX_DRONES; i++) {
        drone->contributions[i] = fsVec2_zero;
    }

    destroyEntity(e, drone->ent);

    shieldEntity *shield = drone->shield;
    if (shield != NULL) {
        destroyDroneShield(e, shield, false);
    }

    fsWorld_DestroyBody(drone->body);
    fastFree(drone);
}





void droneChangeWeapon(const iwEnv *e, droneEntity *drone, const enum weaponType newWeapon) {
    // top up ammo but change nothing else if the weapon is the same
    if (drone->weaponInfo->type != newWeapon || drone->dead) {
        drone->weaponInfo = weaponInfos[newWeapon];
        drone->weaponCooldown = 0.0f;
        drone->weaponCharge = 0.0f;
        drone->heat = 0;
    }
    drone->ammo = weaponAmmo(e->defaultWeapon->type, drone->weaponInfo->type);
}


void applyTrackedForce(const iwEnv *e, fsBody *b, fsVec2 *contributions, const fsVec2 force, const uint8_t srcIdx) {
    const fsVec2 accel = fsMul(force, b->invMass);
    b->vel = fsAdd(b->vel, fsMul(accel, e->deltaTime));
    contributions[srcIdx] = fsAdd(contributions[srcIdx], fsMul(accel, e->deltaTime));
}

void applyTrackedImpulse(const iwEnv *e, fsBody *b, fsVec2 *contributions, const fsVec2 impulse, const uint8_t srcIdx) {
    const fsVec2 accel = fsMul(impulse, b->invMass);
    b->vel = fsAdd(b->vel, accel);
    contributions[srcIdx] = fsAdd(contributions[srcIdx], accel);
}

void trackImpulse(const iwEnv *e, fsVec2 *contributions, const fsVec2 impulse, const uint8_t srcIdx) {
    contributions[srcIdx] = fsAdd(contributions[srcIdx], fsMul(impulse, DRONE_INV_MASS));
}

int8_t findBiggestContributor(const iwEnv *e, const fsVec2 contributions[], const fsVec2 velocity, float *maxMoveContrib) {
    // determine the killer by finding the drone that pushed the dead
    // drone towards the wall that killed it the most
    const fsVec2 deathNormal = fsNormalize(velocity);
    DEBUG_LOGF("> death normal (%f, %f) velocity (%f, %f)", deathNormal.x, deathNormal.y, velocity.x, velocity.y);
    float maxContrib = -FLT_MAX;
    int8_t killer = -1;
    for (uint8_t i = 0; i < e->numDrones; i++) {
        DEBUG_LOGF("---\n> src drone contrib %d %f (%f, %f)", i, fsLength(contributions[i]), contributions[i].x, contributions[i].y);
        if (fsVecEqual(contributions[i], fsVec2_zero)) {
            continue;
        }

        const float currentContrib = fsDot(contributions[i], deathNormal);
        if (currentContrib > maxContrib) {
            maxContrib = currentContrib;
            killer = i;
        }
    }

    *maxMoveContrib = maxContrib;
    return killer;
}


void findDroneKiller(iwEnv *e, droneEntity *drone, const wallEntity *killWall) {
    float maxMoveContrib = -FLT_MAX;
    DEBUG_LOG("finding drone killer");
    int8_t killer = findBiggestContributor(e, drone->contributions, drone->velocity, &maxMoveContrib);
    if (killWall != NULL && killWall->isFloating) {
        float wallContrib = -FLT_MAX;
        DEBUG_LOG("finding mover of floating death wall");
        const int8_t wallMover = findBiggestContributor(e, killWall->contributions, killWall->velocity, &wallContrib);
        if (wallContrib > maxMoveContrib) {
            DEBUG_LOGF(">>> drone %d killed by drone %d pushing floating death wall", drone->idx, wallMover);
            killer = wallMover;
        }
    }


    if (killer == -1) {
        DEBUG_LOGF(">>> drone %d killed by UNKNOWN", drone->idx);
        e->stats[drone->idx].unknownKills++;
        return;
    }

    DEBUG_LOGF(">>> drone %d killed by drone %d", drone->idx, killer);

    if (killer == drone->idx) {
        e->stats[drone->idx].selfKills++;
    } else {
        e->stats[killer].kills++;
    }
    drone->killedBy = killer;
    droneEntity *killerDrone = safe_array_get_at(e->drones, killer);
    killerDrone->killed[drone->idx] = true;
}

void killDrone(iwEnv *e, droneEntity *drone, const wallEntity *killWall) {
    if (drone->dead || drone->livesLeft == 0) {
        return;
    }
    DEBUG_LOGF("drone %d died", drone->idx);

    findDroneKiller(e, drone, killWall);

    drone->livesLeft--;
    drone->dead = true;
    drone->diedThisStep = true;
    drone->respawnWait = DRONE_RESPAWN_WAIT;

    for (uint8_t i = 0; i < DRONE_PIECE_COUNT; i++) {
        createDronePiece(e, drone, false);
    }

    drone->body->isActive = false;
    droneChangeWeapon(e, drone, e->defaultWeapon->type);
    drone->braking = false;
    drone->chargingBurst = false;
    drone->energyFullyDepleted = false;
    drone->shotThisStep = false;
    drone->velocity = fsVec2_zero;
    drone->lastVelocity = fsVec2_zero;
}

bool respawnDrone(iwEnv *e, droneEntity *drone) {
    fsVec2 pos;
    if (!findOpenPos(e, DRONE_SHAPE, &pos, -1)) {
        return false;
    }
    drone->body->pos = pos;
    drone->body->angle = 0;
    drone->body->isActive = true;
    drone->body->vel = fsVec2_zero;

    drone->dead = false;
    drone->pos = pos;
    drone->respawnGuideLifetime = UINT16_MAX;
    drone->killedBy = -1;

    droneAddEnergy(drone, DRONE_ENERGY_RESPAWN_REFILL);

    createDroneShield(e, drone, -(drone->idx + 1));

    if (e->client != NULL) {
        drone->trailPoints.length = 0;
    }

    return true;
}

void createProjectile(iwEnv *e, droneEntity *drone, const fsVec2 normAim) {
    ASSERT_VEC_NORMALIZED(normAim);

    const float radius = drone->weaponInfo->radius;
    float droneRadius = DRONE_RADIUS;
    if (drone->shield != NULL) {
        droneRadius = DRONE_SHIELD_RADIUS;
    }
    // spawn the projectile just outside the drone so they don't
    // immediately collide
    fsVec2 pos = fsMulAdd(drone->pos, droneRadius + (radius * 1.5f), normAim);
     // if the projectile is inside a wall or out of the map, move the
    // projectile to be just outside the wall
    bool projectileInWall = false;
    int16_t cellIdx = entityPosToCellIdx(e, pos);
    if (cellIdx == -1) {
        projectileInWall = true;
    } else {
        const mapCell *cell = safe_array_get_at(e->cells, cellIdx);
        if (cell->ent != NULL && entityTypeIsWall(cell->ent->type)) {
            projectileInWall = true;
        }
    }
    if (projectileInWall) {
        fsRayCastResult rayRes = fsRayCast(&e->world, drone->pos, normAim, droneRadius + (radius * 2.5f), (entity*)drone->ent);
        if (rayRes.hit) {
            fsVec2 hitPoint = fsAdd(drone->pos, fsMul(normAim, rayRes.fraction * (droneRadius + (radius * 2.5f))));
            pos = fsAdd(hitPoint, fsMul(normAim, -radius * 1.5f));
        }
    }

    fsBody *b = fsWorld_CreateBody(&e->world);
    b->pos = pos;
    b->isStatic = false;
    b->invMass = (drone->weaponInfo->mass > 0) ? 1.0f / drone->weaponInfo->mass : 0.0f;
    b->categoryBits = PROJECTILE_SHAPE;
    b->maskBits = WALL_SHAPE | FLOATING_WALL_SHAPE | PROJECTILE_SHAPE | DRONE_SHAPE | SHIELD_SHAPE;
    b->shape.type = FS_CIRCLE;
    b->shape.circle.radius = radius;
    b->isSensor = drone->weaponInfo->isSensor;

    projectileEntity *projectile;
    if (cc_array_size(e->projectilePool) > 0) {
        cc_array_remove_last(e->projectilePool, (void **)&projectile);
        memset(projectile, 0, sizeof(projectileEntity));
    } else {
        projectile = fastCalloc(1, sizeof(projectileEntity));
    }

    projectile->body = b;
    projectile->droneIdx = drone->idx;
    projectile->weaponInfo = drone->weaponInfo;
    projectile->pos = pos;
    projectile->lastPos = pos;
    
    // Add lateral velocity and weapon variance
    fsVec2 aim = weaponAdjustAim(&e->randState, drone->weaponInfo->type, drone->heat, normAim);
    projectile->velocity = fsMul(aim, drone->weaponInfo->speed);
    b->vel = projectile->velocity;

    projectile->lastVelocity = projectile->velocity;
    projectile->speed = drone->weaponInfo->speed;
    projectile->lastSpeed = drone->weaponInfo->speed;
    projectile->mapCellIdx = entityPosToCellIdx(e, pos);
    projectile->numDronesBehindWalls = 0;
    if (e->client != NULL) {
        memset(&projectile->trailPoints, 0x0, sizeof(trailPoints));
    }
    if (drone->weaponInfo->type == BLACK_HOLE_WEAPON) {
        create_array(&projectile->entsInBlackHole, 16);
    }
    projectile->needsToBeDestroyed = false;

    entity *ent = createEntity(e, PROJECTILE_ENTITY, projectile);
    projectile->ent = ent;
    b->userData = ent;

    cc_array_add(e->projectiles, projectile);
}

void createProjectileExplosion(iwEnv *e, projectileEntity *projectile, const bool initalProjectile) {
    if (projectile->needsToBeDestroyed) {
        return;
    }
    projectile->needsToBeDestroyed = true;
    cc_array_add(e->explodingProjectiles, projectile);

    droneEntity *drone = safe_array_get_at(e->drones, projectile->droneIdx);
    createExplosion(e, drone, projectile, projectile->pos, 2.0f, 10.0f, FLOATING_WALL_SHAPE | PROJECTILE_SHAPE | DRONE_SHAPE);
}

void fixProjectileSpeed(projectileEntity *projectile) {
    fsVec2 newVel = projectile->body->vel;
    float newSpeed = fsLength(newVel);
    if (newSpeed < projectile->lastSpeed) {
        newSpeed = projectile->lastSpeed;
        newVel = fsMul(fsNormalize(newVel), newSpeed);
        projectile->body->vel = newVel;
    }

    projectile->velocity = newVel;
    projectile->lastSpeed = newSpeed;
    projectile->speed = newSpeed;
}

#define MAX_WALL_HITS 8

typedef struct wallBurstImpulse {
    float distance;
    fsVec2 direction;
    float magnitude;
    uint16_t wallCellIdx;
} wallBurstImpulse;


void createExplosion(iwEnv *e, droneEntity *drone, const projectileEntity *projectile, const fsVec2 pos, float radius, float magnitude, uint32_t maskBits) {
    const bool isBurst = projectile == NULL;
    
    for (int i = 0; i < MAX_BODIES; i++) {
        fsBody *b = &e->world.bodies[i];
        if (!b->isActive || b->isStatic) continue;
        if (!(b->categoryBits & maskBits)) continue;

        fsVec2 delta = fsSub(b->pos, pos);
        float distSq = fsLengthSq(delta);
        if (distSq < radius * radius) {
            float dist = sqrtf(distSq);
            fsVec2 dir = (dist < 0.001f) ? (fsVec2){0, 1} : fsMul(delta, 1.0f/dist);
            float force = magnitude * (1.0f - dist/radius);
            
            // Check line of sight
            if (posBehindWall(e, pos, b->pos, b->userData, 0, WALL_SHAPE | FLOATING_WALL_SHAPE, NULL)) {
                continue;
            }

            b->vel = fsAdd(b->vel, fsMul(dir, force * b->invMass));
            
            entity *ent = b->userData;
            if (ent && ent->type == DRONE_ENTITY) {
                droneEntity *hitDrone = ent->entity;
                uint8_t srcIdx = isBurst ? drone->idx : projectile->droneIdx;
                hitDrone->contributions[srcIdx] = fsAdd(hitDrone->contributions[srcIdx], fsMul(dir, force * b->invMass));
            } else if (ent && entityTypeIsWall(ent->type)) {
                wallEntity *wall = ent->entity;
                if (wall->isFloating) {
                     uint8_t srcIdx = isBurst ? drone->idx : projectile->droneIdx;
                     wall->contributions[srcIdx] = fsAdd(wall->contributions[srcIdx], fsMul(dir, force * b->invMass));
                }
            }
        }
    }
}

void destroyProjectile(iwEnv *e, projectileEntity *projectile, const bool processExplosions, const bool full) {
    // explode projectile if necessary
    if (processExplosions && projectile->weaponInfo->explosive) {
        createProjectileExplosion(e, projectile, true);
    }

    destroyEntity(e, projectile->ent);
    fsWorld_DestroyBody(projectile->bodyID);

    if (full) {
        enum cc_stat res = cc_array_remove_fast(e->projectiles, projectile, NULL);
        MAYBE_UNUSED(res);
        ASSERT(res == CC_OK);
    }

    e->stats[projectile->droneIdx].shotDistances[projectile->weaponInfo->type] += projectile->distance;
    e->stats[projectile->droneIdx].totalShotDistances += projectile->distance;

    if (projectile->entsInBlackHole != NULL) {
        for (uint8_t i = 0; i < cc_array_size(projectile->entsInBlackHole); i++) {
            entityID *id = safe_array_get_at(projectile->entsInBlackHole, i);
            cc_array_add(e->entityIdPool, id);
        }
        cc_array_destroy(projectile->entsInBlackHole);
        projectile->entsInBlackHole = NULL;
    }

    cc_array_add(e->projectilePool, projectile);
}



// destroy projectiles that were caught in an explosion; projectiles
// can't be destroyed in explodeCallback because box2d assumes all shapes
// and bodies are valid for the lifetime of an AABB query
static inline void destroyExplodedProjectiles(iwEnv *e) {
    if (cc_array_size(e->explodingProjectiles) == 0) {
        return;
    }

    CC_ArrayIter iter;
    cc_array_iter_init(&iter, e->explodingProjectiles);
    projectileEntity *projectile;
    while (cc_array_iter_next(&iter, (void **)&projectile) != CC_ITER_END) {
        destroyProjectile(e, projectile, false, false);
        const enum cc_stat res = cc_array_remove_fast(e->projectiles, projectile, NULL);
        MAYBE_UNUSED(res);
        ASSERT(res == CC_OK);
    }
    cc_array_remove_all(e->explodingProjectiles);
}

void createSuddenDeathWalls(iwEnv *e, const fsVec2 startPos, const fsVec2 size) {
    int16_t endIdx;
    uint8_t indexIncrement;
    if (size.y == WALL_THICKNESS) {
        // horizontal walls
        const fsVec2 endPos = (fsVec2){.x = startPos.x + size.x, .y = startPos.y};
        endIdx = entityPosToCellIdx(e, endPos);
        if (endIdx == -1) {
            ERRORF("invalid position for sudden death wall: (%f, %f)", endPos.x, endPos.y);
        }
        indexIncrement = 1;
    } else {
        // vertical walls
        const fsVec2 endPos = (fsVec2){.x = startPos.x, .y = startPos.y + size.y};
        endIdx = entityPosToCellIdx(e, endPos);
        if (endIdx == -1) {
            ERRORF("invalid position for sudden death wall: (%f, %f)", endPos.x, endPos.y);
        }
        indexIncrement = e->map->columns;
    }
    const int16_t startIdx = entityPosToCellIdx(e, startPos);
    if (startIdx == -1) {
        ERRORF("invalid position for sudden death wall: (%f, %f)", startPos.x, startPos.y);
    }
    for (uint16_t i = startIdx; i <= endIdx; i += indexIncrement) {
        mapCell *cell = safe_array_get_at(e->cells, i);
        if (cell->ent != NULL) {
            if (cell->ent->type == WEAPON_PICKUP_ENTITY) {
                weaponPickupEntity *pickup = cell->ent->entity;
                disableWeaponPickup(e, pickup);
            } else {
                continue;
            }
        }
        entity *ent = createWall(e, cell->pos, WALL_THICKNESS, WALL_THICKNESS, i, DEATH_WALL_ENTITY, false);
        cell->ent = ent;
    }
}

void handleSuddenDeath(iwEnv *e) {
    ASSERT(e->suddenDeathSteps == 0);
    if (e->suddenDeathWallCounter >= e->map->maxSuddenDeathWalls) {
        return;
    }

    // create new walls that will close in on the arena
    e->suddenDeathWallCounter++;
    e->suddenDeathWallsPlaced = true;

    const float leftX = (e->suddenDeathWallCounter - 1) * WALL_THICKNESS;
    const float yOffset = (WALL_THICKNESS * (e->suddenDeathWallCounter - 1)) + (WALL_THICKNESS / 2);
    const float xWidth = WALL_THICKNESS * (e->map->columns - (e->suddenDeathWallCounter * 2) - 1);

    // top walls
    createSuddenDeathWalls(
        e,
        (fsVec2){
            .x = e->map->bounds.min.x + leftX,
            .y = e->map->bounds.min.y + yOffset,
        },
        (fsVec2){
            .x = xWidth,
            .y = WALL_THICKNESS,
        }
    );
    // bottom walls
    createSuddenDeathWalls(
        e,
        (fsVec2){
            .x = e->map->bounds.min.x + leftX,
            .y = e->map->bounds.max.y - yOffset,
        },
        (fsVec2){
            .x = xWidth,
            .y = WALL_THICKNESS,
        }
    );
    // left walls
    createSuddenDeathWalls(
        e,
        (fsVec2){
            .x = e->map->bounds.min.x + leftX,
            .y = e->map->bounds.min.y + (e->suddenDeathWallCounter * WALL_THICKNESS),
        },
        (fsVec2){
            .x = WALL_THICKNESS,
            .y = WALL_THICKNESS * (e->map->rows - (e->suddenDeathWallCounter * 2) - 2),
        }
    );
    // right walls
    createSuddenDeathWalls(
        e,
        (fsVec2){
            .x = e->map->bounds.min.x + ((e->map->columns - e->suddenDeathWallCounter - 2) * WALL_THICKNESS),
            .y = e->map->bounds.min.y + (e->suddenDeathWallCounter * WALL_THICKNESS),
        },
        (fsVec2){
            .x = WALL_THICKNESS,
            .y = WALL_THICKNESS * (e->map->rows - (e->suddenDeathWallCounter * 2) - 2),
        }
    );

    // mark drones as dead if they touch a newly placed wall
    for (uint8_t i = 0; i < e->numDrones; i++) {
        droneEntity *drone = safe_array_get_at(e->drones, i);
        if (isOverlappingCircleInLineOfSight(e, drone->ent, drone->pos, DRONE_RADIUS, 0, WALL_SHAPE, NULL)) {
            killDrone(e, drone, NULL);
        }
    }

    // make floating walls static bodies if they are now overlapping with
    // a newly placed wall, but destroy them if they are fully inside a wall
    CC_ArrayIter floatingWallIter;
    cc_array_iter_init(&floatingWallIter, e->floatingWalls);
    wallEntity *wall;
    while (cc_array_iter_next(&floatingWallIter, (void **)&wall) != CC_ITER_END) {
        const mapCell *cell = safe_array_get_at(e->cells, wall->mapCellIdx);
        if (cell->ent != NULL && entityTypeIsWall(cell->ent->type)) {
            // floating wall is overlapping with a wall, destroy it
            const enum cc_stat res = cc_array_iter_remove_fast(&floatingWallIter, NULL);
            MAYBE_UNUSED(res);
            ASSERT(res == CC_OK);

            const fsVec2 wallPos = wall->pos;
            MAYBE_UNUSED(wallPos);
            destroyWall(e, wall, false);
            DEBUG_LOGF("destroyed floating wall at %f, %f", wallPos.x, wallPos.y);
            continue;
        }
    }

    // detroy all projectiles that are now overlapping with a newly placed wall
    CC_ArrayIter projectileIter;
    cc_array_iter_init(&projectileIter, e->projectiles);
    projectileEntity *projectile;
    while (cc_array_iter_next(&projectileIter, (void **)&projectile) != CC_ITER_END) {
        const mapCell *cell = safe_array_get_at(e->cells, projectile->mapCellIdx);
        if (cell->ent != NULL && entityTypeIsWall(cell->ent->type)) {
            cc_array_iter_remove_fast(&projectileIter, NULL);
            destroyProjectile(e, projectile, false, false);
        }
    }
}

void droneMove(const iwEnv *e, droneEntity *drone, fsVec2 direction) {
    ASSERT_VEC_BOUNDED(direction);

    // if energy is fully depleted halve movement until energy starts
    // to refill again
    if (drone->energyFullyDepleted && drone->energyRefillWait != 0.0f) {
        direction = fsMul(0.5f, direction);
        drone->lastMove = direction;
    }
    const fsVec2 force = fsMul(DRONE_MOVE_MAGNITUDE, direction);
    applyTrackedForce(e, drone->bodyID, drone->contributions, force, drone->idx);
}

void droneShoot(iwEnv *e, droneEntity *drone, const fsVec2 aim, const bool chargingWeapon) {
    ASSERT(drone->ammo != 0);

    drone->shotThisStep = true;
    // TODO: rework heat to only increase when projectiles are fired,
    // and only cool down after the next shot was skipped
    drone->heat++;
    if (drone->weaponCooldown != 0.0f) {
        return;
    }
    const bool weaponNeedsCharge = drone->weaponInfo->charge != 0.0f;
    if (weaponNeedsCharge) {
        if (chargingWeapon) {
            drone->chargingWeapon = true;
            drone->weaponCharge = min(drone->weaponCharge + e->deltaTime, drone->weaponInfo->charge);
        } else if (drone->weaponCharge < drone->weaponInfo->charge) {
            drone->chargingWeapon = false;
            drone->weaponCharge = max(drone->weaponCharge - e->deltaTime, 0.0f);
        }
    }
    // if the weapon needs to be charged, only fire the weapon if it's
    // fully charged and the agent released the trigger
    if (weaponNeedsCharge && (chargingWeapon || drone->weaponCharge < drone->weaponInfo->charge)) {
        return;
    }

    if (drone->ammo != INFINITE) {
        drone->ammo--;
    }
    drone->weaponCooldown = drone->weaponInfo->coolDown;
    drone->chargingWeapon = false;
    drone->weaponCharge = 0.0f;

    fsVec2 normAim = drone->lastAim;
    if (!fsVecEqual(aim, fsVec2_zero)) {
        normAim = fsNormalize(aim);
    }
    ASSERT_VEC_NORMALIZED(normAim);
    fsVec2 recoil = fsMul(-drone->weaponInfo->recoilMagnitude, normAim);
    applyTrackedImpulse(e, drone->bodyID, drone->contributions, recoil, drone->idx);

    for (int i = 0; i < drone->weaponInfo->numProjectiles; i++) {
        createProjectile(e, drone, normAim);

        e->stats[drone->idx].shotsFired[drone->weaponInfo->type]++;
        e->stats[drone->idx].totalShotsFired++;
    }
    drone->stepInfo.firedShot = true;

    if (drone->ammo == 0) {
        droneChangeWeapon(e, drone, e->defaultWeapon->type);
        drone->weaponCooldown = drone->weaponInfo->coolDown;
    }
}

void droneBrake(iwEnv *e, droneEntity *drone, const bool brake) {
    if (drone->shield != NULL) {
        return;
    }
    // if the drone isn't braking or energy is fully depleted, return
    // unless the drone was braking during the last step
    if (!brake || drone->energyFullyDepleted) {
        if (drone->braking) {
            drone->braking = false;
            //drone->bodyID, DRONE_LINEAR_DAMPING);
            if (drone->energyRefillWait == 0.0f && !drone->chargingBurst) {
                drone->energyRefillWait = DRONE_ENERGY_REFILL_WAIT;
            }


            if (e->client != NULL) {
                brakeTrailPoint *trailPoint = NULL;
                if (cc_array_size(e->brakeTrailPointPool) > 0) {
                    cc_array_remove_last(e->brakeTrailPointPool, (void **)&trailPoint);
                    memset(trailPoint, 0, sizeof(brakeTrailPoint));
                } else {
                    trailPoint = fastCalloc(1, sizeof(brakeTrailPoint));
                }
                trailPoint->pos = drone->pos;
                trailPoint->lifetime = UINT16_MAX;
                trailPoint->isEnd = true;
                cc_array_add(drone->brakeTrailPoints, trailPoint);
            }
        }
        return;
    }
    ASSERT(!drone->energyFullyDepleted);

    // apply additional brake damping and decrease energy
    if (brake) {
        if (!drone->braking) {
            drone->braking = true;
            //drone->bodyID, DRONE_LINEAR_DAMPING * DRONE_BRAKE_DAMPING_COEF);

        }
        drone->energyLeft = max(drone->energyLeft - (DRONE_BRAKE_DRAIN_RATE * e->deltaTime), 0.0f);
        e->stats[drone->idx].brakeTime += e->deltaTime;
    }

    // if energy is empty but burst is being charged, let burst functions
    // handle energy refill
    if (drone->energyLeft == 0.0f && !drone->chargingBurst) {
        drone->energyFullyDepleted = true;
        drone->energyFullyDepletedThisStep = true;
        drone->energyRefillWait = DRONE_ENERGY_REFILL_EMPTY_WAIT;
        e->stats[drone->idx].energyEmptied++;
    }

    if (e->client != NULL) {
        brakeTrailPoint *trailPoint = NULL;
        if (cc_array_size(e->brakeTrailPointPool) > 0) {
            cc_array_remove_last(e->brakeTrailPointPool, (void **)&trailPoint);
            memset(trailPoint, 0, sizeof(brakeTrailPoint));
        } else {
            trailPoint = fastCalloc(1, sizeof(brakeTrailPoint));
        }
        trailPoint->pos = drone->pos;
        trailPoint->lifetime = UINT16_MAX;
        cc_array_add(drone->brakeTrailPoints, trailPoint);
    }
}

void droneChargeBurst(iwEnv *e, droneEntity *drone) {
    if (drone->energyFullyDepleted || drone->burstCooldown != 0.0f || drone->shield != NULL || (!drone->chargingBurst && drone->energyLeft < DRONE_BURST_BASE_COST)) {
        return;
    }

    // take energy and put it into burst charge
    if (drone->chargingBurst) {
        drone->burstCharge = min(drone->burstCharge + (DRONE_BURST_CHARGE_RATE * e->deltaTime), DRONE_ENERGY_MAX);
        drone->energyLeft = max(drone->energyLeft - (DRONE_BURST_CHARGE_RATE * e->deltaTime), 0.0f);
    } else {
        drone->burstCharge = min(drone->burstCharge + DRONE_BURST_BASE_COST, DRONE_ENERGY_MAX);
        drone->energyLeft = max(drone->energyLeft - DRONE_BURST_BASE_COST, 0.0f);
        drone->chargingBurst = true;
    }

    if (drone->energyLeft == 0.0f) {
        drone->energyFullyDepleted = true;
        e->stats[drone->idx].energyEmptied++;
    }
}

void droneBurst(iwEnv *e, droneEntity *drone) {
    if (!drone->chargingBurst) {
        return;
    }

    const float radius = (DRONE_BURST_RADIUS_BASE * drone->burstCharge) + DRONE_BURST_RADIUS_MIN;
    const float magnitude = (DRONE_BURST_IMPACT_BASE * drone->burstCharge) + DRONE_BURST_IMPACT_MIN;
    createExplosion(e, drone, NULL, drone->pos, radius, magnitude, WALL_SHAPE | FLOATING_WALL_SHAPE | PROJECTILE_SHAPE | DRONE_SHAPE);
    destroyExplodedProjectiles(e);

    drone->chargingBurst = false;
    drone->burstCharge = 0.0f;
    drone->burstCooldown = DRONE_BURST_COOLDOWN;
    if (drone->energyLeft == 0.0f) {
        drone->energyFullyDepletedThisStep = true;
        drone->energyRefillWait = DRONE_ENERGY_REFILL_EMPTY_WAIT;
    } else {
        drone->energyRefillWait = DRONE_ENERGY_REFILL_WAIT;
    }
    e->stats[drone->idx].totalBursts++;

    if (e->client != NULL) {
        explosionInfo *explInfo;
        if (cc_array_size(e->explosionPool) > 0) {
            cc_array_remove_last(e->explosionPool, (void **)&explInfo);
            memset(explInfo, 0, sizeof(explosionInfo));
        } else {
            explInfo = fastCalloc(1, sizeof(explosionInfo));
        }
        explInfo->def = explosion;
        explInfo->isBurst = true;
        explInfo->droneIdx = drone->idx;
        explInfo->renderSteps = UINT16_MAX;
        cc_array_add(e->explosions, explInfo);
    }

}

void droneDiscardWeapon(iwEnv *e, droneEntity *drone) {
    if (drone->weaponInfo->type == e->defaultWeapon->type || (drone->energyFullyDepleted && !drone->chargingBurst)) {
        return;
    }

    droneChangeWeapon(e, drone, e->defaultWeapon->type);
    droneAddEnergy(drone, -WEAPON_DISCARD_COST);
    if (drone->chargingBurst) {
        return;
    }

    if (drone->energyLeft == 0.0f) {
        drone->energyFullyDepleted = true;
        drone->energyFullyDepletedThisStep = true;
        drone->energyRefillWait = DRONE_ENERGY_REFILL_EMPTY_WAIT;
        e->stats[drone->idx].energyEmptied++;
    } else {
        drone->energyRefillWait = DRONE_ENERGY_REFILL_WAIT;
    }
}

// update drone state, respawn the drone if necessary; false is returned
// if no position could be found to respawn the drone at, and true otherwise
bool droneStep(iwEnv *e, droneEntity *drone) {
    if (drone->dead) {
        drone->respawnWait -= e->deltaTime;
        if (drone->respawnWait <= 0.0f) {
            const bool foundPos = respawnDrone(e, drone);
            if (!foundPos) {
                return false;
            }
        }
        return true;
    }

    // manage weapon charge and heat
    if (drone->weaponCooldown != 0.0f) {
        drone->weaponCooldown = max(drone->weaponCooldown - e->deltaTime, 0.0f);
    }
    if (!drone->shotThisStep) {
        drone->weaponCharge = max(drone->weaponCharge - e->deltaTime, 0);
        drone->heat = max(drone->heat - 1, 0);
    } else {
        drone->shotThisStep = false;
    }
    ASSERT(!drone->shotThisStep);

    // manage drone energy
    if (drone->burstCooldown != 0.0f) {
        drone->burstCooldown = max(drone->burstCooldown - e->deltaTime, 0.0f);
    }
    if (drone->energyFullyDepletedThisStep) {
        drone->energyFullyDepletedThisStep = false;
    } else if (drone->energyRefillWait != 0.0f) {
        drone->energyRefillWait = max(drone->energyRefillWait - e->deltaTime, 0.0f);
    } else if (drone->energyLeft != DRONE_ENERGY_MAX && !drone->chargingBurst) {
        // don't start recharging energy until the burst charge is used
        drone->energyLeft = min(drone->energyLeft + (DRONE_ENERGY_REFILL_RATE * e->deltaTime), DRONE_ENERGY_MAX);
    }
    if (drone->energyLeft == DRONE_ENERGY_MAX) {
        drone->energyFullyDepleted = false;
    }

    const float distance = fsDistance(drone->lastPos, drone->pos);
    e->stats[drone->idx].distanceTraveled += distance;

    shieldEntity *shield = drone->shield;
    if (shield != NULL) {
        shield->duration -= e->deltaTime;
        if (shield->duration <= 0.0f || shield->health <= 0.0f) {
            destroyDroneShield(e, shield, true);
        }
    }

    return true;
}

void handleBlackHolePull(iwEnv *e, projectileEntity *projectile) {
    ASSERT(projectile->weaponInfo->type == BLACK_HOLE_WEAPON);

    CC_ArrayIter entIter;
    cc_array_iter_init(&entIter, projectile->entsInBlackHole);
    entityID *id;
    while (cc_array_iter_next(&entIter, (void **)&id) != CC_ITER_END) {
        // check if the entity is still valid
        const entity *ent = getEntityByID(e, id);
        if (ent == NULL) {
            cc_array_add(e->entityIdPool, id);
            enum cc_stat res = cc_array_iter_remove_fast(&entIter, NULL);
            MAYBE_UNUSED(res);
            ASSERT(res == CC_OK);
            continue;
        }
        float distance = fsLength(fsSub(ent->pos, projectile->pos));
        if (posBehindWall(e, projectile->pos, ent->pos, ent, 0, WALL_SHAPE | FLOATING_WALL_SHAPE, NULL)) {
            continue;
        }

        fsBody *body = NULL;
        fsVec2 *contributions = NULL;
        bool hasShield = false;

        switch (ent->type) {
        case STANDARD_WALL_ENTITY:
        case BOUNCY_WALL_ENTITY:
        case DEATH_WALL_ENTITY: {
            wallEntity *wall = ent->entity;
            body = wall->body;
            if (wall->isFloating) contributions = wall->contributions;
            break;
        }
        case DRONE_ENTITY: {
            droneEntity *drone = ent->entity;
            body = drone->body;
            contributions = drone->contributions;
            hasShield = drone->shield != NULL;
            break;
        }
        case PROJECTILE_ENTITY: {
            projectileEntity *proj = ent->entity;
            body = proj->body;
            break;
        }
        default:
            break;
        }

        if (body == NULL) continue;

        fsVec2 direction = fsNormalize(fsSub(ent->pos, projectile->pos));
        float scale = 1.0f - (distance / BLACK_HOLE_PROXIMITY_RADIUS);
        if (scale < 0) scale = 0;

        float magnitude = BLACK_HOLE_PULL_MAGNITUDE * scale;
        if (hasShield) {
            magnitude *= DRONE_SHIELD_EXPLOSION_REDUCTION;
        }
        fsVec2 force = fsMul(direction, -magnitude); // Pull towards black hole

        if (contributions) {
            applyTrackedForce(e, body, contributions, force, projectile->droneIdx);
        } else {
            b->vel = fsAdd(b->vel, fsMul(fsMul(force, b->invMass), e->deltaTime)); //body, force, true);
        }
    }
}
void projectilesStep(iwEnv *e) {
    CC_ArrayIter projIter;
    cc_array_iter_init(&projIter, e->projectiles);
    projectileEntity *projectile;
    while (cc_array_iter_next(&projIter, (void **)&projectile) != CC_ITER_END) {
        if (projectile->needsToBeDestroyed) {
            continue;
        }
        const float maxDistance = projectile->weaponInfo->maxDistance;
        const float distance = fsLength(fsSub(projectile->pos, projectile->lastPos));
        projectile->distance += distance;

        if (projectile->numDronesBehindWalls != 0) {
            bool destroyed = false;
            for (uint8_t i = 0; i < projectile->numDronesBehindWalls; i++) {
                const uint8_t droneIdx = projectile->dronesBehindWalls[i];
                const droneEntity *drone = safe_array_get_at(e->drones, droneIdx);
                if (posBehindWall(e, projectile->pos, drone->pos, NULL, 0, WALL_SHAPE | FLOATING_WALL_SHAPE, NULL)) {
                    continue;
                }

                destroyProjectile(e, projectile, true, false);
                enum cc_stat res = cc_array_iter_remove_fast(&projIter, NULL);
                MAYBE_UNUSED(res);
                ASSERT(res == CC_OK);
                destroyed = true;
                break;
            }
            if (destroyed) {
                continue;
            }
        }

        if (projectile->entsInBlackHole != NULL) {
            handleBlackHolePull(e, projectile);
        }

        if (maxDistance == INFINITE) {
            continue;
        }
        if (projectile->distance >= maxDistance) {
            // we have to destroy the projectile using the iterator so
            // we can continue to iterate correctly
            destroyProjectile(e, projectile, true, false);
            enum cc_stat res = cc_array_iter_remove_fast(&projIter, NULL);
            MAYBE_UNUSED(res);
            ASSERT(res == CC_OK);
            continue;
        }
    }

    destroyExplodedProjectiles(e);
}

void weaponPickupsStep(iwEnv *e) {
    CC_ArrayIter iter;
    cc_array_iter_init(&iter, e->pickups);
    weaponPickupEntity *pickup;

    // respawn weapon pickups at a random location as a random weapon type
    // once the respawn wait has elapsed
    while (cc_array_iter_next(&iter, (void **)&pickup) != CC_ITER_END) {
        if (pickup->respawnWait == 0.0f) {
            continue;
        }
        pickup->respawnWait = max(pickup->respawnWait - e->deltaTime, 0.0f);
        if (pickup->respawnWait != 0.0f) {
            continue;
        }

        fsVec2 pos;
        if (!findOpenPos(e, WEAPON_PICKUP_SHAPE, &pos, -1)) {
            const enum cc_stat res = cc_array_iter_remove_fast(&iter, NULL);
            MAYBE_UNUSED(res);
            ASSERT(res == CC_OK);
            DEBUG_LOG("destroying weapon pickup");
            destroyWeaponPickup(e, pickup);
            continue;
        }
        pickup->pos = pos;
        pickup->weapon = randWeaponPickupType(e);

        const int16_t cellIdx = entityPosToCellIdx(e, pos);
        if (cellIdx == -1) {
            ERRORF("invalid position for weapon pickup spawn: (%f, %f)", pos.x, pos.y);
        }
        DEBUG_LOGF("respawned weapon pickup at cell %d (%f, %f)", cellIdx, pos.x, pos.y);
        pickup->mapCellIdx = cellIdx;
        createWeaponPickupBodyShape(e, pickup);

        mapCell *cell = safe_array_get_at(e->cells, cellIdx);
        cell->ent = pickup->ent;
    }
}

void handleBodyMoveEvents(iwEnv *e) {
    for (int i = 0; i < MAX_BODIES; i++) {
        fsBody *b = &e->world.bodies[i];
        if (!b->isActive || b->isStatic) continue;

        entity *ent = b->userData;
        if (ent == NULL) continue;

        fsVec2 newPos = b->pos;
        int16_t mapIdx;

        switch (ent->type) {
        case STANDARD_WALL_ENTITY:
        case BOUNCY_WALL_ENTITY:
        case DEATH_WALL_ENTITY: {
            wallEntity *wall = ent->entity;
            mapIdx = entityPosToCellIdx(e, newPos);
            if (mapIdx == -1) {
                cc_array_remove_fast(e->floatingWalls, wall, NULL);
                destroyWall(e, wall, false);
                continue;
            }
            wall->mapCellIdx = mapIdx;
            wall->pos = newPos;
            wall->velocity = b->vel;
            break;
        }
        case PROJECTILE_ENTITY: {
            projectileEntity *proj = ent->entity;
            mapIdx = entityPosToCellIdx(e, newPos);
            if (mapIdx == -1) {
                destroyProjectile(e, proj, false, true);
                continue;
            }
            proj->mapCellIdx = mapIdx;
            proj->lastPos = proj->pos;
            proj->pos = newPos;
            proj->lastVelocity = proj->velocity;
            proj->velocity = b->vel;
            if (proj->weaponInfo->damping != 0.0f) {
                proj->lastSpeed = proj->speed;
                proj->speed = fsLength(proj->velocity);
            }
            if (e->client != NULL) {
                updateTrailPoints(&proj->trailPoints, MAX_PROJECTLE_TRAIL_POINTS, newPos);
            }
            break;
        }
        case DRONE_ENTITY: {
            droneEntity *drone = ent->entity;
            mapIdx = entityPosToCellIdx(e, newPos);
            if (mapIdx == -1) {
                killDrone(e, drone, NULL);
                continue;
            }
            drone->mapCellIdx = mapIdx;
            drone->lastPos = drone->pos;
            drone->pos = newPos;
            drone->lastVelocity = drone->velocity;
            drone->velocity = b->vel;
            if (e->client != NULL) {
                updateTrailPoints(&drone->trailPoints, MAX_DRONE_TRAIL_POINTS, newPos);
            }
            break;
        }
        case SHIELD_ENTITY: {
            shieldEntity *shield = ent->entity;
            shield->pos = newPos;
            break;
        }
        case DRONE_PIECE_ENTITY: {
            dronePieceEntity *piece = ent->entity;
            piece->pos = newPos;
            break;
        }
        default:
            break;
        }
    }
}

// destroy the projectile if it has traveled enough or has bounced enough
// times, and update drone stats if a drone was hit
uint8_t handleProjectileBeginContact(iwEnv *e, const entity *proj, const entity *ent, fsContact *contact, const bool projIsShapeA) {
    projectileEntity *projectile = proj->entity;
    projectile->contacts++;

    if (ent == NULL || ent->type == PROJECTILE_ENTITY) {
        if (projectile->weaponInfo->type == MINE_LAUNCHER_WEAPON) {
            uint8_t numDestroyed = 1;
            if (ent != NULL) {
                const projectileEntity *projectile2 = ent->entity;
                if (projectile2->weaponInfo->type == MINE_LAUNCHER_WEAPON) {
                    numDestroyed = 2;
                }
            }
            destroyProjectile(e, projectile, true, true);
            destroyExplodedProjectiles(e);
            return numDestroyed;
        }
        return false;
    } else if (entityTypeIsWall(ent->type)) {
        wallEntity *wall = ent->entity;
        if (wall->isFloating) {
            fsVec2 hitImpulse = fsMul(contact->normal, 0.1f); // Approximation since we don't have impulses yet
            applyTrackedImpulse(e, wall->body, wall->contributions, hitImpulse, projectile->droneIdx);
        }
        if (ent->type == BOUNCY_WALL_ENTITY) return false;
    } else if (ent->type == SHIELD_ENTITY) {
        shieldEntity *shield = ent->entity;
        if (shield->health <= 0.0f) return false;
        const float damage = projectile->lastSpeed * projectile->weaponInfo->mass * DRONE_SHIELD_HEALTH_IMPULSE_COEF;
        shield->health -= damage;
        if (shield->health <= 0.0f) {
            droneEntity *parentDrone = safe_array_get_at(e->drones, projectile->droneIdx);
            droneAddEnergy(parentDrone, DRONE_SHIELD_BREAK_ENERGY_REFILL);
            parentDrone->stepInfo.brokeShield[shield->drone->idx] = true;
            e->stats[parentDrone->idx].shieldsBroken++;
        }
        return false;
    }

    if (projectile->weaponInfo->type != BLACK_HOLE_WEAPON || ent->type != DRONE_ENTITY) {
        projectile->bounces++;
    }

    if (ent->type == DRONE_ENTITY) {
        droneEntity *hitDrone = ent->entity;
        fsVec2 hitImpulse = fsMul(contact->normal, 0.1f); // Approximation
        applyTrackedImpulse(e, hitDrone->body, hitDrone->contributions, hitImpulse, projectile->droneIdx);
        float hitStrength = fsLength(hitImpulse);

        if (projectile->droneIdx != hitDrone->idx) {
            droneEntity *shooterDrone = safe_array_get_at(e->drones, projectile->droneIdx);
            if (shooterDrone->team != hitDrone->team) {
                const float impulseEnergy = projectile->lastSpeed * projectile->weaponInfo->mass * projectile->weaponInfo->energyRefillCoef;
                droneAddEnergy(shooterDrone, impulseEnergy);
            }
            shooterDrone->stepInfo.shotHit[hitDrone->idx] += hitStrength;
            e->stats[shooterDrone->idx].shotsHit[projectile->weaponInfo->type]++;
            e->stats[shooterDrone->idx].totalShotsHit++;
            hitDrone->stepInfo.shotTaken[shooterDrone->idx] += hitStrength;
            e->stats[hitDrone->idx].shotsTaken[projectile->weaponInfo->type]++;
            e->stats[hitDrone->idx].totalShotsTaken++;
        } else {
            hitDrone->stepInfo.ownShotTaken = true;
            e->stats[hitDrone->idx].ownShotsTaken[projectile->weaponInfo->type]++;
            e->stats[hitDrone->idx].totalOwnShotsTaken++;
        }

        if (projectile->weaponInfo->destroyedOnDroneHit) {
            destroyProjectile(e, projectile, projectile->weaponInfo->explodesOnDroneHit, true);
            destroyExplodedProjectiles(e);
            return 1;
        }
    } else if (projectile->weaponInfo->type == MINE_LAUNCHER_WEAPON && !projectile->setMine) {
        if (isOverlappingCircleInLineOfSight(e, projectile->ent, projectile->pos, MINE_LAUNCHER_PROXIMITY_RADIUS, 0, DRONE_SHAPE, NULL)) {
            destroyProjectile(e, projectile, true, true);
            destroyExplodedProjectiles(e);
            return 1;
        }
        projectile->vel = fsVec2_zero;
        projectile->lastVelocity = fsVec2_zero;
        projectile->speed = 0.0f;
        projectile->lastSpeed = 0.0f;
        projectile->setMine = true;
        projectile->body->isStatic = true; // Stick to the wall
    }

    const uint8_t maxBounces = projectile->weaponInfo->maxBounces;
    if (projectile->bounces == maxBounces) {
        destroyProjectile(e, projectile, true, true);
        destroyExplodedProjectiles(e);
        return 1;
    }

    return 0;
}

void handleProjectileEndContact(const entity *proj, const entity *ent) {
    projectileEntity *projectile = proj->entity;
    projectile->contacts--;

    if (projectile->weaponInfo->type == MINE_LAUNCHER_WEAPON) return;

    float newSpeed = projectile->lastSpeed;
    if (projectile->weaponInfo->type == ACCELERATOR_WEAPON) {
        newSpeed = min(projectile->lastSpeed * ACCELERATOR_BOUNCE_SPEED_COEF, ACCELERATOR_MAX_SPEED);
    }

    projectile->vel = fsMul(fsNormalize(projectile->vel), newSpeed);
    projectile->speed = newSpeed;
    projectile->lastSpeed = newSpeed;
}

void handleContactEvents(iwEnv *e) {
    for (int i = 0; i < e->world.numEvents; i++) {
        const fsContactEvent *event = &e->world.events[i];
        entity *e1 = event->a->userData;
        entity *e2 = event->b->userData;
        if (e1 == NULL || e2 == NULL) continue;

        if (event->type == FS_CONTACT_BEGIN) {
            if (e1->type == PROJECTILE_ENTITY) {
                handleProjectileBeginContact(e, e1, e2, 0, true);
            } else if (e2->type == PROJECTILE_ENTITY) {
                handleProjectileBeginContact(e, e2, e1, 0, false);
            } else if (e1->type == DRONE_ENTITY && e2->type == DEATH_WALL_ENTITY) {
                killDrone(e, e1->entity, e2->entity);
            } else if (e2->type == DRONE_ENTITY && e1->type == DEATH_WALL_ENTITY) {
                killDrone(e, e2->entity, e1->entity);
            } else if (e1->type == WEAPON_PICKUP_ENTITY) {
                handleWeaponPickupBeginTouch(e, e1, e2);
            } else if (e2->type == WEAPON_PICKUP_ENTITY) {
                handleWeaponPickupBeginTouch(e, e2, e1);
            } else if (e1->type == PROJECTILE_ENTITY && event->a->isSensor) {
                 handleProjectileBeginTouch(e, e1, e2);
            } else if (e2->type == PROJECTILE_ENTITY && event->b->isSensor) {
                 handleProjectileBeginTouch(e, e2, e1);
            }
        } else {
            if (e1->type == PROJECTILE_ENTITY) {
                handleProjectileEndContact(e1, e2);
                if (event->a->isSensor) handleProjectileEndTouch(e, e1, e2);
            } else if (e2->type == PROJECTILE_ENTITY) {
                handleProjectileEndContact(e2, e1);
                if (event->b->isSensor) handleProjectileEndTouch(e, e2, e1);
            } else if (e1->type == WEAPON_PICKUP_ENTITY) {
                handleWeaponPickupEndTouch(e1, e2);
            } else if (e2->type == WEAPON_PICKUP_ENTITY) {
                handleWeaponPickupEndTouch(e2, e1);
            }
        }
    }
    e->world.numEvents = 0;
}

void handleSensorEvents(iwEnv *e) {
    // Sensors are handled as contacts in FastSim
}
// set pickup to respawn somewhere else randomly if a drone touched it,
// mark the pickup as disabled if a floating wall is touching it
void handleWeaponPickupBeginTouch(iwEnv *e, const entity *sensor, entity *visitor) {
    weaponPickupEntity *pickup = sensor->entity;
    if (pickup->floatingWallsTouching != 0) {
        return;
    }

    wallEntity *wall;

    switch (visitor->type) {
    case DRONE_ENTITY:
        disableWeaponPickup(e, pickup);

        droneEntity *drone = visitor->entity;
        drone->stepInfo.pickedUpWeapon = true;
        drone->stepInfo.prevWeapon = drone->weaponInfo->type;
        droneChangeWeapon(e, drone, pickup->weapon);

        e->stats[drone->idx].weaponsPickedUp[pickup->weapon]++;
        e->stats[drone->idx].totalWeaponsPickedUp++;
        DEBUG_LOGF("drone %d picked up weapon %d", drone->idx, pickup->weapon);
        break;
    case STANDARD_WALL_ENTITY:
    case BOUNCY_WALL_ENTITY:
    case DEATH_WALL_ENTITY:
        wall = visitor->entity;
        if (!wall->isFloating) {
            if (!wall->isSuddenDeath) {
                ERRORF("non sudden death wall type %d at cell %d touched weapon pickup", visitor->type, wall->mapCellIdx);
            }
            return;
        }

        pickup->floatingWallsTouching++;
        break;
    default:
        ERRORF("invalid weapon pickup begin touch visitor %d", visitor->type);
    }
}

// explode proximity detonating projectiles
void handleProjectileBeginTouch(iwEnv *e, const entity *sensor, entity *visitor) {
    projectileEntity *projectile = sensor->entity;

    switch (projectile->weaponInfo->type) {
    case FLAK_CANNON_WEAPON:
        if (projectile->distance < FLAK_CANNON_SAFE_DISTANCE) {
            return;
        }
        destroyProjectile(e, projectile, true, true);
        destroyExplodedProjectiles(e);
        break;
    case MINE_LAUNCHER_WEAPON:
        if (!projectile->setMine) {
            return;
        }

        ASSERT(visitor->type == DRONE_ENTITY);
        const fsDistanceOutput output = closestPoint(sensor, visitor);
        if (posBehindWall(e, projectile->pos, fsAdd(projectile->pos, fsMul(output.normal, output.distance)), visitor, 0, WALL_SHAPE | FLOATING_WALL_SHAPE, NULL)) {
            const droneEntity *drone = visitor->entity;
            projectile->dronesBehindWalls[projectile->numDronesBehindWalls++] = drone->idx;
            return;
        }

        destroyProjectile(e, projectile, true, true);
        destroyExplodedProjectiles(e);
        break;
    case BLACK_HOLE_WEAPON:
        if (visitor->type == DRONE_ENTITY) {
            const droneEntity *drone = visitor->entity;
            if (projectile->droneIdx == drone->idx && projectile->distance < BLACK_HOLE_PARENT_IGNORE_DISTANCE) {
                return;
            }
        }

        // copy the entity ID so it won't be changed if the entity is
        // destroyed and reused later
        const entityID *visitorID = visitor->id;
        entityID *id = NULL;
        if (cc_array_size(e->entityIdPool) > 0) {
            cc_array_remove_last(e->entityIdPool, (void **)&id);
            memset(id, 0, sizeof(entityID));
        } else {
            id = fastCalloc(1, sizeof(entityID));
        }
        id->id = visitorID->id;
        id->generation = visitorID->generation;
        cc_array_add(projectile->entsInBlackHole, id);
        break;
    default:
        ERRORF("invalid projectile type %d for begin touch event", sensor->type);
    }
}

// mark the pickup as enabled if no floating walls are touching it
void handleWeaponPickupEndTouch(const entity *sensor, entity *visitor) {
    weaponPickupEntity *pickup = sensor->entity;
    if (pickup->respawnWait != 0.0f) {
        return;
    }

    wallEntity *wall;

    switch (visitor->type) {
    case DRONE_ENTITY:
        break;
    case STANDARD_WALL_ENTITY:
    case BOUNCY_WALL_ENTITY:
    case DEATH_WALL_ENTITY:
        wall = visitor->entity;
        if (!wall->isFloating) {
            return;
        }

        pickup->floatingWallsTouching--;
        break;
    default:
        ERRORF("invalid weapon pickup end touch visitor %d", visitor->type);
    }
}

void handleProjectileEndTouch(iwEnv *e, const entity *sensor, entity *visitor) {
    projectileEntity *projectile = sensor->entity;

    switch (projectile->weaponInfo->type) {
    case FLAK_CANNON_WEAPON:
        break;
    case MINE_LAUNCHER_WEAPON:
        if (projectile->numDronesBehindWalls == 0) {
            return;
        }
        projectile->numDronesBehindWalls--;
        break;
    case BLACK_HOLE_WEAPON:
        if (visitor == NULL) {
            return;
        }

        const entityID *visitorID = visitor->id;
        for (uint8_t i = 0; i < cc_array_size(projectile->entsInBlackHole); ++i) {
            entityID *id = safe_array_get_at(projectile->entsInBlackHole, i);
            if (id->id == visitorID->id) {
                cc_array_add(e->entityIdPool, id);
                cc_array_remove_fast_at(projectile->entsInBlackHole, i, NULL);
                return;
            }
        }
        break;
    default:
        ERRORF("invalid projectile type %d for end touch event", projectile->weaponInfo->type);
    }
}

void findNearWalls(const iwEnv *e, const droneEntity *drone, nearEntity nearestWalls[], const uint8_t nWalls) {
    nearEntity nearWalls[MAX_NEAREST_WALLS];

    for (uint8_t i = 0; i < MAX_NEAREST_WALLS; ++i) {
        const uint32_t idx = (MAX_NEAREST_WALLS * drone->mapCellIdx) + i;
        const uint16_t wallIdx = e->map->nearestWalls[idx].idx;
        wallEntity *wall = safe_array_get_at(e->walls, wallIdx);
        nearWalls[i].entity = wall;
        nearWalls[i].distanceSquared = fsDistanceSq(drone->pos, wall->pos);
    }
    insertionSort(nearWalls, MAX_NEAREST_WALLS);
    memcpy(nearestWalls, nearWalls, nWalls * sizeof(nearEntity));
}

void dampTrackedPhysics(iwEnv *e) {
    for (uint8_t i = 0; i < e->numDrones; i++) {
        droneEntity *drone = safe_array_get_at(e->drones, i);
        if (drone->dead) {
            continue;
        }

        float droneDamping = DRONE_LINEAR_DAMPING;
        if (drone->braking) {
            droneDamping *= DRONE_BRAKE_DAMPING_COEF;
        }

        const float damp = 1.0f / (1.0f + (droneDamping * e->deltaTime));
        for (uint8_t k = 0; k < e->numDrones; k++) {
            drone->contributions[k] = fsMul(damp, drone->contributions[k]);
        }
    }

    CC_ArrayIter wallIter;
    cc_array_iter_init(&wallIter, e->floatingWalls);
    wallEntity *wall;
    while (cc_array_iter_next(&wallIter, (void **)&wall) != CC_ITER_END) {
        const float damp = 1.0f / (1.0f + (FLOATING_WALL_DAMPING * e->deltaTime));
        for (uint8_t k = 0; k < e->numDrones; k++) {
            wall->contributions[k] = fsMul(damp, wall->contributions[k]);
        }
    }
}

#endif

