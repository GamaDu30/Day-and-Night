//: defines
// Memory limit
#define MAX_ENTITIES_COUNT 1024
#define MAX_IMAGES_COUNT 1024
#define MAX_ITEMS_COUNT 1024

// World const
#define TILE_SIZE 15
#define CAM_ZOOM 5
#define ENT_SELECT_RADIUS (TILE_SIZE * 0.5f)

// Inventory
#define INV_COUNT 15
#define INV_STACK_SIZE 10
#define INV_CELL_MARGIN 8
#define INV_CELL_SIZE 64
#define INV_UI_WIDTH 0.5
#define INV_UI_HEIGHT 0.5
#define INV_UI_INNER_MARGIN_X 0.15
#define INV_UI_INNER_MARGIN_Y 0.25
#define INV_HOTBAR_AMOUNT 5

//: enums
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

typedef enum EntityType
{
	ENTITY_nil,
	ENTITY_player,
	ENTITY_mineral,
	ENTITY_tree,
	ENTITY_item,
	ENTITY_MAX,
} EntityType;

typedef enum ItemType
{
	ITEM_nil,
	ITEM_iron,
	ITEM_log,
	ITEM_MAX
} ItemType;

typedef enum UXState
{
	UX_nil,
	UX_inventory,
} UXState;

//: structs
typedef struct Sprite
{
	Gfx_Image *image;
	Pivot pivot;
} Sprite;

typedef struct ItemData
{
	SpriteId spriteId;
} ItemData;

typedef struct Item
{
	ItemType type;
	int amount;
} Item;

typedef struct EntityData
{
	SpriteId spriteId;

	bool isDestroyable;
	bool isSelectable;
	bool isPickable;

	ItemType lootType;
} EntityData;

typedef struct Entity
{
	bool isValid;

	EntityType entityType;
	ItemType itemType;
	SpriteId spriteId;

	Vector2 pos;
	int health;

	int amount;
} Entity;

typedef struct World
{
	Entity entitites[MAX_ENTITIES_COUNT];
	Entity *selectedEntity;
	Item *inventory[INV_COUNT];
	Item hotbar[INV_HOTBAR_AMOUNT];
	Item *heldItem;
	int heldItemOriginId;
	UXState uxState;
} World;

//: global
World *world = 0;
Sprite sprites[SPRITE_MAX] = {0};
ItemData itemsData[ITEM_MAX] = {0};
EntityData entityData[ENTITY_MAX] = {0};
Gfx_Font *font = 0;
Entity *player = 0;

//: Foward declarations
Entity *createEntity();
void setupEntity(Entity *entity, EntityType type, Vector2 pos);

//: collision
boolean colAABBPoint(Vector2 pos, Vector2 size, Vector2 point)
{
	boolean res = point.x > pos.x &&
				  point.x < pos.x + size.x &&
				  point.y > pos.y &&
				  point.y < pos.y + size.y;

	draw_rect(pos, size, res ? v4(1, 0, 0, 0.5) : v4(1, 1, 1, 0.5));

	return res;
}

boolean colCircleCircle(Vector2 p1, float r1, Vector2 p2, float r2)
{
	float dist = v2_length(v2_sub(p1, p2));
	return dist < r1 + r2;
}

//: sprite

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

//: item

void initItems()
{
	itemsData[ITEM_iron] = (ItemData){.spriteId = SPRITE_ITEM_iron};
	itemsData[ITEM_log] = (ItemData){.spriteId = SPRITE_ITEM_log};
}

ItemData getItemData(ItemType type)
{
	return itemsData[type];
}

//: inventory
Vector2 getMousePosInNDC()
{
	return v2(input_frame.mouse_x / (window.width * 0.5) - 1.0f, input_frame.mouse_y / (window.height * 0.5) - 1.0f);
}

void destroyEntity(Entity *entity)
{
	entity->isValid = false;
	memset(entity, 0, sizeof(Entity));
}

bool addItemToInventoryAtIndex(struct Entity *itemEntity, int id)
{
	Item *curItem = world->inventory[id];
	if (curItem->type)
	{
		if (curItem->type == itemEntity->itemType)
		{
			if (curItem->amount >= INV_STACK_SIZE)
			{
				return false;
			}
			else if (curItem->amount + itemEntity->amount > INV_STACK_SIZE)
			{
				itemEntity->amount -= INV_STACK_SIZE - curItem->amount;
				curItem->amount = INV_STACK_SIZE;
				return false;
			}

			curItem->amount += itemEntity->amount;
			destroyEntity(itemEntity);
			return true;
		}

		// Can't place, items are different type
		return false;
	}
	else
	{
		// Current inventory cell is empty
		world->inventory[id]->type = itemEntity->itemType;
		world->inventory[id]->amount = itemEntity->amount;
		destroyEntity(itemEntity);

		return true;
	}
}

bool addItemToInventory(Entity *itemEntity)
{
	for (int i = 0; i < INV_COUNT; i++)
	{
		if (world->inventory[i]->type == itemEntity->itemType || world->inventory[i]->type == 0)
		{
			if (addItemToInventoryAtIndex(itemEntity, i))
			{
				return true;
			}
		}
	}

	return false;
}

void removeItemFromInventory(Item *item)
{
	for (int i = 0; i < INV_COUNT; i++)
	{
		if (world->inventory[i] == item)
		{
			world->inventory[i]->type = 0;
		}
	}
}

void drawItemCell(Item *item, Matrix4 xformCell, int id)
{
	// Cell
	Draw_Quad *quad = draw_rect_xform(xformCell, v2(INV_CELL_SIZE, INV_CELL_SIZE), v4(0.5, 0.25, 0.5, 0.9));
	Vector2 finalSize = v2(INV_CELL_SIZE, INV_CELL_SIZE);

	// Mouse hover detection
	boolean hovered = colAABBPoint(quad->bottom_left, v2(quad->top_right.x - quad->bottom_left.x, quad->top_right.y - quad->bottom_left.y), getMousePosInNDC());

	// Mouse click check
	if (hovered)
	{
		Item *heldItem = world->heldItem;

		if (is_key_just_pressed(MOUSE_BUTTON_LEFT))
		{

			if (!heldItem->type)
			{
				// No held item, setting one
				heldItem->type = item->type;
				heldItem->amount = item->amount;

				item->type = ITEM_nil;
				item->amount = 0;
			}
			else
			{
				if (!item->type || item->type == heldItem->type)
				{
					// No item or same item
					// Setting the type to the one of the held item and adding its amount
					item->type = heldItem->type;
					if (item->amount + heldItem->amount <= INV_STACK_SIZE)
					{
						item->amount += heldItem->amount;
						heldItem->type = 0;
					}
					else
					{
						heldItem->amount -= INV_STACK_SIZE - item->amount;
						item->amount = INV_STACK_SIZE;
					}
				}
				else
				{
					// Different items, need to swap them
					ItemType tempType = item->type;
					item->type = heldItem->type;
					heldItem->type = tempType;

					ItemType tempAmount = item->amount;
					item->amount = heldItem->amount;
					heldItem->amount = tempAmount;
				}
			}
		}
		else if (is_key_just_pressed(MOUSE_BUTTON_RIGHT) && item->type)
		{
			heldItem->type = item->type;
			heldItem->amount = (item->amount >> 1) + item->amount % 2;
			item->amount = item->amount >> 1;

			if (!item->amount)
			{
				item->type = ITEM_nil;
			}
		}
		else if (is_key_just_pressed('O') && item->type)
		{
			Entity *newEntity = createEntity();
			setupEntity(newEntity, ENTITY_item, player->pos);
			newEntity->spriteId = getItemData(item->type).spriteId;
			newEntity->itemType = item->type;
			newEntity->amount = item->amount;

			removeItemFromInventory(item);
		}
	}

	// If there is no item, there is no need to draw
	if (!item->type)
	{
		return;
	}

	Sprite *sprite = getSpriteFromId(itemsData[item->type].spriteId);
	Vector2 imageSize = v2(sprite->image->width, sprite->image->height);
	float innerSize = INV_CELL_SIZE * 0.75;
	float smallestScale = min(innerSize / imageSize.x, innerSize / imageSize.y);
	finalSize = v2(smallestScale * imageSize.x, smallestScale * imageSize.y);

	if (hovered)
	{
		finalSize = v2_mulf(finalSize, 1.25);
	}

	// Item
	Matrix4 xform_item = m4_translate(xformCell, v3(INV_CELL_SIZE * 0.5, INV_CELL_SIZE * 0.5, 0.0));

	drawSprite(itemsData[item->type].spriteId, m4_translate(xform_item, v3(0, -5, 0.0f)), finalSize, v4(0, 0, 0, 0.5));
	drawSprite(itemsData[item->type].spriteId, xform_item, finalSize, COLOR_WHITE);

	// Amount text
	Matrix4 xform_itemCount = m4_scale(xformCell, v3(0.5, 0.5, 1));
	xform_itemCount = m4_translate(xform_itemCount, v3(INV_CELL_SIZE * 0.1, INV_CELL_SIZE * 0.1, 1));
	draw_text_xform(font, tprint("%ix", item->amount), 48, m4_translate(xform_itemCount, v3(3, -3, 0)), v2(1, 1), COLOR_BLACK);
	draw_text_xform(font, tprint("%ix", item->amount), 48, xform_itemCount, v2(1, 1), COLOR_WHITE);
}

void drawHeldItem(Item *heldItem, Matrix4 cameraTransform)
{
	SpriteId spriteId = getItemData(heldItem->type).spriteId;
	Vector2 spriteSize = getspriteSize(spriteId);
	Matrix4 xform = m4_translate(cameraTransform, v3(input_frame.mouse_x - window.width * 0.5f, input_frame.mouse_y - window.height * 0.5f, 0));
	drawSprite(spriteId, xform, v2(INV_CELL_SIZE * 0.75f, INV_CELL_SIZE * 0.75f), v4(1, 1, 1, 0.5f));
}

void drawInventory()
{
	// Background
	Vector2 inventorySize = v2(window.width * INV_UI_WIDTH, window.height * INV_UI_HEIGHT);
	Matrix4 bgTransform = m4_translate(draw_frame.camera_xform, v3(-inventorySize.x * 0.5f, -inventorySize.y * 0.5f, 0));
	draw_rect_xform(bgTransform, inventorySize, v4(0.25f, 0.25f, 0.25f, 0.9f));

	// Inner Area
	Vector2 innerSize = v2_mul(inventorySize, v2(1.0f - INV_UI_INNER_MARGIN_X, 1.0f - INV_UI_INNER_MARGIN_Y));
	Matrix4 innerTransform = m4_translate(draw_frame.camera_xform, v3(-innerSize.x * 0.5f, -innerSize.y * 0.5f, 0));

	// Calculate Grid
	int columnSize = floor(innerSize.x / (INV_CELL_SIZE + INV_CELL_MARGIN));
	Vector2 origin = v2(
		(innerSize.x - ((INV_CELL_SIZE + INV_CELL_MARGIN) * columnSize - INV_CELL_MARGIN)) * 0.5f,
		(innerSize.y - ((INV_CELL_SIZE + INV_CELL_MARGIN) * ceil(INV_COUNT / (float)columnSize) - INV_CELL_MARGIN)) * 0.5f);

	// Draw Inventory Items
	for (int i = 0; i < INV_COUNT; i++)
	{
		Vector2 cellPos = v2(
			origin.x + (i % columnSize) * (INV_CELL_SIZE + INV_CELL_MARGIN),
			innerSize.y - INV_CELL_SIZE - (i / columnSize) * (INV_CELL_SIZE + INV_CELL_MARGIN) - origin.y);
		drawItemCell(world->inventory[i], m4_translate(innerTransform, v3(cellPos.x, cellPos.y, 0)), i);
	}

	// Draw Held Item
	if (world->heldItem->type)
	{
		drawHeldItem(world->heldItem, draw_frame.camera_xform);
	}
}

void drawHotBar()
{
	float hotBarWidth = (INV_CELL_SIZE + INV_CELL_MARGIN) * INV_HOTBAR_AMOUNT - INV_CELL_MARGIN;

	Matrix4 xformHotBar = m4_translate(draw_frame.camera_xform, v3(-hotBarWidth * 0.5, -window.height * 0.45, 0));

	for (int i = 0; i < INV_HOTBAR_AMOUNT; i++)
	{
		drawItemCell(&world->hotbar[i], xformHotBar, i);
		xformHotBar = m4_translate(xformHotBar, v3(INV_CELL_SIZE + INV_CELL_MARGIN, 0, 0));
	}
}

//: entity

void initEntity()
{
	entityData[ENTITY_player] = (EntityData){.spriteId = SPRITE_player};
	entityData[ENTITY_mineral] = (EntityData){.spriteId = SPRITE_mineral, .isDestroyable = true, .isSelectable = true, .lootType = ITEM_iron};
	entityData[ENTITY_tree] = (EntityData){.spriteId = SPRITE_tree, .isDestroyable = true, .isSelectable = true, .lootType = ITEM_log};
	entityData[ENTITY_item] = (EntityData){.isSelectable = true, .isPickable = true};

	for (int i = 0; i < INV_COUNT; i++)
	{
		world->inventory[i] = alloc(get_heap_allocator(), sizeof(Item));
	}

	world->heldItem = alloc(get_heap_allocator(), sizeof(Item));
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
		entity->amount = 3;
	}
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

//: global functions
float lerp_f(float a, float b, float t)
{
	return a + (b - a) * t;
}

Vector2 lerp_v2(Vector2 a, Vector2 b, float t)
{
	return v2(lerp_f(a.x, b.x, t), lerp_f(a.y, b.y, t));
}

Vector2 screenToWorld(Vector2 screenPos)
{
	Vector2 posNorm = v2(screenPos.x / (window.width * 0.5) - 1.0f, screenPos.y / (window.height * 0.5) - 1.0f);
	Vector4 posWorld = v4(posNorm.x, posNorm.y, 0, 1);
	posWorld = m4_transform(m4_inverse(draw_frame.projection), posWorld);
	posWorld = m4_transform(draw_frame.camera_xform, posWorld);

	return v2(posWorld.x, posWorld.y);
}

Vector2 worldToTilePos(Vector2 worldPos)
{
	return v2_mulf(v2(floor(worldPos.x / TILE_SIZE), floor(worldPos.y / TILE_SIZE)), TILE_SIZE);
}

Vector2 tileToWorldPos(Vector2 tilePos)
{
	return v2_mulf(tilePos, TILE_SIZE);
}

void checkMouseClickEntity()
{
	if (world->selectedEntity)
	{
		Entity *selectedEntity = world->selectedEntity;

		EntityData *entityData = getEntityData(selectedEntity->entityType);
		if (entityData->isDestroyable)
		{
			selectedEntity->health--;
			play_one_audio_clip(STR("assets/sounds/EntityHit.wav"));

			if (selectedEntity->health <= 0)
			{
				if (entityData->lootType)
				{
					Entity *newEntity = createEntity();
					setupEntity(newEntity, ENTITY_item, selectedEntity->pos);
					newEntity->spriteId = getItemData(entityData->lootType).spriteId;
					newEntity->itemType = entityData->lootType;
					newEntity->amount = 3;
				}

				play_one_audio_clip(STR("assets/sounds/EntityDestroy.wav"));
				destroyEntity(selectedEntity);
			}
		}
		else if (entityData->isPickable)
		{
			addItemToInventory(selectedEntity);
		}
	}
}

void drawGround(Vector2 origin, Vector2 size)
{
	Vector2 tilePos = worldToTilePos(origin);
	Vector2 offset = v2((size.x * TILE_SIZE) / 2, (size.y * TILE_SIZE) / 2);

	for (int y = tilePos.y - size.y * TILE_SIZE; y < tilePos.y + size.y * TILE_SIZE; y += TILE_SIZE)
	{
		for (int x = tilePos.x - size.x * TILE_SIZE; x < tilePos.x + size.x * TILE_SIZE; x += TILE_SIZE)
		{
			Vector2 pos = v2(x, y);
			Vector4 col = v4(0.2f, 0.2f, 0.5f, 0.75f);
			if ((x / TILE_SIZE + y / TILE_SIZE) % 2 == 0)
			{
				col.r += 0.1f;
				col.g += 0.1f;
			}

			draw_rect(pos, v2(TILE_SIZE, TILE_SIZE), col);
		}
	}
}

//: entry
int entry(int argc, char **argv)
{

	//: init
	window.title = STR("Day and Night");
	window.width = 1280;
	window.height = 720;
	window.x = (1920 - window.width) / 2;
	window.y = (1080 - window.height) / 2;
	window.clear_color = hex_to_rgba(0x2A2A38ff);
	window.force_topmost = false;

	world = alloc(get_heap_allocator(), sizeof(World));

	initSprites();
	initItems();
	initEntity();

	player = createEntity();
	setupEntity(player, ENTITY_player, v2(0, 0));
	player->health = 100;

	for (int i = 0; i < 10; i++)
	{
		Vector2 pos = v2(get_random_int_in_range(-5, 5), get_random_int_in_range(-5, 5));
		pos = tileToWorldPos(pos);
		pos = v2_add(pos, v2(TILE_SIZE * 0.5f, TILE_SIZE * 0.25f));
		Entity *mineral = createEntity();
		setupEntity(mineral, ENTITY_mineral, pos);

		mineral->health = 5;
	}

	for (int i = 0; i < 15; i++)
	{
		Vector2 pos = v2(get_random_int_in_range(-10, 10), get_random_int_in_range(-10, 10));
		pos = tileToWorldPos(pos);
		pos = v2_add(pos, v2(TILE_SIZE * 0.5f, TILE_SIZE * 0.25f));
		Entity *tree = createEntity();
		setupEntity(tree, ENTITY_tree, pos);
		tree->health = 5;
	}

	float seconds_counter = 0.0;
	s32 frame_count = 0;
	float64 last_time = os_get_elapsed_seconds();
	Vector2 camPos = v2(0, 0);
	Vector2 mousePosTile;
	font = load_font_from_disk(STR("C:/windows/fonts/arial.ttf"), get_heap_allocator());
	assert(font, "Failed loading arial.ttf");

	//: update
	while (!window.should_close)
	{
		reset_temporary_storage();
		draw_frame_reset(&draw_frame);

		// Delta Time
		float64 now = os_get_elapsed_seconds();
		float64 dt = now - last_time;
		last_time = now;

		// Camera
		camPos = lerp_v2(camPos, player->pos, dt * 10);
		if (v2_length(v2_sub(player->pos, camPos)) < 0.01)
		{
			camPos = player->pos;
		}
		draw_frame.camera_xform = m4_make_scale(v3(1.0, 1.0, 1.0));
		draw_frame.camera_xform = m4_mul(draw_frame.camera_xform, m4_make_translation(v3(camPos.x, camPos.y, 0)));
		draw_frame.camera_xform = m4_mul(draw_frame.camera_xform, m4_make_scale(v3(1.0 / CAM_ZOOM, 1.0 / CAM_ZOOM, 1)));

		// Update
		os_update();

		Vector2 mouseWorld = screenToWorld(v2(input_frame.mouse_x, input_frame.mouse_y));

		// Entity Selection
		if (world->uxState == UX_nil)
		{
			float distMin = 1000;
			world->selectedEntity = 0;
			for (int i = 0; i < MAX_ENTITIES_COUNT; i++)
			{
				Entity *curEntity = &world->entitites[i];
				EntityData *entityData = getEntityData(curEntity->entityType);
				if (curEntity->isValid && entityData->isSelectable)
				{
					float distCur = v2_length(v2_sub(v2_add(curEntity->pos, v2(0, TILE_SIZE * 0.25f)), mouseWorld));
					if (distCur < ENT_SELECT_RADIUS && distCur < distMin)
					{
						distMin = distCur;
						world->selectedEntity = curEntity;
					}
				}
			}
		}
		else
		{
			world->selectedEntity = 0;
		}

		//: Input
		if (is_key_just_pressed(MOUSE_BUTTON_LEFT) && world->uxState == UX_nil)
		{
			// consume_key_just_pressed(MOUSE_BUTTON_LEFT);
			checkMouseClickEntity();
		}

		if (is_key_just_pressed(KEY_ESCAPE))
		{
			window.should_close = true;
		}

		if (is_key_just_pressed('U'))
		{
			world->uxState = world->uxState == UX_inventory ? UX_nil : UX_inventory;
		}

		Vector2 input_axis = v2(0, 0);
		if (is_key_down('J'))
		{
			input_axis.x -= 1;
		}
		if (is_key_down('L'))
		{
			input_axis.x += 1;
		}
		if (is_key_down('I'))
		{
			input_axis.y += 1;
		}
		if (is_key_down('K'))
		{
			input_axis.y -= 1;
		}

		input_axis = v2_normalize(input_axis);
		player->pos = v2_add(player->pos, v2_mulf(input_axis, 50.0 * dt));

		//: draw
		drawGround(player->pos, v2(10, 6));

		// Entity
		for (int i = 0; i < MAX_ENTITIES_COUNT; i++)
		{
			Entity *curEntity = &world->entitites[i];
			if (curEntity->isValid)
			{
				drawEntity(curEntity);
			}
		}

		if (world->uxState == UX_inventory)
		{
			drawInventory();
		}

		drawHotBar();

		gfx_update();

		// FPS
		seconds_counter += dt;
		frame_count++;
		if (seconds_counter > 1.0)
		{
			log("fps: %i", frame_count);
			seconds_counter = 0.0;
			frame_count = 0;
		}
	}

	return 0;
}