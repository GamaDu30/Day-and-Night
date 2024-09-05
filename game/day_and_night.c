#pragma region DEFINE

#define MAX_ENTITIES_COUNT 1024
#define MAX_IMAGES_COUNT 1024

#pragma endregion DEFINE

#pragma region ENUM

typedef enum EntityType
{
	ENTITY_nil = 0,
	ENTITY_mineral = 1,
	ENTITY_tree = 2,
	ENTITY_player = 3,
} EntityType;

typedef enum SpriteId
{
	SPRITE_player,
	SPRITE_tree0,
	SPRITE_tree1,
	SPRITE_mineral0,
	SPRITE_mineral1,
	SPRITE_MAX
} SpriteId;
#pragma endregion ENUM

#pragma region STRUCT

typedef struct Entity
{
	bool isValid;
	EntityType type;
	Vector2 pos;

	SpriteId spriteId;
} Entity;

typedef struct World
{
	Entity entitites[MAX_ENTITIES_COUNT];
	Entity* selectedEntity;
} World;

typedef struct Sprite
{
	Gfx_Image* image;
} Sprite;

#pragma endregion STRUCT

#pragma region GLOBAL

World *world = 0;
Sprite sprites[SPRITE_MAX];
const int tileSize = 15;
float camZoom = 5.0;
float entitySelectionRadius = tileSize * 0.5f;

#pragma endregion GLOBAL

#pragma region FUNCTION

Entity* entityCreate(EntityType type, SpriteId spriteId, Vector2 pos)
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
	return entityFound;
}

void destroyEntity(Entity *entity)
{
	memset(entity, 0, sizeof(Entity));
	entity->isValid = false;
}

Sprite* getSprite(SpriteId spriteId)
{
	if (spriteId >= 0 && spriteId < SPRITE_MAX)
	{
		return &sprites[spriteId];
	}

	assert(false, "getSprite: no sprite found");
}

void drawSpriteAtPos(SpriteId spriteId, Vector2 pos, Vector4 color)
{
	Gfx_Image* image = getSprite(spriteId)->image;
	Vector2 size = v2(image->width, image->height);
	Matrix4 xform = m4_scalar(1.0);
	xform = m4_translate(xform, v3(pos.x, pos.y, 0.0f));
	xform = m4_translate(xform, v3(-size.x * 0.5, 0.0, 0.0f));
	draw_image_xform(image, xform, size, color);
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
	sprites[SPRITE_player] = (Sprite){.image = load_image_from_disk(STR("assets/player.png"), get_heap_allocator())};
	sprites[SPRITE_tree0] = (Sprite){.image = load_image_from_disk(STR("assets/tree0.png"), get_heap_allocator())};
	sprites[SPRITE_tree1] = (Sprite){.image = load_image_from_disk(STR("assets/tree1.png"), get_heap_allocator())};
	sprites[SPRITE_mineral0] = (Sprite){.image = load_image_from_disk(STR("assets/mineral0.png"), get_heap_allocator())};
	sprites[SPRITE_mineral1] = (Sprite){.image = load_image_from_disk(STR("assets/mineral1.png"), get_heap_allocator())};

	world = alloc(get_heap_allocator(), sizeof(World));

	Entity *player = entityCreate(ENTITY_player, SPRITE_player, v2(0, 0));

	for (int i = 0; i < 10; i++)
	{
		Vector2 pos = v2(get_random_int_in_range(-5, 5), get_random_int_in_range(-5, 5));
		pos = tileToWorldPos(pos);
		pos = v2_add(pos, v2(tileSize * 0.5f, tileSize * 0.25f));
		Entity *mineral = entityCreate(ENTITY_mineral, SPRITE_mineral0, pos);
	}

	for (int i = 0; i < 15; i++)
	{
		Vector2 pos = v2(get_random_int_in_range(-10, 10), get_random_int_in_range(-10, 10));
		pos = tileToWorldPos(pos);
		pos = v2_add(pos, v2(tileSize * 0.5f, tileSize * 0.25f));
		Entity *tree = entityCreate(ENTITY_tree, SPRITE_tree0, pos);
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

		//Delta Time
		float64 now = os_get_elapsed_seconds();
		float64 dt = now - last_time;
		last_time = now;

		//Camera
		camPos = lerp_v2(camPos, player->pos, dt * 10);
		draw_frame.camera_xform = m4_make_scale(v3(1.0, 1.0, 1.0));
		draw_frame.camera_xform = m4_mul(draw_frame.camera_xform, m4_make_translation(v3(camPos.x, camPos.y, 0)));
		draw_frame.camera_xform = m4_mul(draw_frame.camera_xform, m4_make_scale(v3(1.0 / camZoom, 1.0 / camZoom, 1)));

		//Update
		os_update();

		Vector2 mouseWorld = screenToWorld(v2(input_frame.mouse_x, input_frame.mouse_y));

		float distMin = 1000;
		world->selectedEntity = 0;
		for (int i = 0; i < MAX_ENTITIES_COUNT; i++)
		{
			Entity *curEntity = &world->entitites[i];
			if (curEntity->isValid)
			{
				float distCur = v2_length(v2_sub(v2_add(curEntity->pos, v2(0, tileSize * 0.25f)), mouseWorld));
				if (distCur < entitySelectionRadius && distCur < distMin)
				{
					distMin = distCur;
					world->selectedEntity = curEntity;
				}
			}
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

		//Draw
		drawGround(player->pos, v2(10, 6));

		for (int i = 0; i < MAX_ENTITIES_COUNT; i++)
		{
			Entity *curEntity = &world->entitites[i];
			if (curEntity->isValid)
			{
				if (curEntity == world->selectedEntity)
				{
					draw_rect(worldToTilePos(mouseWorld), v2(tileSize, tileSize), v4(0.5f, 0.f, 0.f, 0.25f));
				}

				drawSpriteAtPos(curEntity->spriteId, curEntity->pos, COLOR_WHITE);
			
				// Gfx_Image* image = getSprite(curEntity->spriteId)->image;
				// colAABBPoint(curEntity->pos, v2(image->width, image->height), screenToWorld(v2(input_frame.mouse_x, input_frame.mouse_y)));
			}
		}

		gfx_update();

		//FPS
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