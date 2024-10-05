#pragma region DEFINE

#define MAX_ENTITIES_COUNT 1024
#define MAX_IMAGES_COUNT 1024
#define MAX_ITEMS_COUNT 1024

#define INV_COUNT 15
#define STACK_SIZE 64
#define INV_CELL_MARGIN 8
#define INV_CELL_SIZE 64
#define INV_UI_WIDTH 0.5
#define INV_UI_HEIGHT 0.5
#define INV_UI_INNER_MARGIN_X 0.15
#define INV_UI_INNER_MARGIN_Y 0.25
#define HOTBAR_AMOUNT 5

#pragma endregion DEFINE

#pragma region INCLUDE

#include "game/sprite.c"
#include "game/item.c"
#include "game/entity.c"
#include "game/item.c"

#pragma endregion INCLUDE

#pragma region ENUM

#pragma endregion ENUM

#pragma region STRUCT

#pragma endregion STRUCT

#pragma region GLOBAL

const int tileSize = 15;
float camZoom = 5.0;
float entitySelectionRadius = tileSize * 0.5f;

Gfx_Font *font;

#pragma endregion GLOBAL

#pragma region FUNCTION

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
	return v2_mulf(v2(floor(worldPos.x / tileSize), floor(worldPos.y / tileSize)), tileSize);
}

Vector2 tileToWorldPos(Vector2 tilePos)
{
	return v2_mulf(tilePos, tileSize);
}

Vector2 getMousePosInNDC()
{
	return v2(input_frame.mouse_x / (window.width * 0.5) - 1.0f, input_frame.mouse_y / (window.height * 0.5) - 1.0f);
}

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

void drawGround(Vector2 origin, Vector2 size)
{
	Vector2 tilePos = worldToTilePos(origin);
	Vector2 offset = v2((size.x * tileSize) / 2, (size.y * tileSize) / 2);

	for (int y = tilePos.y - size.y * tileSize; y < tilePos.y + size.y * tileSize; y += tileSize)
	{
		for (int x = tilePos.x - size.x * tileSize; x < tilePos.x + size.x * tileSize; x += tileSize)
		{
			Vector2 pos = v2(x, y);
			Vector4 col = v4(0.2f, 0.2f, 0.5f, 0.75f);
			if ((x / tileSize + y / tileSize) % 2 == 0)
			{
				col.r += 0.1f;
				col.g += 0.1f;
			}

			draw_rect(pos, v2(tileSize, tileSize), col);
		}
	}
}

bool addItemToInventoryAtIndex(ItemType type, int id)
{
	Item *curItem = &world->inventory[id];
	if (curItem->type)
	{
		if (curItem->type == type && curItem->amount < STACK_SIZE)
		{
			// Item already present, we add amount
			curItem->amount++;
			return true;
		}
		else
		{
			// Can't place, items are different type
			return false;
		}
	}

	// Current inventory cell is empty
	curItem->type = type;
	curItem->amount = 1;
	curItem->grabbed = false;
	return true;
}

bool addItemToInventory(ItemType type)
{
	// Searching for the item in the inventory
	for (int i = 0; i < INV_COUNT; i++)
	{
		if (addItemToInventoryAtIndex(type, i))
		{
			return true;
		}
	}

	// Inventory is full
	return false;
}

void removeItemFromInventory(Item *item)
{
	for (int i = 0; i < INV_COUNT; i++)
	{
		if (&world->inventory[i] == item)
		{
			world->inventory[i].type = 0;
		}
	}
}

void drawItemCell(Item *item, Matrix4 xformCell, int id)
{
	// Cell
	Draw_Quad *quad = draw_rect_xform(xformCell, v2(INV_CELL_SIZE, INV_CELL_SIZE), v4(0.5, 0.25, 0.5, 0.9));
	Vector2 finalSize = v2(INV_CELL_SIZE, INV_CELL_SIZE);

	// Mouse click detection
	Vector2 mousePosNDC = getMousePosInNDC();
	boolean hovered = colAABBPoint(quad->bottom_left, v2(quad->top_right.x - quad->bottom_left.x, quad->top_right.y - quad->bottom_left.y), mousePosNDC);

	if (hovered)
	{
		if (is_key_just_pressed(MOUSE_BUTTON_LEFT))
		{
			if (!world->heldItem)
			{
				if (item->type)
				{
					world->heldItem = item;
					world->heldItem->grabbed = true;
					world->heldItemOriginId = id;
				}
			}
			else
			{
				if (world->heldItem == item)
				{
					world->heldItem->grabbed = false;
					world->heldItem = 0;
				}
				else
				{
					Item *itemTemp = item;
					addItemToInventoryAtIndex(world->heldItem->type, id);
					world->heldItem = itemTemp;
					world->heldItem->grabbed = false;
					world->heldItem = 0;
					world->inventory[world->heldItemOriginId].type = 0;
				}
			}
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

	// Pivot
	Vector2 pivot = v2_mulf(getPivot(sprites[getItemSpriteId(item->type)].pivot), -1);
	xform_item = m4_translate(xform_item, v3(pivot.x * finalSize.x, pivot.y * finalSize.y, 0));

	draw_image_xform(sprite->image, m4_translate(xform_item, v3(0, -5, 0.0f)), finalSize, v4(0, 0, 0, 0.5));
	draw_image_xform(sprite->image, xform_item, finalSize, item->grabbed ? v4(1, 1, 1, 0.5) : COLOR_WHITE);

	// Amount text
	Matrix4 xform_itemCount = m4_scale(xformCell, v3(0.5, 0.5, 1));
	xform_itemCount = m4_translate(xform_itemCount, v3(INV_CELL_SIZE * 0.1, INV_CELL_SIZE * 0.1, 1));
	draw_text_xform(font, tprint("%ix", item->amount), 48, m4_translate(xform_itemCount, v3(3, -3, 0)), v2(1, 1), COLOR_BLACK);
	draw_text_xform(font, tprint("%ix", item->amount), 48, xform_itemCount, v2(1, 1), COLOR_WHITE);
}

void drawInventory()
{
	// Background
	Vector2 inventoryGlobalSize = v2(window.width * INV_UI_WIDTH, window.height * INV_UI_HEIGHT);
	draw_rect_xform(m4_translate(draw_frame.camera_xform, v3(-inventoryGlobalSize.x * 0.5f, -inventoryGlobalSize.y * 0.5f, 0)), inventoryGlobalSize, v4(0.25f, 0.25f, 0.25f, 0.9f));

	Vector2 innerSize = v2_mul(inventoryGlobalSize, v2(1.0f - INV_UI_INNER_MARGIN_X, 1.0f - INV_UI_INNER_MARGIN_Y));
	// draw_rect_xform(m4_translate(draw_frame.camera_xform, v3(-innerSize.x * 0.5f, -innerSize.y * 0.5f, 0)), innerSize, v4(0.5f, 0.5f, 0.5f, 0.8f));

	// Items
	int columnSize = floor(innerSize.x / (INV_CELL_SIZE + INV_CELL_MARGIN));

	Vector2 origin;
	origin.x = (innerSize.x - ((INV_CELL_SIZE + INV_CELL_MARGIN) * columnSize - INV_CELL_MARGIN)) * 0.5;
	origin.y = (innerSize.y - ((INV_CELL_SIZE + INV_CELL_MARGIN) * ceil(INV_COUNT / (float)columnSize) - INV_CELL_MARGIN)) * 0.5;

	Matrix4 xform_inner = m4_translate(draw_frame.camera_xform, v3(-innerSize.x * 0.5f, -innerSize.y * 0.5f, 0.0f));

	for (int i = 0; i < INV_COUNT; i++)
	{
		Vector2 cellPos;
		cellPos.x = origin.x + (i % columnSize) * (INV_CELL_SIZE + INV_CELL_MARGIN);
		cellPos.y = innerSize.y - INV_CELL_SIZE - (i / columnSize) * (INV_CELL_SIZE + INV_CELL_MARGIN) - origin.y;

		drawItemCell(&world->inventory[i], m4_translate(xform_inner, v3(cellPos.x, cellPos.y, 0.0f)), i);
	}
}

void drawHotBar()
{
	float hotBarWidth = (INV_CELL_SIZE + INV_CELL_MARGIN) * HOTBAR_AMOUNT - INV_CELL_MARGIN;

	Matrix4 xformHotBar = m4_translate(draw_frame.camera_xform, v3(-hotBarWidth * 0.5, -window.height * 0.45, 0));

	for (int i = 0; i < HOTBAR_AMOUNT; i++)
	{
		drawItemCell(&world->hotbar[i], xformHotBar, i);
		xformHotBar = m4_translate(xformHotBar, v3(INV_CELL_SIZE + INV_CELL_MARGIN, 0, 0));
	}
}

void checkMouseClickEntity()
{
	if (world->selectedEntity)
	{
		Entity *selectedEntity = world->selectedEntity;

		EntityData *entityData = getEntityData(selectedEntity->type);
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
					newEntity->itemType = entityData->lootType;
				}

				play_one_audio_clip(STR("assets/sounds/EntityDestroy.wav"));
				destroyEntity(selectedEntity);
			}
		}
		else if (entityData->isPickable)
		{
			addItemToInventory(selectedEntity->itemType);
			destroyEntity(selectedEntity);
		}
	}
}

#pragma endregion FUNCTION

#pragma region ENTRY

int entry(int argc, char **argv)
{
	window.title = STR("Day and Night");
	window.width = 1280;
	window.height = 720;
	window.x = (1920 - window.width) / 2;
	window.y = (1080 - window.height) / 2;
	window.clear_color = hex_to_rgba(0x2A2A38ff);
	window.force_topmost = false;

#pragma region INIT
	world = alloc(get_heap_allocator(), sizeof(World));

	initSprites();
	initItems();
	initEntity();

	Entity *player = createEntity();
	setupEntity(player, ENTITY_player, v2(0, 0));
	player->health = 100;

	for (int i = 0; i < 10; i++)
	{
		Vector2 pos = v2(get_random_int_in_range(-5, 5), get_random_int_in_range(-5, 5));
		pos = tileToWorldPos(pos);
		pos = v2_add(pos, v2(tileSize * 0.5f, tileSize * 0.25f));
		Entity *mineral = createEntity();
		setupEntity(mineral, ENTITY_mineral, pos);

		mineral->health = 5;
	}

	for (int i = 0; i < 15; i++)
	{
		Vector2 pos = v2(get_random_int_in_range(-10, 10), get_random_int_in_range(-10, 10));
		pos = tileToWorldPos(pos);
		pos = v2_add(pos, v2(tileSize * 0.5f, tileSize * 0.25f));
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

#pragma endregion INIT

#pragma region MAIN_LOOP
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
		draw_frame.camera_xform = m4_mul(draw_frame.camera_xform, m4_make_scale(v3(1.0 / camZoom, 1.0 / camZoom, 1)));

		// Update
		os_update();

		Vector2 mouseWorld = screenToWorld(v2(input_frame.mouse_x, input_frame.mouse_y));

		// Entity Selection
		float distMin = 1000;
		world->selectedEntity = 0;
		for (int i = 0; i < MAX_ENTITIES_COUNT; i++)
		{
			Entity *curEntity = &world->entitites[i];
			EntityData *entityData = getEntityData(curEntity->type);
			if (curEntity->isValid && entityData->isSelectable)
			{
				float distCur = v2_length(v2_sub(v2_add(curEntity->pos, v2(0, tileSize * 0.25f)), mouseWorld));
				if (distCur < entitySelectionRadius && distCur < distMin)
				{
					distMin = distCur;
					world->selectedEntity = curEntity;
				}
			}
		}

		// Mouse Click
		if (is_key_just_pressed(MOUSE_BUTTON_LEFT))
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

		// Draw
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

#pragma endregion MAIN_LOOP

	return 0;
}

#pragma endregion ENTRY