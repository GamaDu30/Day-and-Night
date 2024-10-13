
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

ItemData getItemData(ItemType type)
{
    return itemsData[type];
}