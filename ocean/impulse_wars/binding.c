// VecEnv-style binding for impulse_wars (puffer 4.0)
#include "env.h"

// Max observation size (bytes) per agent for _MAX_DRONES (4)
#define OBS_SIZE 1192
#define NUM_ATNS 1
#define ACT_SIZES { CONTINUOUS_ACTION_SIZE }
#define OBS_TENSOR_T ByteTensor

#define MY_VEC_INIT
#define Env iwEnv
#define num_agents numAgents
#define action_mask masks
#include "../../src/vecenv.h"

// Create vec of envs and initialize shared maps + per-env setup
Env* my_vec_init(int* num_envs_out, int* buffer_env_starts, int* buffer_env_counts,
                 Dict* vec_kwargs, Dict* env_kwargs) {
    int total_agents = (int)dict_get(vec_kwargs, "total_agents")->value;
    int num_buffers = (int)dict_get(vec_kwargs, "num_buffers")->value;
    int agents_per_buffer = total_agents / num_buffers;

    // Allocate max possible envs (1 agent per env worst case)
    Env* envs = (Env*)calloc(total_agents, sizeof(Env));

    int num_envs = 0;
    int agents_created = 0;
    while (agents_created < total_agents) {
        srand(num_envs);
        // default my_init will populate env->numAgents etc
        my_init(&envs[num_envs], env_kwargs);
        agents_created += envs[num_envs].numAgents;
        num_envs++;
    }

    // Shrink to actual size needed
    envs = (Env*)realloc(envs, num_envs * sizeof(Env));

    // Initialize shared maps once and call per-env setup
    initMaps(&envs[0]);
    for (int i = 0; i < num_envs; i++) {
        setupEnv(&envs[i]);
    }

    // Fill buffer info by iterating through envs
    int buf = 0;
    int buf_agents = 0;
    buffer_env_starts[0] = 0;
    buffer_env_counts[0] = 0;
    for (int i = 0; i < num_envs; i++) {
        buf_agents += envs[i].numAgents;
        buffer_env_counts[buf]++;
        if (buf_agents >= agents_per_buffer && buf < num_buffers - 1) {
            buf++;
            buffer_env_starts[buf] = i + 1;
            buffer_env_counts[buf] = 0;
            buf_agents = 0;
        }
    }

    *num_envs_out = num_envs;
    return envs;
}


// Initialize a single env from kwargs
void my_init(Env* e, Dict* kwargs) {
    e->numDrones = (uint8_t)dict_get(kwargs, "num_drones")->value;
    e->numAgents = 2;
    int map_idx = (int)dict_get(kwargs, "map_idx")->value;
    uint64_t seed = (uint64_t)dict_get(kwargs, "seed")->value;
    bool enable_teams = (bool)dict_get(kwargs, "enable_teams")->value;
    bool sitting_duck = (bool)dict_get(kwargs, "sitting_duck")->value;
    bool is_training = (bool)dict_get(kwargs, "is_training")->value;
    bool continuous = (bool)dict_get(kwargs, "continuous")->value;
    initEnv(
        e,
        (uint8_t)e->numDrones,
        (uint8_t)e->numAgents,
        (int8_t)map_idx,
        (uint64_t)seed,
        enable_teams,
        sitting_duck,
        is_training,
        continuous
    );

    setRewards(
        e,
        (float)dict_get(kwargs, "reward_win")->value,
        (float)dict_get(kwargs, "reward_self_kill")->value,
        (float)dict_get(kwargs, "reward_enemy_death")->value,
        (float)dict_get(kwargs, "reward_enemy_kill")->value,
        0.0f, // teammate death punishment
        0.0f, // teammate kill punishment
        (float)dict_get(kwargs, "reward_death")->value,
        (float)dict_get(kwargs, "reward_energy_emptied")->value,
        (float)dict_get(kwargs, "reward_weapon_pickup")->value,
        (float)dict_get(kwargs, "reward_shield_break")->value,
        (float)dict_get(kwargs, "reward_shot_hit_coef")->value,
        (float)dict_get(kwargs, "reward_explosion_hit_coef")->value
    );
}

// Logging: convert env Log to Dict values
void my_log(Log* log, Dict* out) {
    dict_set(out, "episode_length", log->length);
    dict_set(out, "ties", log->ties);

    dict_set(out, "perf", log->stats[0].wins);
    dict_set(out, "score", log->stats[0].wins);

    /*char buf[128];
    for (uint8_t i = 0; i < MAX_DRONES; i++) {
        snprintf(buf, sizeof(buf), "drone_%d_returns", i);
        dict_set(out, buf, log->stats[i].returns);
        snprintf(buf, sizeof(buf), "drone_%d_distance_traveled", i);
        dict_set(out, buf, log->stats[i].distanceTraveled);
        snprintf(buf, sizeof(buf), "drone_%d_abs_distance_traveled", i);
        dict_set(out, buf, log->stats[i].absDistanceTraveled);
        snprintf(buf, sizeof(buf), "drone_%d_brake_time", i);
        dict_set(out, buf, log->stats[i].brakeTime);
        snprintf(buf, sizeof(buf), "drone_%d_total_bursts", i);
        dict_set(out, buf, log->stats[i].totalBursts);
        snprintf(buf, sizeof(buf), "drone_%d_bursts_hit", i);
        dict_set(out, buf, log->stats[i].burstsHit);
        snprintf(buf, sizeof(buf), "drone_%d_energy_emptied", i);
        dict_set(out, buf, log->stats[i].energyEmptied);
        snprintf(buf, sizeof(buf), "drone_%d_shields_broken", i);
        dict_set(out, buf, log->stats[i].shieldsBroken);
        snprintf(buf, sizeof(buf), "drone_%d_own_shield_broken", i);
        dict_set(out, buf, log->stats[i].ownShieldBroken);
        snprintf(buf, sizeof(buf), "drone_%d_self_kills", i);
        dict_set(out, buf, log->stats[i].selfKills);
        snprintf(buf, sizeof(buf), "drone_%d_kills", i);
        dict_set(out, buf, log->stats[i].kills);
        snprintf(buf, sizeof(buf), "drone_%d_unknown_kills", i);
        dict_set(out, buf, log->stats[i].unknownKills);
        snprintf(buf, sizeof(buf), "drone_%d_wins", i);
        dict_set(out, buf, log->stats[i].wins);

        snprintf(buf, sizeof(buf), "drone_%d_total_shots_fired", i);
        dict_set(out, buf, log->stats[i].totalShotsFired);
        snprintf(buf, sizeof(buf), "drone_%d_total_shots_hit", i);
        dict_set(out, buf, log->stats[i].totalShotsHit);
        snprintf(buf, sizeof(buf), "drone_%d_total_shots_taken", i);
        dict_set(out, buf, log->stats[i].totalShotsTaken);
        snprintf(buf, sizeof(buf), "drone_%d_total_own_shots_taken", i);
        dict_set(out, buf, log->stats[i].totalOwnShotsTaken);
        snprintf(buf, sizeof(buf), "drone_%d_total_picked_up", i);
        dict_set(out, buf, log->stats[i].totalWeaponsPickedUp);
        snprintf(buf, sizeof(buf), "drone_%d_total_shot_distances", i);
        dict_set(out, buf, log->stats[i].totalShotDistances);
    }*/
}