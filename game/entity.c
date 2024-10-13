#pragma once

typedef enum EntityType
{
    ENTITY_nil,
    ENTITY_player,
    ENTITY_mineral,
    ENTITY_tree,
    ENTITY_item,
    ENTITY_MAX,
} EntityType;

typedef struct Entity
{
    bool isValid;

    EntityType entityType;
    ItemType itemType;
    SpriteId spriteId;

    Vector2 pos;
    int health;
} Entity;

typedef struct EntityData
{
    SpriteId spriteId;

    bool isDestroyable;
    bool isSelectable;
    bool isPickable;

    ItemType lootType;
} EntityData;

EntityData entityData[ENTITY_MAX] = {0};

typedef enum UXState
{
    UX_nil,
    UX_inventory,
} UXState;

typedef struct World
{
    Entity entitites[MAX_ENTITIES_COUNT];
    Entity *selectedEntity;
    Item *inventory[INV_COUNT];
    Item hotbar[HOTBAR_AMOUNT];
    Item *heldItem;
    int heldItemOriginId;
    UXState uxState;
} World;

World *world = 0;

void initEntity()
{
    entityData[ENTITY_player] = (EntityData){.spriteId = SPRITE_player};
    entityData[ENTITY_mineral] = (EntityData){.spriteId = SPRITE_mineral, .isDestroyable = true, .isSelectable = true, .lootType = ITEM_iron};
    entityData[ENTITY_tree] = (EntityData){.spriteId = SPRITE_tree, .isDestroyable = true, .isSelectable = true, .lootType = ITEM_log};
    entityData[ENTITY_item] = (EntityData){.isSelectable = true, .isPickable = true};
}

EntityData *getEntityData(EntityType type)
{
    return &entityData[type];
}

Entity *createEntity()
{
    Entity *entityFound = 0;
    for (int i = 0; i < MAX_ENTITIES_COUNT; i++)
    {
        Entity *curEntity = &world->entitites[i];
        if (!curEntity->isValid)
        {
            entityFound = curEntity;
            break;
        }
    }

    assert(entityFound, "entityCreate: No entity found");
    entityFound->isValid = true;

    return entityFound;
}

void setupEntity(Entity *entity, EntityType type, Vector2 pos)
{
    entity->pos = pos;
    entity->entityType = type;

    if (type != ENTITY_item)
    {
        entity->spriteId = getEntityData(type)->spriteId;
    }
    else
    {
        entity->spriteId = getItemData(entity->itemType).spriteId;
    }
}

void destroyEntity(Entity *entity)
{
    entity->isValid = false;
    memset(entity, 0, sizeof(Entity));
}

void drawEntity(Entity *entity)
{
    Sprite *sprite = getSpriteFromId(entity->spriteId);
    Vector2 size = getspriteSize(entity->spriteId);

    // Selection color
    Vector4 col = v4(1, 1, 1, 1);
    if (entity == world->selectedEntity)
    {
        col.g = 0.5f;
        col.b = 0.5f;
    }

    // Pos
    Matrix4 xform = m4_scalar(1.0);
    xform = m4_translate(xform, v3(entity->pos.x, entity->pos.y, 0.0));

    // Pivot
    Vector2 pivot = v2_mulf(getPivot(sprite->pivot), -1);
    xform = m4_translate(xform, v3(pivot.x * size.x, pivot.y * size.y, 0));

    // Item shadow
    if (entity->entityType == ENTITY_item)
    {
        xform = m4_translate(xform, v3(0.0, sin(os_get_elapsed_seconds() * 3) + 4, 0.0));
        draw_image_xform(sprite->image, m4_translate(xform, v3(0.0, -5.0, 0.0)), size, v4(0, 0, 0, 0.5));
    }

    draw_image_xform(sprite->image, xform, size, col);
}