
#pragma once

typedef enum ItemType
{
    ITEM_nil,
    ITEM_iron,
    ITEM_log,
    ITEM_MAX
} ItemType;

typedef struct ItemData
{
    SpriteId spriteId;

    // TODO
    string description;
} ItemData;

ItemData itemsData[ITEM_MAX] = {0};

typedef struct Item
{
    ItemType type;
    int amount;
} Item;

void initItems()
{
    itemsData[ITEM_iron] = (ItemData){.spriteId = SPRITE_ITEM_iron};
    itemsData[ITEM_log] = (ItemData){.spriteId = SPRITE_ITEM_log};
}

SpriteId getItemSpriteId(ItemType type)
{
    return itemsData[type].spriteId;
}