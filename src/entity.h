#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "config.h"

typedef enum ActorType {
    ACTOR_NONE = 0,
    ACTOR_PLAYER,
    ACTOR_ENEMY,
} ActorType;

typedef enum PlayerClass {
    PLAYER_CLASS_WARRIOR = 0,
    PLAYER_CLASS_MAGE,
    PLAYER_CLASS_ARCHER,
    PLAYER_CLASS_ROGUE,
    PLAYER_CLASS_COUNT,
} PlayerClass;

typedef enum EnemyType {
    ENEMY_NONE = 0,
    ENEMY_GOBLIN,
    ENEMY_SKELETON,
    ENEMY_ZOMBIE,
    ENEMY_BAT,
    ENEMY_SLIME,
    ENEMY_SPIDER,
    ENEMY_FUNGUS,
    ENEMY_SHADOW_KNIGHT,
    ENEMY_NECROMANCER,
    ENEMY_STONE_GOLEM,
    ENEMY_ASSASSIN,
    ENEMY_WITCH,
    ENEMY_CRYPT_GUARDIAN,
    ENEMY_COUNT,
} EnemyType;

typedef enum AIState {
    AI_IDLE = 0,
    AI_PATROL,
    AI_CHASE,
    AI_ATTACK,
    AI_RETREAT,
} AIState;

typedef enum WeaponType {
    WEAPON_NONE = 0,
    WEAPON_IRON_SWORD,
    WEAPON_OAK_STAFF,
    WEAPON_HUNTER_BOW,
    WEAPON_SHADOW_DAGGER,
    WEAPON_FLAME_BLADE,
    WEAPON_THUNDER_HAMMER,
    WEAPON_SUPREME_STAFF,
    WEAPON_ASTRAL_BOW,
    WEAPON_MOON_TWINS,
    WEAPON_COUNT,
} WeaponType;

typedef enum ItemType {
    ITEM_NONE = 0,
    ITEM_POTION_HEALTH,
    ITEM_POTION_MANA,
    ITEM_SCROLL_FIRE,
    ITEM_SCROLL_ICE,
    ITEM_SCROLL_HEALING,
    ITEM_RELIC_TITAN_RING,
    ITEM_RELIC_DEMON_EYE,
    ITEM_RELIC_CRYSTAL_HEART,
    ITEM_CRYPT_KEY,
    ITEM_COUNT,
} ItemType;

typedef enum AbilityType {
    ABILITY_NONE = 0,
    ABILITY_BRUTAL_STRIKE,
    ABILITY_WAR_CRY,
    ABILITY_SHIELD_WALL,
    ABILITY_FIREBALL,
    ABILITY_BLIZZARD,
    ABILITY_LIGHTNING,
    ABILITY_ARROW_RAIN,
    ABILITY_PIERCING_SHOT,
    ABILITY_TRAP_KIT,
    ABILITY_EVASION,
    ABILITY_POISON_BLADE,
    ABILITY_SMOKE_BOMB,
    ABILITY_COUNT,
} AbilityType;

typedef enum StatusKind {
    STATUS_KIND_POISON = 0,
    STATUS_KIND_BURN,
    STATUS_KIND_FREEZE,
    STATUS_KIND_STUN,
    STATUS_KIND_CURSE,
    STATUS_KIND_INVIS,
    STATUS_KIND_GUARD,
    STATUS_KIND_HASTE,
    STATUS_KIND_COUNT,
} StatusKind;

typedef uint8_t StatusFlags;

#define STATUS_POISON ((StatusFlags)(1u << STATUS_KIND_POISON))
#define STATUS_BURN   ((StatusFlags)(1u << STATUS_KIND_BURN))
#define STATUS_FREEZE ((StatusFlags)(1u << STATUS_KIND_FREEZE))
#define STATUS_STUN   ((StatusFlags)(1u << STATUS_KIND_STUN))
#define STATUS_CURSE  ((StatusFlags)(1u << STATUS_KIND_CURSE))
#define STATUS_INVIS  ((StatusFlags)(1u << STATUS_KIND_INVIS))
#define STATUS_GUARD  ((StatusFlags)(1u << STATUS_KIND_GUARD))
#define STATUS_HASTE  ((StatusFlags)(1u << STATUS_KIND_HASTE))

typedef struct WeaponProfile {
    WeaponType type;
    const char *name;
    char glyph;
    int attack_bonus;
    int range;
    int crit_bonus;
    bool magical;
} WeaponProfile;

typedef struct ItemProfile {
    ItemType type;
    const char *name;
    char glyph;
    int power;
    bool consumable;
    bool relic;
} ItemProfile;

typedef struct AbilityProfile {
    AbilityType type;
    const char *name;
    int mana_cost;
    int cooldown;
    const char *summary;
} AbilityProfile;

typedef struct StatusTickResult {
    int damage;
    StatusFlags expired;
    bool alive;
} StatusTickResult;

typedef struct Entity {
    ActorType kind;
    PlayerClass player_class;
    EnemyType enemy_type;
    AIState ai_state;
    char name[32];
    char glyph;
    int x;
    int y;
    int spawn_x;
    int spawn_y;
    int hp;
    int max_hp;
    int mp;
    int max_mp;
    int attack;
    int defense;
    int vision;
    int level;
    int exp;
    int exp_to_level;
    int gold;
    int exp_reward;
    int gold_reward;
    WeaponType weapon;
    AbilityType abilities[3];
    int ability_cooldowns[3];
    StatusFlags status_flags;
    int status_turns[STATUS_KIND_COUNT];
    int attack_buff_turns;
    int poison_blade_turns;
    bool alive;
    bool blocks;
    bool elite;
    bool boss;
} Entity;

void entity_init_player(Entity *entity, PlayerClass player_class);
void entity_init_enemy(Entity *entity, EnemyType enemy_type, int floor_number, int x, int y);

char entity_player_class_glyph(PlayerClass player_class);
const char *entity_player_class_name(PlayerClass player_class);
const char *entity_player_class_summary(PlayerClass player_class);
AbilityType entity_class_ability(PlayerClass player_class, int slot);

const WeaponProfile *entity_weapon_profile(WeaponType weapon_type);
const ItemProfile *entity_item_profile(ItemType item_type);
const AbilityProfile *entity_ability_profile(AbilityType ability_type);
WeaponType entity_recommended_upgrade(PlayerClass player_class, int floor_number);

int entity_weapon_range(const Entity *entity);
int entity_total_attack(const Entity *entity);
int entity_total_defense(const Entity *entity);
int entity_attack_damage(const Entity *attacker, const Entity *defender,
                         int bonus_damage, bool ignore_defense);

void entity_apply_damage(Entity *entity, int damage);
void entity_restore_hp(Entity *entity, int amount);
void entity_restore_mp(Entity *entity, int amount);
bool entity_spend_mp(Entity *entity, int amount);

bool entity_has_status(const Entity *entity, StatusFlags flag);
void entity_apply_status(Entity *entity, StatusFlags flag, int turns);
void entity_clear_status(Entity *entity, StatusFlags flag);
StatusTickResult entity_tick_statuses(Entity *entity);
void entity_tick_cooldowns(Entity *entity);

bool entity_is_adjacent(const Entity *a, const Entity *b);
