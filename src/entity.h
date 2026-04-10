#pragma once

#include <stdbool.h>

#include "config.h"

typedef enum EntityKind {
    ENTITY_NONE = 0,
    ENTITY_PLAYER,
    ENTITY_ENEMY,
} EntityKind;

typedef struct Entity {
    EntityKind kind;
    char name[32];
    char glyph;
    int x;
    int y;
    int hp;
    int max_hp;
    int attack;
    int defense;
    bool alive;
    bool blocks;
} Entity;

void entity_init(Entity *entity, EntityKind kind, const char *name, char glyph,
                 int x, int y, int hp, int attack, int defense, bool blocks);
int entity_damage_to(const Entity *attacker, const Entity *defender);
void entity_apply_damage(Entity *entity, int damage);
bool entity_is_adjacent(const Entity *a, const Entity *b);
