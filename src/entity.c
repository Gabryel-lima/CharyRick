#include "entity.h"

#include <stdio.h>
#include <string.h>

void entity_init(Entity *entity, EntityKind kind, const char *name, char glyph,
                 int x, int y, int hp, int attack, int defense, bool blocks) {
    if (entity == NULL) {
        return;
    }

    memset(entity, 0, sizeof(*entity));
    entity->kind = kind;
    entity->glyph = glyph;
    entity->x = x;
    entity->y = y;
    entity->hp = hp;
    entity->max_hp = hp;
    entity->attack = attack;
    entity->defense = defense;
    entity->alive = true;
    entity->blocks = blocks;

    if (name != NULL) {
        snprintf(entity->name, sizeof(entity->name), "%s", name);
    }
}

int entity_damage_to(const Entity *attacker, const Entity *defender) {
    if (attacker == NULL || defender == NULL) {
        return 0;
    }

    int damage = attacker->attack - defender->defense;
    return damage < 1 ? 1 : damage;
}

void entity_apply_damage(Entity *entity, int damage) {
    if (entity == NULL || !entity->alive) {
        return;
    }

    entity->hp -= damage;
    if (entity->hp <= 0) {
        entity->hp = 0;
        entity->alive = false;
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
