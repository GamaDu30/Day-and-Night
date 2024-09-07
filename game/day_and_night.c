#pragma region DEFINE

#define MAX_ENTITIES_COUNT 1024
#define MAX_IMAGES_COUNT 1024
#define MAX_ITEMS_COUNT 1024

#pragma endregion DEFINE

#pragma region ENUM

typedef enum EntityType
{
	ENTITY_nil = 0,
	ENTITY_mineral = 1,
	ENTITY_tree = 2,
	ENTITY_player = 3,
	ENTITY_item = 4,
} EntityType;

typedef enum SpriteId
{
	SPRITE_player,
	SPRITE_tree,
	SPRITE_log,
	SPRITE_mineral,
	SPRITE_iron,
	SPRITE_MAX
} SpriteId;

typedef enum ItemId
{
	ITEM_nil,
	ITEM_mineral1,
	ITEM_log1,
	ITEM_mineral2,
	ITEM_log2,
	ITEM_mineral3,
	ITEM_log3,
	ITEM_MAX
} ItemId;

#pragma endregion ENUM

#pragma region STRUCT

typedef struct Entity
{
	bool isValid;
	EntityType type;
	Vector2 pos;

	SpriteId spriteId;

	int health;
	bool isDestroyable;
	bool isSelectable;
	bool isPickable;
	ItemId lootId;
	SpriteId lootSpriteId;
} Entity;

typedef struct World
{
	Entity entitites[MAX_ENTITIES_COUNT];
	Entity *selectedEntity;
} World;

typedef struct Sprite
{
	Gfx_Image *image;
} Sprite;

typedef struct Item
{

} Item;

#pragma endregion STRUCT

#pragma region GLOBAL

World *world = 0;
Sprite sprites[SPRITE_MAX];
Sprite item[ITEM_MAX];
const int tileSize = 15;
float camZoom = 5.0;
float entitySelectionRadius = tileSize * 0.5f;

#pragma endregion GLOBAL

#pragma region FUNCTION

Entity *entityCreate(EntityType type, SpriteId spriteId, Vector2 pos)
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
	entityFound->pos = pos;
	entityFound->spriteId = spriteId;
	entityFound->type = type;

	switch (type)
	{
	case ENTITY_player:
		entityFound->isDestroyable = false;
		entityFound->isSelectable = false;
		break;
	case ENTITY_mineral:
		entityFound->isDestroyable = true;
		entityFound->isSelectable = true;
		entityFound->lootId = ITEM_mineral1;
		entityFound->lootSpriteId = SPRITE_iron;
		break;
	case ENTITY_tree:
		entityFound->isDestroyable = true;
		entityFound->isSelectable = true;
		entityFound->lootId = ITEM_log1;
		entityFound->lootSpriteId = SPRITE_log;
		break;
	case ENTITY_item:
		entityFound->isDestroyable = false;
		entityFound->isSelectable = true;
		break;
	default:
		break;
	}

	return entityFound;
}

void destroyEntity(Entity *entity)
{
	entity->isValid = false;
	memset(entity, 0, sizeof(Entity));
}

Sprite *getSprite(SpriteId spriteId)
{
	if (spriteId >= 0 && spriteId < SPRITE_MAX)
	{
		return &sprites[spriteId];
	}

	assert(false, "getSprite: no sprite found");
}

void drawEntity(Entity *entity)
{
	Gfx_Image *image = getSprite(entity->spriteId)->image;
	Vector2 size = v2(image->width, image->height);

	Vector4 col = v4(1, 1, 1, 1);
	if (entity == world->selectedEntity)
	{
		col.g = 0.5f;
		col.b = 0.5f;
	}

	Matrix4 xform = m4_scalar(1.0);
	xform = m4_translate(xform, v3(entity->pos.x - size.x * 0.5, entity->pos.y, 0.0));

	if (entity->type == ENTITY_item)
	{
		xform = m4_translate(xform, v3(0.0, sin(os_get_elapsed_seconds() * 3) + 4, 0.0));
		draw_image_xform(image, m4_translate(xform, v3(0.0, -5.0, 0.0)), size, v4(0, 0, 0, 0.5));
	}

	draw_image_xform(image, xform, size, col);
}

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

boolean colAABBPoint(Vector2 pos, Vector2 size, Vector2 point)
{
	pos.x -= size.x * 0.5;
	boolean res = point.x > pos.x &&
				  point.x < pos.x + size.x &&
				  point.y > pos.y &&
				  point.y < pos.y + size.y;

	draw_rect(pos, size, res ? v4(1, 0, 0, 0.5) : v4(1, 1, 1, 0.5));

	return res;
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

void manageMouseClick()
{
	Entity *selectedEntity = world->selectedEntity;

	if (selectedEntity)
	{
		if (selectedEntity->isDestroyable)
		{
			selectedEntity->health--;
			play_one_audio_clip(STR("assets/sounds/EntityHit.wav"));

			if (selectedEntity->health <= 0)
			{
				if (selectedEntity->lootId)
				{
					entityCreate(ENTITY_item, selectedEntity->lootSpriteId, selectedEntity->pos);
				}

				play_one_audio_clip(STR("assets/sounds/EntityDestroy.wav"));
				destroyEntity(selectedEntity);
			}
		}
		else if (selectedEntity->isPickable)
		{
		}
	}
}

void createSprite(SpriteId spriteId, string path)
{
	sprites[spriteId] = (Sprite){.image = load_image_from_disk(path, get_heap_allocator())};
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

#pragma region INIT
	createSprite(SPRITE_player, STR("assets/images/player.png"));
	createSprite(SPRITE_tree, STR("assets/images/ressource_tree0.png"));
	createSprite(SPRITE_log, STR("assets/images/item_tree0.png"));
	createSprite(SPRITE_mineral, STR("assets/images/ressource_mineral0.png"));
	createSprite(SPRITE_iron, STR("assets/images/item_mineral0.png"));

	world = alloc(get_heap_allocator(), sizeof(World));

	Entity *player = entityCreate(ENTITY_player, SPRITE_player, v2(0, 0));
	player->health = 100;

	for (int i = 0; i < 10; i++)
	{
		Vector2 pos = v2(get_random_int_in_range(-5, 5), get_random_int_in_range(-5, 5));
		pos = tileToWorldPos(pos);
		pos = v2_add(pos, v2(tileSize * 0.5f, tileSize * 0.25f));
		Entity *mineral = entityCreate(ENTITY_mineral, SPRITE_mineral, pos);
		mineral->health = 5;
	}

	for (int i = 0; i < 15; i++)
	{
		Vector2 pos = v2(get_random_int_in_range(-10, 10), get_random_int_in_range(-10, 10));
		pos = tileToWorldPos(pos);
		pos = v2_add(pos, v2(tileSize * 0.5f, tileSize * 0.25f));
		Entity *tree = entityCreate(ENTITY_tree, SPRITE_tree, pos);
		tree->health = 5;
	}

	float seconds_counter = 0.0;
	s32 frame_count = 0;

	float64 last_time = os_get_elapsed_seconds();

	Vector2 camPos;

	Gfx_Font *font = load_font_from_disk(STR("C:/windows/fonts/arial.ttf"), get_heap_allocator());
	assert(font, "Failed loading arial.ttf");

	Vector2 mousePosTile;

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
			if (curEntity->isValid && curEntity->isSelectable)
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
			consume_key_just_pressed(MOUSE_BUTTON_LEFT);
			manageMouseClick();
		}

		if (is_key_just_pressed(KEY_ESCAPE))
		{
			window.should_close = true;
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

		for (int i = 0; i < MAX_ENTITIES_COUNT; i++)
		{
			Entity *curEntity = &world->entitites[i];
			if (curEntity->isValid)
			{
				drawEntity(curEntity);
			}
		}

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