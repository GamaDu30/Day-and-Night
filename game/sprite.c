#pragma once

typedef enum SpriteId
{
    SPRITE_nil,
    SPRITE_player,
    SPRITE_tree,
    SPRITE_mineral,
    SPRITE_ITEM_log,
    SPRITE_ITEM_iron,
    SPRITE_MAX
} SpriteId;

typedef enum Pivot
{
    PIVOT_TOP_LEFT,
    PIVOT_TOP_CENTER,
    PIVOT_TOP_RIGHT,
    PIVOT_CENTER_LEFT,
    PIVOT_CENTER_CENTER,
    PIVOT_CENTER_RIGHT,
    PIVOT_BOT_LEFT,
    PIVOT_BOT_CENTER,
    PIVOT_BOT_RIGHT,
} Pivot;

typedef struct Sprite
{
    Gfx_Image *image;
    Pivot pivot;
} Sprite;

Sprite sprites[SPRITE_MAX];

Sprite *getSpriteFromId(SpriteId spriteId)
{
    if (spriteId >= 0 && spriteId < SPRITE_MAX)
    {
        return &sprites[spriteId];
    }

    assert(false, "getSprite: no sprite found");
}

Vector2 getspriteSize(SpriteId spriteId)
{
    Sprite *sprite = getSpriteFromId(spriteId);
    return (Vector2){.x = sprite->image->width, .y = sprite->image->height};
}

Vector2 getPivot(Pivot pivot)
{
    switch (pivot)
    {
    case PIVOT_BOT_LEFT:
        return v2(0, 0);
        break;
    case PIVOT_BOT_CENTER:
        return v2(0.5, 0);
        break;
    case PIVOT_BOT_RIGHT:
        return v2(1, 0);
        break;
    case PIVOT_CENTER_LEFT:
        return v2(0, 0.5);
        break;
    case PIVOT_CENTER_CENTER:
        return v2(0.5, 0.5);
        break;
    case PIVOT_CENTER_RIGHT:
        return v2(1, 0.5);
        break;
    case PIVOT_TOP_LEFT:
        return v2(0, 1);
        break;
    case PIVOT_TOP_CENTER:
        return v2(0.5, 1);
        break;
    case PIVOT_TOP_RIGHT:
        return v2(1, 1);
        break;
    default:
        assert(false, "getPivot: Provided pivot doens't exist");
        break;
    }
}

void createSprite(SpriteId spriteId, string path, Pivot pivot)
{
    sprites[spriteId] = (Sprite){.image = load_image_from_disk(path, get_heap_allocator()), .pivot = pivot};
}

void initSprites()
{
    createSprite(SPRITE_player, STR("assets/images/player.png"), PIVOT_BOT_CENTER);
    createSprite(SPRITE_tree, STR("assets/images/ressource_tree0.png"), PIVOT_BOT_CENTER);
    createSprite(SPRITE_ITEM_log, STR("assets/images/item_tree0.png"), PIVOT_CENTER_CENTER);
    createSprite(SPRITE_mineral, STR("assets/images/ressource_mineral0.png"), PIVOT_BOT_CENTER);
    createSprite(SPRITE_ITEM_iron, STR("assets/images/item_mineral0.png"), PIVOT_CENTER_CENTER);
}

void drawSprite(SpriteId spriteId, Matrix4 xform, Vector2 size, Vector4 color)
{
    Sprite *sprite = getSpriteFromId(spriteId);

    Vector2 pivot = v2_mulf(getPivot(sprite->pivot), -1);
    xform = m4_translate(xform, v3(pivot.x * size.x, pivot.y * size.y, 0));

    draw_image_xform(sprite->image, xform, size, color);
}