#include "entity.h"

#include <stdio.h>
#include <string.h>

typedef struct ClassProfile {
    const char *name;
    const char *summary;
    char glyph;
    int max_hp;
    int max_mp;
    int attack;
    int defense;
    int vision;
    WeaponType weapon;
    AbilityType abilities[3];
} ClassProfile;

typedef struct EnemyProfile {
    EnemyType type;
    const char *name;
    char glyph;
    int max_hp;
    int max_mp;
    int attack;
    int defense;
    int vision;
    int exp_reward;
    int gold_reward;
    bool elite;
    bool boss;
} EnemyProfile;

static const ClassProfile CLASS_PROFILES[PLAYER_CLASS_COUNT] = {
    [PLAYER_CLASS_WARRIOR] = {
        "Warrior",
        "High armor, strong melee, and shield skills.",
        PLAYER_GLYPH_WARRIOR,
        36,
        14,
        6,
        3,
        6,
        WEAPON_IRON_SWORD,
        { ABILITY_BRUTAL_STRIKE, ABILITY_WAR_CRY, ABILITY_SHIELD_WALL },
    },
    [PLAYER_CLASS_MAGE] = {
        "Mage",
        "Low defense, strong mana pool, and heavy ranged spells.",
        PLAYER_GLYPH_MAGE,
        24,
        32,
        4,
        1,
        7,
        WEAPON_OAK_STAFF,
        { ABILITY_FIREBALL, ABILITY_BLIZZARD, ABILITY_LIGHTNING },
    },
    [PLAYER_CLASS_ARCHER] = {
        "Archer",
        "Flexible range, trap control, and precise burst attacks.",
        PLAYER_GLYPH_ARCHER,
        28,
        22,
        5,
        2,
        8,
        WEAPON_HUNTER_BOW,
        { ABILITY_ARROW_RAIN, ABILITY_PIERCING_SHOT, ABILITY_TRAP_KIT },
    },
    [PLAYER_CLASS_ROGUE] = {
        "Rogue",
        "Fast tempo, poison pressure, and evasive escapes.",
        PLAYER_GLYPH_ROGUE,
        26,
        18,
        5,
        1,
        7,
        WEAPON_SHADOW_DAGGER,
        { ABILITY_EVASION, ABILITY_POISON_BLADE, ABILITY_SMOKE_BOMB },
    },
};

static const EnemyProfile ENEMY_PROFILES[ENEMY_COUNT] = {
    [ENEMY_GOBLIN] = { ENEMY_GOBLIN, "Goblin", 'g', 18, 0, 5, 0, 5, 10, 8, false, false },
    [ENEMY_SKELETON] = { ENEMY_SKELETON, "Skeleton", 's', 15, 0, 6, 1, 4, 12, 10, false, false },
    [ENEMY_ZOMBIE] = { ENEMY_ZOMBIE, "Zombie", 'z', 24, 0, 4, 1, 4, 14, 11, false, false },
    [ENEMY_BAT] = { ENEMY_BAT, "Bat", 'b', 10, 0, 4, 0, 7, 8, 7, false, false },
    [ENEMY_SLIME] = { ENEMY_SLIME, "Slime", 'm', 14, 0, 4, 0, 4, 9, 8, false, false },
    [ENEMY_SPIDER] = { ENEMY_SPIDER, "Spider", 'x', 16, 0, 5, 1, 5, 13, 12, false, false },
    [ENEMY_FUNGUS] = { ENEMY_FUNGUS, "Fungus", 'F', 20, 0, 3, 2, 3, 15, 10, false, false },
    [ENEMY_SHADOW_KNIGHT] = { ENEMY_SHADOW_KNIGHT, "Shadow Knight", 'K', 42, 0, 12, 4, 7, 32, 22, true, false },
    [ENEMY_NECROMANCER] = { ENEMY_NECROMANCER, "Necromancer", 'N', 34, 16, 10, 2, 8, 30, 20, true, false },
    [ENEMY_STONE_GOLEM] = { ENEMY_STONE_GOLEM, "Stone Golem", 'G', 54, 0, 14, 6, 6, 38, 24, true, false },
    [ENEMY_ASSASSIN] = { ENEMY_ASSASSIN, "Assassin", 'A', 28, 0, 15, 1, 8, 34, 22, true, false },
    [ENEMY_WITCH] = { ENEMY_WITCH, "Witch", 'W', 30, 18, 11, 2, 8, 34, 24, true, false },
    [ENEMY_CRYPT_GUARDIAN] = { ENEMY_CRYPT_GUARDIAN, "Crypt Guardian", 'B', 120, 0, 18, 6, GRID_COLS, 160, 120, true, true },
};

static const WeaponProfile WEAPON_PROFILES[WEAPON_COUNT] = {
    [WEAPON_NONE] = { WEAPON_NONE, "Fists", '*', 0, 1, 0, false },
    [WEAPON_IRON_SWORD] = { WEAPON_IRON_SWORD, "Iron Sword", '/', 3, 1, 10, false },
    [WEAPON_OAK_STAFF] = { WEAPON_OAK_STAFF, "Oak Staff", '|', 2, 4, 0, true },
    [WEAPON_HUNTER_BOW] = { WEAPON_HUNTER_BOW, "Hunter Bow", ')', 3, 6, 8, false },
    [WEAPON_SHADOW_DAGGER] = { WEAPON_SHADOW_DAGGER, "Shadow Dagger", '/', 2, 1, 25, false },
    [WEAPON_FLAME_BLADE] = { WEAPON_FLAME_BLADE, "Flame Blade", '/', 6, 1, 14, true },
    [WEAPON_THUNDER_HAMMER] = { WEAPON_THUNDER_HAMMER, "Thunder Hammer", '=', 8, 1, 6, true },
    [WEAPON_SUPREME_STAFF] = { WEAPON_SUPREME_STAFF, "Supreme Staff", '!', 8, 6, 0, true },
    [WEAPON_ASTRAL_BOW] = { WEAPON_ASTRAL_BOW, "Astral Bow", ')', 9, 8, 18, true },
    [WEAPON_MOON_TWINS] = { WEAPON_MOON_TWINS, "Moon Twins", '/', 7, 1, 35, false },
};

static const ItemProfile ITEM_PROFILES[ITEM_COUNT] = {
    [ITEM_NONE] = { ITEM_NONE, "Empty", ' ', 0, false, false },
    [ITEM_POTION_HEALTH] = { ITEM_POTION_HEALTH, "Health Potion", 'H', 30, true, false },
    [ITEM_POTION_MANA] = { ITEM_POTION_MANA, "Mana Potion", 'M', 20, true, false },
    [ITEM_SCROLL_FIRE] = { ITEM_SCROLL_FIRE, "Fire Scroll", 'F', 18, true, false },
    [ITEM_SCROLL_ICE] = { ITEM_SCROLL_ICE, "Ice Scroll", 'I', 12, true, false },
    [ITEM_SCROLL_HEALING] = { ITEM_SCROLL_HEALING, "Healing Scroll", 'C', 24, true, false },
    [ITEM_RELIC_TITAN_RING] = { ITEM_RELIC_TITAN_RING, "Titan Ring", 'R', 10, false, true },
    [ITEM_RELIC_DEMON_EYE] = { ITEM_RELIC_DEMON_EYE, "Demon Eye", 'E', 2, false, true },
    [ITEM_RELIC_CRYSTAL_HEART] = { ITEM_RELIC_CRYSTAL_HEART, "Crystal Heart", 'X', 1, false, true },
    [ITEM_CRYPT_KEY] = { ITEM_CRYPT_KEY, "Crypt Key", 'K', 1, false, false },
};

static const AbilityProfile ABILITY_PROFILES[ABILITY_COUNT] = {
    [ABILITY_NONE] = { ABILITY_NONE, "None", 0, 0, "" },
    [ABILITY_BRUTAL_STRIKE] = { ABILITY_BRUTAL_STRIKE, "Brutal Strike", 6, 3, "Double melee damage to an adjacent target." },
    [ABILITY_WAR_CRY] = { ABILITY_WAR_CRY, "War Cry", 5, 4, "Gain +3 attack for three turns." },
    [ABILITY_SHIELD_WALL] = { ABILITY_SHIELD_WALL, "Shield Wall", 4, 4, "Gain guard and reduce incoming damage." },
    [ABILITY_FIREBALL] = { ABILITY_FIREBALL, "Fireball", 8, 3, "Blast the nearest target and burn nearby enemies." },
    [ABILITY_BLIZZARD] = { ABILITY_BLIZZARD, "Blizzard", 10, 4, "Freeze enemies around you." },
    [ABILITY_LIGHTNING] = { ABILITY_LIGHTNING, "Lightning", 12, 4, "Heavy damage to the nearest visible enemy." },
    [ABILITY_ARROW_RAIN] = { ABILITY_ARROW_RAIN, "Arrow Rain", 8, 3, "Hit several enemies at long range." },
    [ABILITY_PIERCING_SHOT] = { ABILITY_PIERCING_SHOT, "Piercing Shot", 7, 3, "Strike enemies in a line." },
    [ABILITY_TRAP_KIT] = { ABILITY_TRAP_KIT, "Trap Kit", 4, 2, "Place a trap on your current tile." },
    [ABILITY_EVASION] = { ABILITY_EVASION, "Evasion", 5, 3, "Ignore enemy attacks for one turn." },
    [ABILITY_POISON_BLADE] = { ABILITY_POISON_BLADE, "Poison Blade", 4, 3, "Your next melee hit applies poison." },
    [ABILITY_SMOKE_BOMB] = { ABILITY_SMOKE_BOMB, "Smoke Bomb", 7, 4, "Teleport to a safer nearby tile." },
};

static StatusFlags entity_status_mask(StatusKind kind) {
    return (StatusFlags)(1u << kind);
}

static const ClassProfile *entity_class_profile(PlayerClass player_class) {
    if (player_class < 0 || player_class >= PLAYER_CLASS_COUNT) {
        return NULL;
    }

    return &CLASS_PROFILES[player_class];
}

static const EnemyProfile *entity_enemy_profile(EnemyType enemy_type) {
    if (enemy_type < 0 || enemy_type >= ENEMY_COUNT) {
        return NULL;
    }

    return &ENEMY_PROFILES[enemy_type];
}

void entity_init_player(Entity *entity, PlayerClass player_class) {
    const ClassProfile *profile = entity_class_profile(player_class);
    if (entity == NULL || profile == NULL) {
        return;
    }

    memset(entity, 0, sizeof(*entity));
    entity->kind = ACTOR_PLAYER;
    entity->player_class = player_class;
    entity->ai_state = AI_IDLE;
    snprintf(entity->name, sizeof(entity->name), "%s", profile->name);
    entity->glyph = profile->glyph;
    entity->hp = profile->max_hp;
    entity->max_hp = profile->max_hp;
    entity->mp = profile->max_mp;
    entity->max_mp = profile->max_mp;
    entity->attack = profile->attack;
    entity->defense = profile->defense;
    entity->vision = profile->vision;
    entity->level = 1;
    entity->exp_to_level = 32;
    entity->gold = 20;
    entity->weapon = profile->weapon;
    entity->abilities[0] = profile->abilities[0];
    entity->abilities[1] = profile->abilities[1];
    entity->abilities[2] = profile->abilities[2];
    entity->alive = true;
    entity->blocks = true;
}

void entity_init_enemy(Entity *entity, EnemyType enemy_type, int floor_number, int x, int y) {
    const EnemyProfile *profile = entity_enemy_profile(enemy_type);
    if (entity == NULL || profile == NULL) {
        return;
    }

    memset(entity, 0, sizeof(*entity));
    entity->kind = ACTOR_ENEMY;
    entity->enemy_type = enemy_type;
    entity->ai_state = AI_PATROL;
    snprintf(entity->name, sizeof(entity->name), "%s", profile->name);
    entity->glyph = profile->glyph;
    entity->x = x;
    entity->y = y;
    entity->spawn_x = x;
    entity->spawn_y = y;
    entity->hp = profile->max_hp + (floor_number - 1) * (profile->boss ? 0 : 3);
    entity->max_hp = entity->hp;
    entity->mp = profile->max_mp;
    entity->max_mp = profile->max_mp;
    entity->attack = profile->attack + (floor_number - 1) * (profile->boss ? 0 : 1);
    entity->defense = profile->defense + (floor_number > 2 && !profile->boss ? 1 : 0);
    entity->vision = profile->vision;
    entity->exp_reward = profile->exp_reward + (floor_number - 1) * 2;
    entity->gold_reward = profile->gold_reward + (floor_number - 1) * 2;
    entity->alive = true;
    entity->blocks = true;
    entity->elite = profile->elite;
    entity->boss = profile->boss;
}

char entity_player_class_glyph(PlayerClass player_class) {
    const ClassProfile *profile = entity_class_profile(player_class);
    return profile != NULL ? profile->glyph : PLAYER_GLYPH_WARRIOR;
}

const char *entity_player_class_name(PlayerClass player_class) {
    const ClassProfile *profile = entity_class_profile(player_class);
    return profile != NULL ? profile->name : "Warrior";
}

const char *entity_player_class_summary(PlayerClass player_class) {
    const ClassProfile *profile = entity_class_profile(player_class);
    return profile != NULL ? profile->summary : "Balanced melee class.";
}

AbilityType entity_class_ability(PlayerClass player_class, int slot) {
    const ClassProfile *profile = entity_class_profile(player_class);
    if (profile == NULL || slot < 0 || slot >= 3) {
        return ABILITY_NONE;
    }

    return profile->abilities[slot];
}

const WeaponProfile *entity_weapon_profile(WeaponType weapon_type) {
    if (weapon_type < 0 || weapon_type >= WEAPON_COUNT) {
        return &WEAPON_PROFILES[WEAPON_NONE];
    }

    return &WEAPON_PROFILES[weapon_type];
}

const ItemProfile *entity_item_profile(ItemType item_type) {
    if (item_type < 0 || item_type >= ITEM_COUNT) {
        return &ITEM_PROFILES[ITEM_NONE];
    }

    return &ITEM_PROFILES[item_type];
}

const AbilityProfile *entity_ability_profile(AbilityType ability_type) {
    if (ability_type < 0 || ability_type >= ABILITY_COUNT) {
        return &ABILITY_PROFILES[ABILITY_NONE];
    }

    return &ABILITY_PROFILES[ability_type];
}

WeaponType entity_recommended_upgrade(PlayerClass player_class, int floor_number) {
    (void)floor_number;

    switch (player_class) {
        case PLAYER_CLASS_WARRIOR:
            return WEAPON_FLAME_BLADE;
        case PLAYER_CLASS_MAGE:
            return WEAPON_SUPREME_STAFF;
        case PLAYER_CLASS_ARCHER:
            return WEAPON_ASTRAL_BOW;
        case PLAYER_CLASS_ROGUE:
            return WEAPON_MOON_TWINS;
        default:
            return WEAPON_IRON_SWORD;
    }
}

int entity_weapon_range(const Entity *entity) {
    if (entity == NULL) {
        return 1;
    }

    const WeaponProfile *profile = entity_weapon_profile(entity->weapon);
    return profile->range > 0 ? profile->range : 1;
}

int entity_total_attack(const Entity *entity) {
    if (entity == NULL) {
        return 0;
    }

    const WeaponProfile *weapon = entity_weapon_profile(entity->weapon);
    int total = entity->attack + weapon->attack_bonus;
    if (entity->attack_buff_turns > 0) {
        total += 3;
    }
    if (entity_has_status(entity, STATUS_CURSE)) {
        total -= 1;
    }
    if (entity_has_status(entity, STATUS_HASTE)) {
        total += 1;
    }

    return total < 1 ? 1 : total;
}

int entity_total_defense(const Entity *entity) {
    if (entity == NULL) {
        return 0;
    }

    int total = entity->defense;
    if (entity_has_status(entity, STATUS_GUARD)) {
        total += 3;
    }
    if (entity_has_status(entity, STATUS_CURSE)) {
        total -= 1;
    }

    return total < 0 ? 0 : total;
}

int entity_attack_damage(const Entity *attacker, const Entity *defender,
                         int bonus_damage, bool ignore_defense) {
    if (attacker == NULL || defender == NULL) {
        return 0;
    }

    int defense = ignore_defense ? 0 : entity_total_defense(defender);
    int damage = entity_total_attack(attacker) + bonus_damage - defense;
    return damage < 1 ? 1 : damage;
}

void entity_apply_damage(Entity *entity, int damage) {
    if (entity == NULL || !entity->alive || damage <= 0) {
        return;
    }

    entity->hp -= damage;
    if (entity->hp <= 0) {
        entity->hp = 0;
        entity->alive = false;
    }
}

void entity_restore_hp(Entity *entity, int amount) {
    if (entity == NULL || !entity->alive || amount <= 0) {
        return;
    }

    entity->hp += amount;
    if (entity->hp > entity->max_hp) {
        entity->hp = entity->max_hp;
    }
}

void entity_restore_mp(Entity *entity, int amount) {
    if (entity == NULL || !entity->alive || amount <= 0) {
        return;
    }

    entity->mp += amount;
    if (entity->mp > entity->max_mp) {
        entity->mp = entity->max_mp;
    }
}

bool entity_spend_mp(Entity *entity, int amount) {
    if (entity == NULL || amount < 0 || entity->mp < amount) {
        return false;
    }

    entity->mp -= amount;
    return true;
}

bool entity_has_status(const Entity *entity, StatusFlags flag) {
    if (entity == NULL) {
        return false;
    }

    return (entity->status_flags & flag) != 0;
}

void entity_apply_status(Entity *entity, StatusFlags flag, int turns) {
    if (entity == NULL || turns <= 0) {
        return;
    }

    for (int kind = 0; kind < STATUS_KIND_COUNT; ++kind) {
        StatusFlags mask = entity_status_mask((StatusKind)kind);
        if ((flag & mask) == 0) {
            continue;
        }

        entity->status_flags |= mask;
        if (entity->status_turns[kind] < turns) {
            entity->status_turns[kind] = turns;
        }
    }
}

void entity_clear_status(Entity *entity, StatusFlags flag) {
    if (entity == NULL) {
        return;
    }

    for (int kind = 0; kind < STATUS_KIND_COUNT; ++kind) {
        StatusFlags mask = entity_status_mask((StatusKind)kind);
        if ((flag & mask) == 0) {
            continue;
        }

        entity->status_flags &= (StatusFlags)~mask;
        entity->status_turns[kind] = 0;
    }
}

StatusTickResult entity_tick_statuses(Entity *entity) {
    StatusTickResult result = { 0, 0, false };
    if (entity == NULL || !entity->alive) {
        return result;
    }

    for (int kind = 0; kind < STATUS_KIND_COUNT; ++kind) {
        if (entity->status_turns[kind] <= 0) {
            continue;
        }

        if (kind == STATUS_KIND_POISON) {
            result.damage += 2;
        } else if (kind == STATUS_KIND_BURN) {
            result.damage += 3;
        }

        entity->status_turns[kind] -= 1;
        if (entity->status_turns[kind] <= 0) {
            StatusFlags mask = entity_status_mask((StatusKind)kind);
            entity->status_flags &= (StatusFlags)~mask;
            result.expired |= mask;
        }
    }

    if (result.damage > 0) {
        entity_apply_damage(entity, result.damage);
    }

    result.alive = entity->alive;
    return result;
}

void entity_tick_cooldowns(Entity *entity) {
    if (entity == NULL) {
        return;
    }

    for (int slot = 0; slot < 3; ++slot) {
        if (entity->ability_cooldowns[slot] > 0) {
            entity->ability_cooldowns[slot] -= 1;
        }
    }

    if (entity->attack_buff_turns > 0) {
        entity->attack_buff_turns -= 1;
    }
    if (entity->poison_blade_turns > 0) {
        entity->poison_blade_turns -= 1;
    }
}

bool entity_is_adjacent(const Entity *a, const Entity *b) {
    if (a == NULL || b == NULL) {
        return false;
    }

    int dx = a->x - b->x;
    if (dx < 0) {
        dx = -dx;
    }

    int dy = a->y - b->y;
    if (dy < 0) {
        dy = -dy;
    }

    return dx + dy == 1;
}
