#ifndef IMPULSE_WARS_ENV_H
#define IMPULSE_WARS_ENV_H

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

double lastFrameTime = 0.0;
double accumulator = 0.0;
#endif

#include "game.h"
#include "map.h"
#include "render.h"
#include "scripted_agent.h"
#include "settings.h"
#include "types.h"

const uint8_t THREE_BIT_MASK = 0x7;
const uint8_t FOUR_BIT_MASK = 0xf;

// Enable aggressive observation fast-paths during training to maximize SPS.
// This intentionally reduces observation detail (leaves some fields as zero),
// but keeps observation tensor shape identical.
#ifndef IW_FAST_TRAINING_OBS
#define IW_FAST_TRAINING_OBS 1
#endif

// Select up to K nearest projectiles to an agent using direct SoA access.
// Uses min-heap approach for O(N log K) complexity.
static inline size_t selectNearestProjectiles(const fsVec2 agentPos, const ProjectileSoA *soa,
                                              projectileEntity **out, float *outDist2, const size_t K) {
    const size_t numProjectiles = soa->size;
    size_t filled = 0;
    for (size_t i = 0; i < K; i++) {
        out[i] = NULL;
        outDist2[i] = FLT_MAX;
    }

    for (size_t i = 0; i < numProjectiles; i++) {
        const float d2 = fsDistanceSq(agentPos, soa->poses[i]);

        if (filled == K && d2 >= outDist2[0]) {
            continue;
        }

        if (filled < K) {
            out[filled] = soa->entities[i];
            outDist2[filled] = d2;
            
            size_t child = filled;
            while (child > 0) {
                size_t parent = (child - 1) / 2;
                if (outDist2[parent] <= outDist2[child]) break;
                
                projectileEntity *tmpEnt = out[parent];
                out[parent] = out[child];
                out[child] = tmpEnt;
                float tmpDist = outDist2[parent];
                outDist2[parent] = outDist2[child];
                outDist2[child] = tmpDist;
                child = parent;
            }
            filled++;
        } else {
            out[0] = soa->entities[i];
            outDist2[0] = d2;
            
            size_t parent = 0;
            while (true) {
                size_t left = 2 * parent + 1;
                size_t right = 2 * parent + 2;
                size_t smallest = parent;
                
                if (left < K && outDist2[left] < outDist2[smallest]) {
                    smallest = left;
                }
                if (right < K && outDist2[right] < outDist2[smallest]) {
                    smallest = right;
                }
                
                if (smallest == parent) break;
                
                projectileEntity *tmpEnt = out[parent];
                out[parent] = out[smallest];
                out[smallest] = tmpEnt;
                float tmpDist = outDist2[parent];
                outDist2[parent] = outDist2[smallest];
                outDist2[smallest] = tmpDist;
                parent = smallest;
            }
        }
    }

    // Extraction in-place sort (insertion sort for small K is faster than heapify-all)
    for (size_t i = 1; i < filled; i++) {
        projectileEntity *keyEnt = out[i];
        float keyDist = outDist2[i];
        int j = (int)i - 1;
        while (j >= 0 && outDist2[j] < keyDist) { // Sort largest to smallest for compatibility
            out[j + 1] = out[j];
            outDist2[j + 1] = outDist2[j];
            j--;
        }
        out[j + 1] = keyEnt;
        outDist2[j + 1] = keyDist;
    }

    return filled;
}

// Optimized nearest selection for any SoA type with an entities and poses array
#define SELECT_NEAREST_SOA(agentPos, soa, out, outDist2, K) ({ \
    size_t _num = (soa)->size; \
    size_t _filled = 0; \
    for (size_t _i = 0; _i < (K); _i++) { \
        (out)[_i] = NULL; \
        (outDist2)[_i] = FLT_MAX; \
    } \
    for (size_t _i = 0; _i < _num; _i++) { \
        float _d2 = fsDistanceSq(agentPos, (soa)->poses[_i]); \
        if (_filled == (K) && _d2 >= (outDist2)[0]) continue; \
        if (_filled < (K)) { \
            (out)[_filled] = (soa)->entities[_i]; \
            (outDist2)[_filled] = _d2; \
            size_t _child = _filled; \
            while (_child > 0) { \
                size_t _parent = (_child - 1) / 2; \
                if ((outDist2)[_parent] <= (outDist2)[_child]) break; \
                void *_tmpE = (out)[_parent]; (out)[_parent] = (out)[_child]; (out)[_child] = _tmpE; \
                float _tmpD = (outDist2)[_parent]; (outDist2)[_parent] = (outDist2)[_child]; (outDist2)[_child] = _tmpD; \
                _child = _parent; \
            } \
            _filled++; \
        } else { \
            (out)[0] = (soa)->entities[_i]; (outDist2)[0] = _d2; \
            size_t _p = 0; \
            while (true) { \
                size_t _l = 2 * _p + 1; size_t _r = 2 * _p + 2; size_t _s = _p; \
                if (_l < (K) && (outDist2)[_l] < (outDist2)[_s]) _s = _l; \
                if (_r < (K) && (outDist2)[_r] < (outDist2)[_s]) _s = _r; \
                if (_s == _p) break; \
                void *_tmpE = (out)[_p]; (out)[_p] = (out)[_s]; (out)[_s] = _tmpE; \
                float _tmpD = (outDist2)[_p]; (outDist2)[_p] = (outDist2)[_s]; (outDist2)[_s] = _tmpD; \
                _p = _s; \
            } \
        } \
    } \
    for (size_t _i = 1; _i < _filled; _i++) { \
        void *_kE = (out)[_i]; float _kD = (outDist2)[_i]; int _j = (int)_i - 1; \
        while (_j >= 0 && (outDist2)[_j] < _kD) { \
            (out)[_j + 1] = (out)[_j]; (outDist2)[_j + 1] = (outDist2)[_j]; _j--; \
        } \
        (out)[_j + 1] = _kE; (outDist2)[_j + 1] = _kD; \
    } \
    _filled; \
})

// Helper to get position from wallEntity
static inline fsVec2 getWallPos(void *entity) {
    wallEntity *wall = (wallEntity *)entity;
    return wall->pos;
}

// Helper to get position from weaponPickupEntity
static inline fsVec2 getPickupPos(void *entity) {
    weaponPickupEntity *pickup = (weaponPickupEntity *)entity;
    return pickup->pos;
}

// Get cached drone-to-drone distance, computing if cache is stale
static inline float getCachedDroneDistance(iwEnv *e, uint8_t i, uint8_t j) {
    if (i == j) return 0.0f;
    if (e->droneDistanceTimestamp[i][j] == e->currentDistanceTimestamp) {
        return e->droneDistanceCache[i][j];
    }
    
    droneEntity *droneI = drone_soa_get(&e->drones, i);
    droneEntity *droneJ = drone_soa_get(&e->drones, j);
    float distSq = fsDistanceSq(droneI->pos, droneJ->pos);
    
    e->droneDistanceCache[i][j] = distSq;
    e->droneDistanceCache[j][i] = distSq;
    e->droneDistanceTimestamp[i][j] = e->currentDistanceTimestamp;
    e->droneDistanceTimestamp[j][i] = e->currentDistanceTimestamp;
    
    return distSq;
}

// Invalidate distance cache (call when drones move significantly)
static inline void invalidateDistanceCache(iwEnv *e) {
    e->currentDistanceTimestamp++;
}

// Forward declarations
void stepEnv(iwEnv *e);
void resetEnv(iwEnv *e);
void destroyEnv(iwEnv *e);
void ensureObsComputed(iwEnv *e);

// pufferlib compatibility
static inline void c_step(iwEnv *e) {
    stepEnv(e);
    ensureObsComputed(e);
}

static inline void c_reset(iwEnv *e) {
    resetEnv(e);
}

static inline void c_close(iwEnv *e) {
    destroyEnv(e);
}

// In Puffer, render() is expected to draw a frame, not just initialize rendering.
// `setupRayClient()` only creates the window/client and flags a reset; it does not
// call BeginDrawing()/renderEnv(). That manifests as a black screen in eval.
static inline void c_render(iwEnv *e) {
    ensureObsComputed(e);
    setupRayClient(e);
    if (e->client == NULL || e->map == NULL) {
        return;
    }

    // If the window is created after setupEnv() already ran, we still need a camera.
    setupEnvCamera(e);

    // Draw a frame immediately (even before the next step()).
    renderEnv(e, false, false, -1, -1);
}

// returns a cell index that is closest to pos that isn't cellIdx
uint16_t findNearestCell(const iwEnv *e, const fsVec2 pos, const uint16_t cellIdx) {
    uint16_t closestCell = cellIdx;
    float minDistance = FLT_MAX;
    const uint8_t cellCol = cellIdx / e->map->columns;
    const uint8_t cellRow = cellIdx % e->map->columns;
    for (uint8_t i = 0; i < 8; i++) {
        const int8_t newCellCol = cellCol + cellOffsets[i][0];
        if (newCellCol < 0 || newCellCol >= e->map->columns) {
            continue;
        }
        const int8_t newCellRow = cellRow + cellOffsets[i][1];
        if (newCellRow < 0 || newCellRow >= e->map->rows) {
            continue;
        }
        const int16_t newCellIdx = cellIndex(e, newCellCol, newCellRow);
        const mapCell *cell = cell_soa_get(&e->cells, newCellIdx);
        if (cell == NULL) continue;
        if (minDistance != min(minDistance, fsDistanceSq(pos, cell->pos))) {
            closestCell = newCellIdx;
        }
    }

    return closestCell;
}

// normalize a drone's ammo count, setting infinite ammo as no ammo
static inline float scaleAmmo(const iwEnv *e, const droneEntity *drone) {
    int8_t maxAmmo = weaponAmmo(e->defaultWeapon->type, drone->weaponInfo->type);
    float scaledAmmo = 0;
    if (drone->ammo != INFINITE) {
        scaledAmmo = scaleValue(drone->ammo, maxAmmo, true);
    }
    return scaledAmmo;
}

// fills a small 2D grid centered around the agent with discretized
// walls, floating walls, weapon pickups, and drone positions
void computeMapObs(iwEnv *e, const uint8_t agentIdx, const uint16_t obsStartOffset) {
    const uint8_t droneCellCol = e->drones.mapCellIdxs[agentIdx] % e->map->columns;
    const uint8_t droneCellRow = e->drones.mapCellIdxs[agentIdx] / e->map->columns;

    const int8_t obsStartCol = droneCellCol - (MAP_OBS_COLUMNS / 2);
    const int8_t startCol = max(obsStartCol, 0);
    const int8_t obsStartRow = droneCellRow - (MAP_OBS_ROWS / 2);
    const int8_t startRow = max(obsStartRow, 0);

    const int8_t obsEndCol = droneCellCol + (MAP_OBS_COLUMNS / 2);
    const int8_t endCol = min(obsEndCol, e->map->columns - 1);
    const int8_t endRow = min(droneCellRow + (MAP_OBS_ROWS / 2), e->map->rows - 1);

    const int8_t obsColOffset = startCol - obsStartCol;
    const int8_t obsRowOffset = startRow - obsStartRow;
    uint16_t startOffset = obsStartOffset;
    if (obsColOffset == 0 && obsRowOffset != 0) {
        startOffset += obsRowOffset * MAP_OBS_COLUMNS;
    } else if (obsColOffset != 0 && obsRowOffset == 0) {
        startOffset += obsColOffset;
    } else if (obsColOffset != 0 && obsRowOffset != 0) {
        startOffset += obsColOffset + (obsRowOffset * MAP_OBS_COLUMNS);
    }
    uint16_t offset = startOffset;

    if (!e->suddenDeathWallsPlaced) {
        const int8_t numCols = endCol - startCol + 1;
        for (int8_t row = startRow; row <= endRow; row++) {
            const int16_t cellIdx = cellIndex(e, startCol, row);
            memcpy(e->observations + offset, e->packedLayout + cellIdx, numCols * sizeof(uint8_t));
            offset += MAP_OBS_COLUMNS;
        }

#if IW_FAST_TRAINING_OBS
        // Training fast-path: skip dynamic entity stamping into the map grid.
        // This removes per-step loops over pickups/floating walls/drones.
        if (e->isTraining) {
            return;
        }
#endif

        const uint8_t nPickups = pickup_soa_size(&e->pickups);
        for (size_t i = 0; i < nPickups; i++) {
            const uint16_t cellIdx = e->pickups.mapCellIdxs[i];
            const uint8_t cellCol = cellIdx % e->map->columns;
            if (cellCol < startCol || cellCol > endCol) continue;
            const uint8_t cellRow = cellIdx / e->map->columns;
            if (cellRow < startRow || cellRow > endRow) continue;

            offset = startOffset + ((cellCol - startCol) + ((cellRow - startRow) * MAP_OBS_COLUMNS));
            e->observations[offset] |= 1 << 3;
        }
    } else {
        const int8_t colPadding = obsColOffset + (obsEndCol - endCol);
        for (int8_t row = startRow; row <= endRow; row++) {
            for (int8_t col = startCol; col <= endCol; col++) {
                const int16_t cellIdx = cellIndex(e, col, row);
                const mapCell *cell = cell_soa_get(&e->cells, cellIdx);
                if (cell == NULL || cell->ent == NULL) {
                    offset++; continue;
                }

                if (entityTypeIsWall(cell->ent->type)) {
                    e->observations[offset] = ((cell->ent->type + 1) & TWO_BIT_MASK) << 5;
                } else if (cell->ent->type == WEAPON_PICKUP_ENTITY) {
                    e->observations[offset] |= 1 << 3;
                }
                offset++;
            }
            offset += colPadding;
        }
    }

#if IW_FAST_TRAINING_OBS
    // Training fast-path: skip dynamic entities outside of sudden-death processing.
    if (e->isTraining) {
        return;
    }
#endif

    const uint16_t nFloating = wall_soa_size(&e->floatingWalls);
    for (size_t i = 0; i < nFloating; i++) {
        const uint16_t cellIdx = e->floatingWalls.mapCellIdxs[i];
        const uint8_t cellCol = cellIdx % e->map->columns;
        if (cellCol < startCol || cellCol > endCol) continue;
        const uint8_t cellRow = cellIdx / e->map->columns;
        if (cellRow < startRow || cellRow > endRow) continue;

        offset = startOffset + ((cellCol - startCol) + ((cellRow - startRow) * MAP_OBS_COLUMNS));
        e->observations[offset] = ((e->floatingWalls.entities[i]->type + 1) & TWO_BIT_MASK) << 5;
        e->observations[offset] |= 1 << 4;
    }

    uint8_t newDroneIdx = 1;
    uint16_t droneCells[_MAX_DRONES] = {0};
    const uint8_t nDrones = drone_soa_size(&e->drones);
    for (uint8_t i = 0; i < nDrones; i++) {
        if (i == agentIdx) continue;
        if (e->drones.livesLefts[i] == 0) continue;

        uint16_t mIdx = e->drones.mapCellIdxs[i];
        if (i != 0) {
            for (uint8_t j = 0; j < i; j++) {
                if (droneCells[j] == mIdx) {
                    mIdx = findNearestCell(e, e->drones.poses[i], mIdx);
                    break;
                }
            }
        }
        const uint8_t cellCol = mIdx % e->map->columns;
        if (cellCol < startCol || cellCol > endCol) continue;
        const uint8_t cellRow = mIdx / e->map->columns;
        if (cellRow < startRow || cellRow > endRow) continue;

        droneCells[i] = mIdx;
        offset = startOffset + ((cellCol - startCol) + ((cellRow - startRow) * MAP_OBS_COLUMNS));
        e->observations[offset] |= (newDroneIdx++ & THREE_BIT_MASK);
    }
}

// computes observations for N nearest walls, floating walls, and weapon pickups
void computeNearObs(iwEnv *e, const droneEntity *drone, const uint8_t agentIdx, const uint16_t discreteObsStart, float *continuousObs) {
    nearEntity nearWalls[NUM_NEAR_WALL_OBS];
    findNearWalls(e, drone, nearWalls, NUM_NEAR_WALL_OBS);

    uint16_t offset;

    // compute type and position of N nearest walls
    for (uint8_t i = 0; i < NUM_NEAR_WALL_OBS; i++) {
        const wallEntity *wall = nearWalls[i].entity;
        if (wall == NULL) break;

        offset = discreteObsStart + NEAR_WALL_TYPES_OBS_OFFSET + i;
        e->observations[offset] = wall->type;

        offset = NEAR_WALL_POS_OBS_OFFSET + (i * NEAR_WALL_POS_OBS_SIZE);
        const fsVec2 agentPos = e->drones.poses[agentIdx];
        const fsVec2 wallRelPos = fsSub(wall->pos, agentPos);

        continuousObs[offset++] = scaleValue(wallRelPos.x, MAX_X_POS, false);
        continuousObs[offset] = scaleValue(wallRelPos.y, MAX_Y_POS, false);
    }

    if (wall_soa_size(&e->floatingWalls) != 0) {
        wallEntity *nearestWalls[NUM_FLOATING_WALL_OBS];
        float nearestDist2[NUM_FLOATING_WALL_OBS];
        const size_t selected = SELECT_NEAREST_SOA(e->drones.poses[agentIdx], &e->floatingWalls, 
                                                  (void**)nearestWalls, nearestDist2, NUM_FLOATING_WALL_OBS);

        for (size_t i = 0; i < selected; i++) {
            const wallEntity *wall = nearestWalls[i];
            if (wall == NULL) break;

            const fsVec2 wallRelPos = fsSub(wall->pos, e->drones.poses[agentIdx]);
            const fsRot wallRot = e->floatingWalls.rots[wall->ent->soaIndex];
            const float angle = atan2f(wallRot.s, wallRot.c);

            offset = discreteObsStart + FLOATING_WALL_TYPES_OBS_OFFSET + i;
            e->observations[offset] = wall->type + 1;

            offset = FLOATING_WALL_INFO_OBS_OFFSET + (i * FLOATING_WALL_INFO_OBS_SIZE);
            continuousObs[offset++] = scaleValue(wallRelPos.x, MAX_X_POS, false);
            continuousObs[offset++] = scaleValue(wallRelPos.y, MAX_Y_POS, false);
            continuousObs[offset++] = scaleValue(angle, MAX_ANGLE, false);
            continuousObs[offset++] = scaleValue(e->floatingWalls.velocities[wall->ent->soaIndex].x, MAX_SPEED, false);
            continuousObs[offset] = scaleValue(e->floatingWalls.velocities[wall->ent->soaIndex].y, MAX_SPEED, false);
        }
    }

    if (pickup_soa_size(&e->pickups) != 0) {
        weaponPickupEntity *nearestPickups[NUM_WEAPON_PICKUP_OBS];
        float nearestDist2[NUM_WEAPON_PICKUP_OBS];
        const size_t selected = SELECT_NEAREST_SOA(e->drones.poses[agentIdx], &e->pickups, 
                                                   (void**)nearestPickups, nearestDist2, NUM_WEAPON_PICKUP_OBS);

        for (size_t i = 0; i < selected; i++) {
            const weaponPickupEntity *pickup = nearestPickups[i];
            if (pickup == NULL) break;

            offset = discreteObsStart + WEAPON_PICKUP_WEAPONS_OBS_OFFSET + i;
            e->observations[offset] = pickup->weapon + 1;

            offset = WEAPON_PICKUP_POS_OBS_OFFSET + (i * WEAPON_PICKUP_POS_OBS_SIZE);
            const fsVec2 pickupRelPos = fsSub(pickup->pos, e->drones.poses[agentIdx]);
            continuousObs[offset++] = scaleValue(pickupRelPos.x, MAX_X_POS, false);
            continuousObs[offset] = scaleValue(pickupRelPos.y, MAX_Y_POS, false);
        }
    }
}

void computeObs(iwEnv *e) {
    for (uint8_t agentIdx = 0; agentIdx < e->numAgents; agentIdx++) {
        if (!e->obsDirtyPerAgent[agentIdx]) {
            continue;
        }
        
        // if the drone is dead, only compute observations if it died
        // this step and it isn't out of bounds
        if (e->drones.livesLefts[agentIdx] == 0 && (!e->drones.diedThisSteps[agentIdx] || e->drones.mapCellIdxs[agentIdx] == -1)) {
            continue;
        }

        const uint16_t discreteObsStart = e->obsBytes * agentIdx;
        memset(e->observations + discreteObsStart, 0x0, e->obsBytes);
        computeMapObs(e, agentIdx, discreteObsStart);

#if IW_FAST_TRAINING_OBS
        // Training fast-path: computing full observations (nearest projectiles, enemy drones,
        // floating walls, etc.) is the dominant c_step cost. For maximizing SPS, we only
        // provide the (static) map grid here; the rest stays zero from memset above.
        if (e->isTraining) {
            continue;
        }
#endif

        const uint16_t continuousObsStart = discreteObsStart + e->discreteObsBytes;
        float *continuousObs = (float *)(e->observations + continuousObsStart);

        droneEntity *agentDrone = e->drones.entities[agentIdx];
        if (agentDrone == NULL) {
            continue;
        }

        uint16_t discreteObsOffset = 0;
        uint16_t continuousObsOffset = 0;

        computeNearObs(e, agentDrone, agentIdx, discreteObsStart, continuousObs);

        const uint8_t nProj = projectile_soa_size(&e->projectiles);
        if (nProj > 0) {
            projectileEntity *nearest[NUM_PROJECTILE_OBS];
            float nearestDist2[NUM_PROJECTILE_OBS];
            const size_t selected = selectNearestProjectiles(e->drones.poses[agentIdx], &e->projectiles,
                                                             nearest, nearestDist2, NUM_PROJECTILE_OBS);

            for (size_t i = 0; i < selected; i++) {
                const projectileEntity *projectile = nearest[i];
                if (projectile == NULL) break;
                const uint16_t soaIdx = projectile->ent->soaIndex;

                discreteObsOffset = discreteObsStart + PROJECTILE_DRONE_OBS_OFFSET + i;
                e->observations[discreteObsOffset] = projectile->droneIdx + 1;

                discreteObsOffset = discreteObsStart + PROJECTILE_WEAPONS_OBS_OFFSET + i;
                e->observations[discreteObsOffset] = e->projectiles.weaponInfos[soaIdx]->type + 1;

                continuousObsOffset = PROJECTILE_INFO_OBS_OFFSET + (i * PROJECTILE_INFO_OBS_SIZE);
                const fsVec2 projectileRelPos = fsSub(e->projectiles.poses[soaIdx], e->drones.poses[agentIdx]);
                continuousObs[continuousObsOffset++] = scaleValue(projectileRelPos.x, MAX_X_POS, false);
                continuousObs[continuousObsOffset++] = scaleValue(projectileRelPos.y, MAX_Y_POS, false);
                continuousObs[continuousObsOffset++] = scaleValue(e->projectiles.velocities[soaIdx].x, MAX_SPEED, false);
                continuousObs[continuousObsOffset] = scaleValue(e->projectiles.velocities[soaIdx].y, MAX_SPEED, false);
            }
        }

        // compute enemy drone observations
        bool hitShot = false;
        bool tookShot = false;
        uint8_t processedDrones = 0;
        for (uint8_t i = 0; i < e->numDrones; i++) {
            if (i == agentIdx) {
                continue;
            }

            if (agentDrone->stepInfo.shotHit[i] != 0.0f) {
                hitShot = true;
            }
            if (agentDrone->stepInfo.shotTaken[i] != 0.0f) {
                tookShot = true;
            }

            droneEntity *enemyDrone = e->drones.entities[i];
            if (enemyDrone->livesLeft == 0) {
                processedDrones++;
                continue;
            }

            const fsVec2 enemyDroneRelPos = fsSub(enemyDrone->pos, agentDrone->pos);
            // Use cached distance
            const float enemyDroneDistanceSquared = getCachedDroneDistance(e, agentIdx, i);
            const fsVec2 enemyDroneAccel = fsSub(enemyDrone->velocity, enemyDrone->lastVelocity);
            const fsVec2 enemyDroneRelNormPos = fsNormalize(fsSub(enemyDrone->pos, agentDrone->pos));
            const float enemyDroneAimAngle = atan2f(enemyDrone->lastAim.y, enemyDrone->lastAim.x);
            float enemyDroneBraking = 0.0f;
            if (enemyDrone->braking) {
                enemyDroneBraking = 1.0f;
            }

            discreteObsOffset = discreteObsStart + ENEMY_DRONE_WEAPONS_OBS_OFFSET + processedDrones;
            e->observations[discreteObsOffset] = enemyDrone->weaponInfo->type + 1;

            continuousObsOffset = ENEMY_DRONE_OBS_OFFSET + (e->numDrones - 1) + (processedDrones * ENEMY_DRONE_OBS_SIZE);
            continuousObs[continuousObsOffset++] = enemyDrone->team == agentDrone->team;
            continuousObs[continuousObsOffset++] = scaleValue(enemyDroneRelPos.x, MAX_X_POS, false);
            continuousObs[continuousObsOffset++] = scaleValue(enemyDroneRelPos.y, MAX_Y_POS, false);
            continuousObs[continuousObsOffset++] = scaleValue(enemyDroneDistanceSquared, SQUARED(MAX_DISTANCE), true);
            continuousObs[continuousObsOffset++] = scaleValue(enemyDrone->velocity.x, MAX_SPEED, false);
            continuousObs[continuousObsOffset++] = scaleValue(enemyDrone->velocity.y, MAX_SPEED, false);
            continuousObs[continuousObsOffset++] = scaleValue(enemyDroneAccel.x, MAX_ACCEL, false);
            continuousObs[continuousObsOffset++] = scaleValue(enemyDroneAccel.y, MAX_ACCEL, false);
            continuousObs[continuousObsOffset++] = scaleValue(enemyDroneRelNormPos.x, 1.0f, false);
            continuousObs[continuousObsOffset++] = scaleValue(enemyDroneRelNormPos.y, 1.0f, false);
            continuousObs[continuousObsOffset++] = scaleValue(enemyDrone->lastAim.x, 1.0f, false);
            continuousObs[continuousObsOffset++] = scaleValue(enemyDrone->lastAim.y, 1.0f, false);
            continuousObs[continuousObsOffset++] = scaleValue(enemyDroneAimAngle, PI, false);
            continuousObs[continuousObsOffset++] = scaleAmmo(e, enemyDrone);
            continuousObs[continuousObsOffset++] = scaleValue(enemyDrone->weaponCooldown, enemyDrone->weaponInfo->coolDown, true);
            continuousObs[continuousObsOffset++] = scaleValue(enemyDrone->weaponCharge, enemyDrone->weaponInfo->charge, true);
            continuousObs[continuousObsOffset++] = scaleValue(enemyDrone->energyLeft, DRONE_ENERGY_MAX, true);
            continuousObs[continuousObsOffset++] = (float)enemyDrone->energyFullyDepleted;
            continuousObs[continuousObsOffset++] = enemyDroneBraking;
            continuousObs[continuousObsOffset++] = scaleValue(enemyDrone->burstCooldown, DRONE_BURST_COOLDOWN, true);
            continuousObs[continuousObsOffset++] = (float)enemyDrone->chargingBurst;
            continuousObs[continuousObsOffset++] = scaleValue(enemyDrone->burstCharge, DRONE_ENERGY_MAX, true);
            continuousObs[continuousObsOffset++] = scaleValue(enemyDrone->livesLeft, DRONE_LIVES, true);
            continuousObs[continuousObsOffset++] = !enemyDrone->dead;

            processedDrones++;
            ASSERTF(continuousObsOffset == ENEMY_DRONE_OBS_OFFSET + (e->numDrones - 1) + (processedDrones * ENEMY_DRONE_OBS_SIZE), "offset: %d", continuousObsOffset);
        }

        // compute active drone observations
        continuousObsOffset = ENEMY_DRONE_OBS_OFFSET + ((e->numDrones - 1) * ENEMY_DRONE_OBS_SIZE);
        const fsVec2 agentDroneAccel = fsSub(agentDrone->velocity, agentDrone->lastVelocity);
        float agentDroneBraking = 0.0f;
        if (agentDrone->braking) {
            agentDroneBraking = 1.0f;
        }

        discreteObsOffset = discreteObsStart + ENEMY_DRONE_WEAPONS_OBS_OFFSET + e->numDrones - 1;
        e->observations[discreteObsOffset] = agentDrone->weaponInfo->type + 1;

        continuousObs[continuousObsOffset++] = scaleValue(agentDrone->pos.x, MAX_X_POS, false);
        continuousObs[continuousObsOffset++] = scaleValue(agentDrone->pos.y, MAX_Y_POS, false);
        continuousObs[continuousObsOffset++] = scaleValue(agentDrone->velocity.x, MAX_SPEED, false);
        continuousObs[continuousObsOffset++] = scaleValue(agentDrone->velocity.y, MAX_SPEED, false);
        continuousObs[continuousObsOffset++] = scaleValue(agentDroneAccel.x, MAX_ACCEL, false);
        continuousObs[continuousObsOffset++] = scaleValue(agentDroneAccel.y, MAX_ACCEL, false);
        continuousObs[continuousObsOffset++] = scaleValue(agentDrone->lastAim.x, 1.0f, false);
        continuousObs[continuousObsOffset++] = scaleValue(agentDrone->lastAim.y, 1.0f, false);
        continuousObs[continuousObsOffset++] = scaleAmmo(e, agentDrone);
        continuousObs[continuousObsOffset++] = scaleValue(agentDrone->weaponCooldown, agentDrone->weaponInfo->coolDown, true);
        continuousObs[continuousObsOffset++] = scaleValue(agentDrone->weaponCharge, agentDrone->weaponInfo->charge, true);
        continuousObs[continuousObsOffset++] = scaleValue(agentDrone->energyLeft, DRONE_ENERGY_MAX, true);
        continuousObs[continuousObsOffset++] = (float)agentDrone->energyFullyDepleted;
        continuousObs[continuousObsOffset++] = agentDroneBraking;
        continuousObs[continuousObsOffset++] = scaleValue(agentDrone->burstCooldown, DRONE_BURST_COOLDOWN, true);
        continuousObs[continuousObsOffset++] = (float)agentDrone->chargingBurst;
        continuousObs[continuousObsOffset++] = scaleValue(agentDrone->burstCharge, DRONE_ENERGY_MAX, true);
        continuousObs[continuousObsOffset++] = hitShot;
        continuousObs[continuousObsOffset++] = tookShot;
        continuousObs[continuousObsOffset++] = agentDrone->stepInfo.ownShotTaken;
        continuousObs[continuousObsOffset++] = scaleValue(agentDrone->livesLeft, DRONE_LIVES, true);
        continuousObs[continuousObsOffset++] = !agentDrone->dead;

        ASSERTF(continuousObsOffset == ENEMY_DRONE_OBS_OFFSET + ((e->numDrones - 1) * ENEMY_DRONE_OBS_SIZE) + DRONE_OBS_SIZE, "offset: %d", continuousObsOffset);
        continuousObs[continuousObsOffset] = scaleValue(e->stepsLeft, e->totalSteps, true);
    }
}

// Lazy observation computation: only compute when observations are actually needed
// Now uses per-agent dirty flags for finer-grained control
void ensureObsComputed(iwEnv *e) {
    if (!e->obsDirty) {
        return;
    }
    
    // Check if any agent needs observations
    bool anyDirty = false;
    for (uint8_t i = 0; i < e->numAgents; i++) {
        if (e->obsDirtyPerAgent[i]) {
            anyDirty = true;
            break;
        }
    }
    
    if (!anyDirty) {
        e->obsDirty = false;
        return;
    }
    
    computeObs(e);
    
    // Clear dirty flags for computed agents
    for (uint8_t i = 0; i < e->numAgents; i++) {
        e->obsDirtyPerAgent[i] = 0;
    }
    e->obsDirty = false;
}

void setupEnv(iwEnv *e) {
    e->needsReset = false;

    e->stepsLeft = e->totalSteps;
    e->suddenDeathSteps = e->totalSuddenDeathSteps;
    e->suddenDeathWallCounter = 0;

    e->lastSpawnQuad = -1;

    int8_t mapIdx = e->pinnedMapIdx;
    if (e->pinnedMapIdx == -1) {
        uint8_t firstMap = 0;
        // don't evaluate on the boring empty map
        if (!e->isTraining) {
            firstMap = 1;
        }
        mapIdx = randInt(&e->randState, firstMap, NUM_MAPS - 1);
    }
    DEBUG_LOGF("setting up map %d", mapIdx);
    setupMap(e, mapIdx);

    // Allocate per-environment map data
    const uint8_t columns = maps[mapIdx]->columns;
    const uint8_t rows = maps[mapIdx]->rows;
    const uint32_t numCells = (uint32_t)columns * rows;

    // Free previous allocations if switching maps
    if (e->droneSpawns != NULL) fastFree(e->droneSpawns);
    if (e->packedLayout != NULL) fastFree(e->packedLayout);
    if (e->nearestWalls != NULL) fastFree(e->nearestWalls);

    e->droneSpawns = fastCalloc(numCells, sizeof(bool));
    e->packedLayout = fastCalloc(numCells, sizeof(uint8_t));
    e->nearestWalls = fastCalloc(MAX_NEAREST_WALLS * numCells, sizeof(nearEntity));

    // Initialize defaults
    for (uint32_t i = 0; i < numCells; i++) {
        e->droneSpawns[i] = true;
        e->packedLayout[i] = 0;
    }
    for (uint32_t i = 0; i < (uint32_t)MAX_NEAREST_WALLS * numCells; i++) {
        e->nearestWalls[i].entity = NULL;
    }

    DEBUG_LOG("creating drones");
    for (uint8_t i = 0; i < e->numDrones; i++) {
        createDrone(e, i);
    }

    DEBUG_LOG("placing floating walls");
    placeRandFloatingWalls(e, mapIdx);

    DEBUG_LOG("creating weapon pickups");
    // start spawning pickups in a random quadrant
    e->lastSpawnQuad = randInt(&e->randState, 0, 3);
    for (uint8_t i = 0; i < maps[mapIdx]->weaponPickups; i++) {
        createWeaponPickup(e);
    }

    if (e->client != NULL) {
        setupEnvCamera(e);
        renderEnv(e, true, false, -1, -1);
    }

    e->obsDirty = true;
    if (e->observations != NULL) {
        ensureObsComputed(e);
    } else {
        DEBUG_LOG("Skipping computeObs: Observation buffer not bound by Python yet.");
    }
}

// sets the timing related variables for the environment depending on
// the frame rate
void setEnvFrameRate(iwEnv *e) {
    float frameRate = TRAINING_FRAME_RATE;
    e->physicsSubSteps = TRAINING_BOX2D_SUBSTEPS;
    // set a higher frame rate and physics substeps when evaluating
    // to make it more enjoyable to play
    if (!e->isTraining) {
        frameRate = EVAL_FRAME_RATE;
        e->physicsSubSteps = EVAL_BOX2D_SUBSTEPS;
    }

    e->frameRate = frameRate;
    // Performance note:
    // For training we run one physics step per action to maximize SPS (c_step/sec).
    // This reduces the number of fsWorld_Step() calls per c_step by ~2x (previously frameSkip=2).
    if (e->isTraining) {
        frameRate = (float)TRAINING_ACTIONS_PER_SECOND;
        e->frameRate = frameRate;
        e->deltaTime = 1.0f / frameRate;
        e->frameSkip = 1;
    } else {
        e->deltaTime = 1.0f / (float)frameRate;
        e->frameSkip = frameRate / TRAINING_ACTIONS_PER_SECOND;
    }

    e->totalSteps = ROUND_STEPS * frameRate;
    e->totalSuddenDeathSteps = SUDDEN_DEATH_STEPS * frameRate;
}

iwEnv *initEnv(iwEnv *e, uint8_t numDrones, uint8_t numAgents, int8_t mapIdx, uint64_t seed, bool enableTeams, bool sittingDuck, bool isTraining, bool continuousActions) {
    DEBUG_LOGF("seed: %lu", seed);

    e->numDrones = numDrones;
    e->numAgents = numAgents;
    e->teamsEnabled = enableTeams;
    e->numTeams = numDrones;
    if (e->teamsEnabled) {
        e->numTeams = 2;
    }
    e->sittingDuck = sittingDuck;
    e->isTraining = isTraining;

    e->winReward = WIN_REWARD;
    e->selfKillPunishment = SELF_KILL_PUNISHMENT;
    e->enemyDeathReward = ENEMY_DEATH_REWARD;
    e->enemyKillReward = ENEMY_KILL_REWARD;
    e->teammateDeathPunishment = TEAMMATE_DEATH_PUNISHMENT;
    e->teammateKillPunishment = TEAMMATE_KILL_PUNISHMENT;
    e->deathPunishment = DEATH_PUNISHMENT;
    e->energyEmptiedPunishment = ENERGY_EMPTY_PUNISHMENT;
    e->weaponPickupReward = WEAPON_PICKUP_REWARD;
    e->shieldBreakReward = SHIELD_BREAK_REWARD;
    e->shotHitRewardCoef = SHOT_HIT_REWARD_COEF;
    e->explosionHitRewardCoef = EXPLOSION_HIT_REWARD_COEF;

    e->obsBytes = obsBytes(e->numDrones);
    e->discreteObsBytes = alignedSize(discreteObsSize(e->numDrones) * sizeof(uint8_t), sizeof(float));

    e->continuousActions = continuousActions;
    e->obsDirty = true;

    // TODO: remove when puffer bindings add truncations
    e->truncations = fastCalloc(numDrones, sizeof(uint8_t));

    setEnvFrameRate(e);
    e->randState = seed;
    e->needsReset = false;

    fsWorld_Init(&e->world, (fsVec2){0.0f, 0.0f});
    e->pinnedMapIdx = mapIdx;
    e->mapIdx = -1;

    // Initialize per-environment map data to NULL (allocated in setupEnv)
    e->droneSpawns = NULL;
    e->packedLayout = NULL;
    e->nearestWalls = NULL;

    e->mapPathing = fastCalloc(NUM_MAPS, sizeof(pathingInfo));
    for (uint8_t i = 0; i < NUM_MAPS; i++) {
        const mapEntry *map = maps[i];
        pathingInfo *info = &e->mapPathing[i];
        info->paths = fastMalloc(map->rows * map->columns * map->rows * map->columns * sizeof(uint8_t));
        memset(info->paths, UINT8_MAX, map->rows * map->columns * map->rows * map->columns * sizeof(uint8_t));
        info->pathBuffer = fastCalloc(3 * 8 * map->rows * map->columns, sizeof(int8_t));
    }

    // Initialize SoA arrays (zero allocation, just zero active flags)
    init_soa_arrays(e);

    // Initialize distance cache
    memset(e->droneDistanceCache, 0, sizeof(e->droneDistanceCache));
    memset(e->droneDistanceTimestamp, 0, sizeof(e->droneDistanceTimestamp));
    e->currentDistanceTimestamp = 1;
    
    // Initialize per-agent dirty flags
    memset(e->obsDirtyPerAgent, 0, sizeof(e->obsDirtyPerAgent));

    // Create CC_Array for entities (still needed for entity management)
    create_array(&e->entities, 256);

    // Create pools (still use CC_Array for pooling)
    create_array(&e->projectilePool, 128);
    create_array(&e->dronePiecePool, 128);
    create_array(&e->explosionPool, 32);
    create_array(&e->wallPool, 64);
    create_array(&e->pickupPool, 32);
    create_array(&e->dronePool, 8);

    e->humanInput = false;
    e->humanDroneInput = 0;
    e->connectedControllers = 0;

#ifndef NDEBUG
    create_array(&e->debugPoints, 4);
#endif

    return e;
}

void setRewards(iwEnv *e, float winReward, float selfKillPunishment, float enemyDeathReward, float enemyKillReward, float teammateDeathPunishment, float teammateKillPunishment, float deathPunishment, float energyEmptiedPunishment, float weaponPickupReward, float shieldBreakReward, float shotHitRewardCoef, float explosionHitRewardCoef) {
    e->winReward = winReward;
    e->selfKillPunishment = selfKillPunishment;
    e->enemyDeathReward = enemyDeathReward;
    e->enemyKillReward = enemyKillReward;
    e->teammateDeathPunishment = teammateDeathPunishment;
    e->teammateKillPunishment = teammateKillPunishment;
    e->deathPunishment = deathPunishment;
    e->energyEmptiedPunishment = energyEmptiedPunishment;
    e->weaponPickupReward = weaponPickupReward;
    e->shieldBreakReward = shieldBreakReward;
    e->shotHitRewardCoef = shotHitRewardCoef;
    e->explosionHitRewardCoef = explosionHitRewardCoef;
}

void clearEnv(iwEnv *e) {
    // Clear physics contact events first to prevent use-after-free
    memset(e->world.events, 0, sizeof(e->world.events));
    e->world.numEvents = 0;
    
    // Clear user data to prevent stale entity pointers
    memset(e->world.userDatas, 0, sizeof(e->world.userDatas));

    // rewards get cleared in stepEnv every step
    // memset(e->masks, 1, e->numAgents * sizeof(uint8_t));
    memset(e->terminals, 0x0, e->numAgents * sizeof(float));
    memset(e->truncations, 0x0, e->numAgents * sizeof(uint8_t));

    e->episodeLength = 0;
    memset(e->stats, 0x0, sizeof(e->stats));

    // Free per-environment map data
    if (e->droneSpawns != NULL) {
        fastFree(e->droneSpawns);
        e->droneSpawns = NULL;
    }
    if (e->packedLayout != NULL) {
        fastFree(e->packedLayout);
        e->packedLayout = NULL;
    }
    if (e->nearestWalls != NULL) {
        fastFree(e->nearestWalls);
        e->nearestWalls = NULL;
    }

    // Destroy projectiles first to prevent them from referencing freed drones
    for (size_t i = 0; i < projectile_soa_size(&e->projectiles); i++) {
        projectileEntity *p = projectile_soa_get(&e->projectiles, i);
        if (p == NULL) continue;
        destroyProjectile(e, p, false, false);
    }

    for (uint8_t i = 0; i < drone_soa_size(&e->drones); i++) {
        droneEntity *drone = drone_soa_get(&e->drones, i);
        if (drone == NULL) continue;
        destroyDrone(e, drone);
    }
    drone_soa_remove_all(&e->drones);

    for (size_t i = 0; i < wall_soa_size(&e->floatingWalls); i++) {
        wallEntity *wall = wall_soa_get(&e->floatingWalls, i);
        if (wall == NULL) continue;
        destroyWall(e, wall, false);
    }

    for (size_t i = 0; i < pickup_soa_size(&e->pickups); i++) {
        weaponPickupEntity *pickup = pickup_soa_get(&e->pickups, i);
        if (pickup == NULL) continue;
        destroyWeaponPickup(e, pickup);
    }

    for (size_t i = 0; i < projectile_soa_size(&e->projectiles); i++) {
        projectileEntity *p = projectile_soa_get(&e->projectiles, i);
        if (p == NULL) continue;
        destroyProjectile(e, p, false, false);
    }

    for (size_t i = 0; i < explosion_soa_size(&e->explosions); i++) {
        explosionInfo *explosion = explosion_soa_get(&e->explosions, i);
        if (explosion == NULL) continue;
        // keep around for reuse; explosions are created frequently when rendering
        cc_array_add(e->explosionPool, explosion);
    }

    for (size_t i = 0; i < drone_piece_soa_size(&e->dronePieces); i++) {
        dronePieceEntity *piece = drone_piece_soa_get(&e->dronePieces, i);
        if (piece == NULL) continue;
        destroyDronePiece(e, piece);
    }

    wall_soa_remove_all(&e->floatingWalls);
    pickup_soa_remove_all(&e->pickups);
    projectile_soa_remove_all(&e->projectiles);
    projectile_soa_remove_all(&e->explodingProjectiles);
    explosion_soa_remove_all(&e->explosions);
    drone_piece_soa_remove_all(&e->dronePieces);
    
    // Clear cell entity pointers to prevent stale references
    for (uint16_t i = 0; i < cell_soa_size(&e->cells); i++) {
        mapCell *cell = cell_soa_get(&e->cells, i);
        if (cell != NULL) {
            cell->ent = NULL;
        }
    }
}

void destroyEnv(iwEnv *e) {
    // Just clear dynamic entities - OS will reclaim all memory on process exit
    // This avoids complex cleanup logic and double-free bugs
    clearEnv(e);
}

void resetEnv(iwEnv *e) {
    clearEnv(e);
    setupEnv(e);
}

float computeReward(iwEnv *e, droneEntity *drone) {
    float reward = 0.0f;

    if (drone->energyFullyDepleted && drone->energyRefillWait == DRONE_ENERGY_REFILL_EMPTY_WAIT) {
        reward += e->energyEmptiedPunishment;
    }

    // only reward picking up a weapon if the standard weapon was
    // previously held; every weapon is better than the standard
    // weapon, but other weapons are situational better so don't
    // reward switching a non-standard weapon
    if (drone->stepInfo.pickedUpWeapon && drone->stepInfo.prevWeapon == STANDARD_WEAPON) {
        reward += e->weaponPickupReward;
    }

    for (uint8_t i = 0; i < e->numDrones; i++) {
        if (i == drone->idx) {
            continue;
        }
        droneEntity *enemyDrone = drone_soa_get(&e->drones, i);
        const bool onTeam = drone->team == enemyDrone->team;

        // TODO: punish for hitting teammates?
        if (drone->stepInfo.shotHit[i] != 0.0f && !onTeam) {
            reward += drone->stepInfo.shotHit[i] * e->shotHitRewardCoef;
        }
        if (drone->stepInfo.explosionHit[i] != 0.0f && !onTeam) {
            reward += drone->stepInfo.explosionHit[i] * e->explosionHitRewardCoef;
        }
        if (drone->stepInfo.brokeShield[i] && !onTeam) {
            reward += e->shieldBreakReward;
        }

        if (e->numAgents == e->numDrones) {
            if (drone->stepInfo.shotTaken[i] != 0) {
                reward -= drone->stepInfo.shotTaken[i] * e->shotHitRewardCoef;
            }
            if (drone->stepInfo.explosionTaken[i]) {
                reward -= drone->stepInfo.explosionTaken[i] * e->explosionHitRewardCoef;
            }
        }

        if (enemyDrone->dead && enemyDrone->diedThisStep) {
            if (!onTeam) {
                reward += e->enemyDeathReward;
                if (drone->killed[i]) {
                    reward += e->enemyKillReward;
                }
            } else {
                reward += e->teammateDeathPunishment;
                if (drone->killed[i]) {
                    reward += e->teammateKillPunishment;
                }
            }
            continue;
        }

        // const fsVec2 enemyDirection = fsNormalize(fsSub(enemyDrone->pos, drone->pos));
        // const float velocityToEnemy = fsDot(drone->lastVelocity, enemyDirection);
        // const float enemyDistance = fsDistance(enemyDrone->pos, drone->pos);
        // // stop rewarding approaching an enemy if they're very close
        // // to avoid constant clashing; always reward approaching when
        // // the current weapon is the shotgun, it greatly benefits from
        // // being close to enemies
        // if (velocityToEnemy > 0.1f && (drone->weaponInfo->type == SHOTGUN_WEAPON || enemyDistance > DISTANCE_CUTOFF)) {
        //     reward += APPROACH_REWARD;
        // }
    }

    return reward;
}

const float REWARD_EPS = 1.0e-6f;

void computeRewards(iwEnv *e, const bool roundOver, const int8_t winner, const int8_t winningTeam) {
    if (roundOver && winner != -1 && winner < e->numAgents) {
        e->rewards[winner] += e->winReward;
    }

    for (uint8_t i = 0; i < e->numDrones; i++) {
        float reward = 0.0f;
        droneEntity *drone = drone_soa_get(&e->drones, i);
        reward = computeReward(e, drone);
        if (!drone->dead && roundOver && winningTeam == drone->team) {
            reward += e->winReward;
        } else if (drone->diedThisStep) {
            reward = e->deathPunishment;
            if (drone->killedBy == drone->idx) {
                reward += e->selfKillPunishment;
            }
        }
        if (i < e->numAgents) {
            e->rewards[i] += reward;
        }
        e->stats[i].returns += reward;
    }
}

static inline bool isActionNoop(const fsVec2 action) {
    return fsLengthSq(action) < (ACTION_NOOP_MAGNITUDE * ACTION_NOOP_MAGNITUDE);
}

// Fast tanh approximation for action squashing (much cheaper than tanhf).
// Accuracy is sufficient for bounding actions to [-1, 1].
static inline float fastTanhf(float x) {
    // Clamp helps stability of the approximation for extreme inputs
    if (x > 3.0f) x = 3.0f;
    else if (x < -3.0f) x = -3.0f;
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

agentActions _computeActions(iwEnv *e, droneEntity *drone, const agentActions *manualActions) {
    agentActions actions = {0};

    const uint8_t offset = drone->idx * CONTINUOUS_ACTION_SIZE;
    if (manualActions == NULL) {
        actions.move = (fsVec2){.x = e->actions[offset + 0], .y = e->actions[offset + 1]};
        actions.aim = (fsVec2){.x = e->actions[offset + 2], .y = e->actions[offset + 3]};
        if (e->continuousActions) {
            actions.move.x = fastTanhf(actions.move.x);
            actions.move.y = fastTanhf(actions.move.y);
            actions.aim.x = fastTanhf(actions.aim.x);
            actions.aim.y = fastTanhf(actions.aim.y);
        }
        actions.chargingWeapon = e->actions[offset + 4] > 0.0f;
        actions.shoot = actions.chargingWeapon;
        if (!actions.chargingWeapon && drone->chargingWeapon) {
            actions.shoot = true;
        }
        actions.brake = e->actions[offset + 5] > 0.0f;
        actions.chargingBurst = e->actions[offset + 6] > 0.0f;
    } else {
        actions.move = manualActions->move;
        actions.aim = manualActions->aim;
        actions.chargingWeapon = manualActions->chargingWeapon;
        actions.shoot = manualActions->shoot;
        actions.brake = manualActions->brake;
        actions.chargingBurst = manualActions->chargingBurst;
        actions.discardWeapon = manualActions->discardWeapon;
    }

    // cap movement magnitude to 1.0
    const float moveLenSq = fsLengthSq(actions.move);
    if (moveLenSq > 1.0f) {
        const float invLen = 1.0f / sqrtf(moveLenSq);
        actions.move = fsMul(actions.move, invLen);
    }

    const float aimLenSq = fsLengthSq(actions.aim);
    if (aimLenSq < (ACTION_NOOP_MAGNITUDE * ACTION_NOOP_MAGNITUDE)) {
        actions.aim = fsVec2_zero;
    } else {
        // Normalize aim for consistency
        const float invLen = 1.0f / sqrtf(aimLenSq);
        actions.aim = fsMul(actions.aim, invLen);
    }

    return actions;
}

agentActions computeActions(iwEnv *e, droneEntity *drone, const agentActions *manualActions) {
    const agentActions actions = _computeActions(e, drone, manualActions);
    drone->lastMove = actions.move;
    if (!fsVecEqual(actions.aim, fsVec2_zero)) {
        drone->lastAim = actions.aim;
    }
    return actions;
}

void updateConnectedControllers(iwEnv *e) {
    for (uint8_t i = 0; i < e->numDrones; i++) {
        if (IsGamepadAvailable(i)) {
            e->connectedControllers++;
        }
    }
}

void updateHumanInputToggle(iwEnv *e) {
    if (IsKeyPressed(KEY_LEFT_CONTROL)) {
        e->humanInput = !e->humanInput;
        if (!e->humanInput) {
            e->connectedControllers = 0;
        }
    }
    if (e->humanInput && e->connectedControllers == 0) {
        updateConnectedControllers(e);
    }
    if (e->connectedControllers > 1) {
        e->humanDroneInput = e->numDrones - e->connectedControllers;
        return;
    }

    if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) {
        e->humanDroneInput = 0;
    }
    if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) {
        e->humanDroneInput = 1;
    }
    if (e->numDrones >= 3 && (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3))) {
        e->humanDroneInput = 2;
    }
    if (e->numDrones >= 4 && (IsKeyPressed(KEY_FOUR) || IsKeyPressed(KEY_KP_4))) {
        e->humanDroneInput = 3;
    }
}

agentActions getPlayerInputs(iwEnv *e, droneEntity *drone, uint8_t gamepadIdx) {
    if (IsKeyPressed(KEY_R)) {
        e->needsReset = true;
    }

    agentActions actions = {0};

    bool controllerConnected = false;
    if (IsGamepadAvailable(gamepadIdx)) {
        controllerConnected = true;
    }
    if (controllerConnected) {
        float lStickX = GetGamepadAxisMovement(gamepadIdx, GAMEPAD_AXIS_LEFT_X);
        float lStickY = GetGamepadAxisMovement(gamepadIdx, GAMEPAD_AXIS_LEFT_Y);
        float rStickX = GetGamepadAxisMovement(gamepadIdx, GAMEPAD_AXIS_RIGHT_X);
        float rStickY = GetGamepadAxisMovement(gamepadIdx, GAMEPAD_AXIS_RIGHT_Y);

        if (IsGamepadButtonDown(gamepadIdx, GAMEPAD_BUTTON_RIGHT_TRIGGER_2)) {
            actions.chargingWeapon = true;
            actions.shoot = true;
        } else if (drone->chargingWeapon && IsGamepadButtonUp(gamepadIdx, GAMEPAD_BUTTON_RIGHT_TRIGGER_2)) {
            actions.shoot = true;
        }

        if (IsGamepadButtonDown(gamepadIdx, GAMEPAD_BUTTON_LEFT_TRIGGER_2)) {
            actions.brake = true;
        }

        if (IsGamepadButtonDown(gamepadIdx, GAMEPAD_BUTTON_RIGHT_TRIGGER_1) || IsGamepadButtonDown(gamepadIdx, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
            actions.chargingBurst = true;
        }

        if (IsGamepadButtonPressed(gamepadIdx, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) {
            actions.discardWeapon = true;
        }

        actions.move = (fsVec2){.x = lStickX, .y = lStickY};
        actions.aim = (fsVec2){.x = rStickX, .y = rStickY};
        return computeActions(e, drone, &actions);
    }
    if (!controllerConnected && drone->idx != e->humanDroneInput) {
        return actions;
    }

    fsVec2 move = fsVec2_zero;
    if (IsKeyDown(KEY_W)) {
        move.y += -1.0f;
    }
    if (IsKeyDown(KEY_S)) {
        move.y += 1.0f;
    }
    if (IsKeyDown(KEY_A)) {
        move.x += -1.0f;
    }
    if (IsKeyDown(KEY_D)) {
        move.x += 1.0f;
    }
    actions.move = fsNormalize(move);

    Vector2 mousePos = (Vector2){.x = (float)GetMouseX(), .y = (float)GetMouseY()};
    actions.aim = fsNormalize(fsSub(rayVecToFsVec(e, mousePos), drone->pos));

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        actions.chargingWeapon = true;
        actions.shoot = true;
    } else if (drone->chargingWeapon && IsMouseButtonUp(MOUSE_BUTTON_LEFT)) {
        actions.shoot = true;
    }
    if (IsKeyDown(KEY_SPACE)) {
        actions.brake = true;
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        actions.chargingBurst = true;
    }

    return computeActions(e, drone, &actions);
}

bool droneControlledByHuman(const iwEnv *e, uint8_t i) {
    if (!e->humanInput) {
        return false;
    }
    return (e->connectedControllers > 1 && i >= e->humanDroneInput) || (e->connectedControllers <= 1 && i == e->humanDroneInput);
}

void addLog(iwEnv *e, Log *log) {
    e->log.length += log->length;
    e->log.ties += log->ties;

    for (uint8_t j = 0; j < e->numDrones; j++) {
        e->log.stats[j].returns += log->stats[j].returns;
        e->log.stats[j].wins += log->stats[j].wins;

        e->log.stats[j].distanceTraveled += log->stats[j].distanceTraveled;
        e->log.stats[j].absDistanceTraveled += log->stats[j].absDistanceTraveled;
        e->log.stats[j].brakeTime += log->stats[j].brakeTime;
        e->log.stats[j].totalBursts += log->stats[j].totalBursts;
        e->log.stats[j].burstsHit += log->stats[j].burstsHit;
        e->log.stats[j].energyEmptied += log->stats[j].energyEmptied;
        e->log.stats[j].shieldsBroken += log->stats[j].shieldsBroken;
        e->log.stats[j].ownShieldBroken += log->stats[j].ownShieldBroken;
        e->log.stats[j].selfKills += log->stats[j].selfKills;
        e->log.stats[j].kills += log->stats[j].kills;
        e->log.stats[j].unknownKills += log->stats[j].unknownKills;

        for (uint8_t k = 0; k < NUM_WEAPONS; k++) {
            e->log.stats[j].shotsFired[k] += log->stats[j].shotsFired[k];
            e->log.stats[j].shotsHit[k] += log->stats[j].shotsHit[k];
            e->log.stats[j].shotsTaken[k] += log->stats[j].shotsTaken[k];
            e->log.stats[j].ownShotsTaken[k] += log->stats[j].ownShotsTaken[k];
            e->log.stats[j].weaponsPickedUp[k] += log->stats[j].weaponsPickedUp[k];
            e->log.stats[j].shotDistances[k] += log->stats[j].shotDistances[k];
        }

        e->log.stats[j].totalShotsFired += log->stats[j].totalShotsFired;
        e->log.stats[j].totalShotsHit += log->stats[j].totalShotsHit;
        e->log.stats[j].totalShotsTaken += log->stats[j].totalShotsTaken;
        e->log.stats[j].totalOwnShotsTaken += log->stats[j].totalOwnShotsTaken;
        e->log.stats[j].totalWeaponsPickedUp += log->stats[j].totalWeaponsPickedUp;
        e->log.stats[j].totalShotDistances += log->stats[j].totalShotDistances;
    }

    e->log.n += 1.0f;
}

// TODO: 2nd agent doesn't seem to work right
void stepEnv(iwEnv *e) {
    bool didReset = false;
    if (e->needsReset) {
        DEBUG_LOG("Resetting environment");
        e->needsReset = false;
        resetEnv(e);
        didReset = true;
        // Skip the rest of this step after reset to prevent use-after-free
        return;
    }

#ifdef __EMSCRIPTEN__
    lastFrameTime = emscripten_get_now();
    accumulator = 0.0;
#endif

#ifndef NDEBUG
    for (uint8_t i = 0; i < cc_array_size(e->debugPoints); i++) {
        debugPoint *point = safe_array_get_at(e->debugPoints, i);
        fastFree(point);
    }
    cc_array_remove_all(e->debugPoints);
#endif

    agentActions stepActions[e->numDrones];
    memset(stepActions, 0x0, e->numDrones * sizeof(agentActions));

    // preprocess agent actions for the next frameSkip steps
    for (uint8_t i = 0; i < e->numDrones; i++) {
        droneEntity *drone = drone_soa_get(&e->drones, i);
        if (drone->dead || droneControlledByHuman(e, i)) {
            continue;
        }

        if (i < e->numAgents) {
            stepActions[i] = computeActions(e, drone, NULL);
        } else {
            const agentActions scriptedActions = scriptedAgentActions(e, drone);
            stepActions[i] = computeActions(e, drone, &scriptedActions);
        }
    }

    // reset reward buffer
    memset(e->rewards, 0x0, e->numAgents * sizeof(float));

    for (int i = 0; i < e->frameSkip; i++) {
        // Skip physics steps if we just reset to prevent use-after-free
        if (didReset) {
            break;
        }
#ifdef __EMSCRIPTEN__
        // running at a fixed frame rate doesn't seem to work well in
        // the browser, so we need to adjust to handle a variable frame
        // rate; see https://www.gafferongames.com/post/fix_your_timestep/
        const double curTime = emscripten_get_now();
        const double deltaTime = (curTime - lastFrameTime) / 1000.0;
        lastFrameTime = curTime;

        accumulator += deltaTime;
        while (accumulator >= e->deltaTime) {
            if (e->needsReset) {
                break;
            }
#endif
            e->episodeLength++;

            // handle actions
            if (e->client != NULL) {
                updateHumanInputToggle(e);
            }

            for (uint8_t i = 0; i < e->numDrones; i++) {
                droneEntity *drone = drone_soa_get(&e->drones, i);
                if (drone == NULL) continue;
                memset(&drone->stepInfo, 0x0, sizeof(droneStepInfo));
                if (drone->dead) {
                    drone->diedThisStep = false;
                }
                drone->killedBy = -1;
                memset(&drone->killed, 0x0, sizeof(drone->killed));
            }

            for (uint8_t i = 0; i < e->numDrones; i++) {
                droneEntity *drone = drone_soa_get(&e->drones, i);
                if (drone == NULL || drone->dead) {
                    continue;
                }

                agentActions actions;
                // take inputs from humans every frame
                if (droneControlledByHuman(e, i)) {
                    actions = getPlayerInputs(e, drone, i - e->humanDroneInput);
                } else {
                    actions = stepActions[i];
                }

                if (actions.discardWeapon) {
                    droneDiscardWeapon(e, drone);
                }
                if (actions.shoot) {
                    droneShoot(e, drone, actions.aim, actions.chargingWeapon);
                }
                if (actions.chargingBurst) {
                    droneChargeBurst(e, drone);
                } else if (drone->chargingBurst) {
                    droneBurst(e, drone);
                }
                if (!fsVecEqual(actions.move, fsVec2_zero)) {
                    droneMove(e, drone, actions.move);
                }
                droneBrake(e, drone, actions.brake);

                // update shield velocity/pos if its active
                if (drone->shield != NULL) {
                    FS_BODY_POS(&e->world, drone->shield->body) = FS_BODY_POS(&e->world, drone->body);
                    FS_BODY_VEL(&e->world, drone->shield->body) = FS_BODY_VEL(&e->world, drone->body);
                }
            }

            fsWorld_Step(&e->world, e->deltaTime);
            dampTrackedPhysics(e);

            // update dynamic body positions and velocities
            handleBodyMoveEvents(e);

            // handle collisions - skip if reset happened during this step
            if (e->needsReset) break;
            handleContactEvents(e);

            // handle sudden death
            e->stepsLeft = max(e->stepsLeft - 1, 0);
            if ((!e->isTraining || e->numDrones == e->numAgents) && e->stepsLeft == 0) {
                e->suddenDeathSteps = max(e->suddenDeathSteps - 1, 0);
                if (e->suddenDeathSteps == 0) {
                    DEBUG_LOG("placing sudden death walls");
                    handleSuddenDeath(e);
                    e->suddenDeathSteps = e->totalSuddenDeathSteps;
                }
            }

            projectilesStep(e);

            int8_t lastAlive = -1;
            int8_t lastAliveTeam = -1;
            bool allAliveOnSameTeam = false;
            bool roundOver = false;
            uint8_t deadDrones = 0;
            for (uint8_t i = 0; i < e->numDrones; i++) {
                droneEntity *drone = drone_soa_get(&e->drones, i);
                if (drone->livesLeft != 0) {
                    if (!droneStep(e, drone)) {
                        // couldn't find a respawn position, end the round
                        deadDrones++;
                        roundOver = true;
                    }
                    lastAlive = i;

                    if (e->teamsEnabled) {
                        if (lastAliveTeam == -1) {
                            lastAliveTeam = drone->team;
                            allAliveOnSameTeam = true;
                        } else if (drone->team != lastAliveTeam) {
                            allAliveOnSameTeam = false;
                        }
                    }
                } else {
                    deadDrones++;
                    if (i < e->numAgents) {
                        if (drone->diedThisStep) {
                            e->terminals[i] = 1.0f;
                        }
                        // else {
                        //     e->masks[i] = 0;
                        // }
                    }
                }
            }

            weaponPickupsStep(e);

            if (!roundOver) {
                roundOver = deadDrones >= e->numDrones - 1;
            }
            if (e->teamsEnabled && allAliveOnSameTeam) {
                roundOver = true;
                lastAlive = -1;
            }
            // if the enemy drone(s) are scripted don't enable sudden death
            // so that the agent has to work for victories
            if (e->isTraining && e->numDrones != e->numAgents && e->stepsLeft == 0) {
                roundOver = true;
                lastAliveTeam = -1;
            }
            if (roundOver && deadDrones < e->numDrones - 1) {
                lastAlive = -1;
            }
            computeRewards(e, roundOver, lastAlive, lastAliveTeam);

            if (e->client != NULL) {
                renderEnv(e, false, roundOver, lastAlive, lastAliveTeam);
            }

            if (roundOver) {
                if (e->numDrones != e->numAgents && e->stepsLeft == 0) {
                    DEBUG_LOG("truncating episode");
                    memset(e->truncations, 1, e->numAgents * sizeof(uint8_t));
                } else {
                    DEBUG_LOG("terminating episode");
                    for (uint8_t i = 0; i < e->numAgents; i++) {
                        e->terminals[i] = 1.0f;
                    }
                }

                Log log = {0};
                log.length = e->episodeLength;
                if (lastAlive != -1) {
                    e->stats[lastAlive].wins = 1.0f;
                } else if (!e->teamsEnabled || (e->teamsEnabled && lastAliveTeam == -1)) {
                    log.ties = 1.0f;
                }

                for (uint8_t i = 0; i < e->numDrones; i++) {
                    const droneEntity *drone = drone_soa_get(&e->drones, i);
                    if (!drone->dead && e->teamsEnabled && drone->team == lastAliveTeam) {
                        e->stats[i].wins = 1.0f;
                    }
                    // set absolute distance traveled of agent drones
                    e->stats[i].absDistanceTraveled = fsDistance(drone->initalPos, drone->pos);
                }

                memcpy(log.stats, e->stats, sizeof(e->stats));
                addLog(e, &log);

                e->needsReset = true;
                break;
            }
#ifdef __EMSCRIPTEN__
            accumulator -= e->deltaTime;
        }

        if (e->needsReset) {
            break;
        }
#endif
    }

#ifndef NDEBUG
    bool gotReward = false;
    for (uint8_t i = 0; i < e->numDrones; i++) {
        if (e->rewards[i] > REWARD_EPS || e->rewards[i] < -REWARD_EPS) {
            gotReward = true;
            break;
        }
    }
    if (gotReward) {
        DEBUG_RAW_LOG("!!! rewards: [");
        for (uint8_t i = 0; i < e->numDrones; i++) {
            const float reward = e->rewards[i];
            DEBUG_RAW_LOGF("%f", reward);
            if (i < e->numDrones - 1) {
                DEBUG_RAW_LOG(", ");
            }
        }
        DEBUG_RAW_LOGF("] step %d\n", e->totalSteps - e->stepsLeft);
    }
#endif

    // Invalidate distance cache since drones moved
#if IW_FAST_TRAINING_OBS
    if (!e->isTraining) {
        invalidateDistanceCache(e);
    }
#else
    invalidateDistanceCache(e);
#endif
    
    // Mark all agents as dirty for observation computation
    e->obsDirty = true;
    for (uint8_t i = 0; i < e->numAgents; i++) {
        e->obsDirtyPerAgent[i] = 1;
    }
}

#endif
