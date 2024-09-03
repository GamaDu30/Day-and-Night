typedef enum EntityArchetype
{
	arch_nil = 0,
	arch_mineral = 1,
	arch_tree = 2,
	arch_player = 3,
} EntityArchetype;

typedef struct Entity
{
	bool is_valid;
	EntityArchetype arch;
	Vector2 pos;

	bool render_sprite;
	Gfx_Image *sprite;
} Entity;

#define MAX_ENTITIES_COUNT 1024

typedef struct World
{
	Entity entitites[MAX_ENTITIES_COUNT];
} World;
World *world = 0;

Entity *entity_create()
{
	Entity *entity_found = 0;
	for (int i = 0; i < MAX_ENTITIES_COUNT; i++)
	{
		Entity *cur_entity = &world->entitites[i];
		if (!cur_entity->is_valid)
		{
			entity_found = cur_entity;
			break;
		}
	}

	assert(entity_found, "No entity found");
	entity_found->is_valid = true;
	return entity_found;
}

void entity_destroy(Entity *entity)
{
	memset(entity, 0, sizeof(Entity));
}

void setup_rock(Entity *entity)
{
	entity->arch = arch_mineral;
}

void setup_tree(Entity *entity)
{
	entity->arch = arch_tree;
}

void setup_player(Entity *entity)
{
	entity->arch = arch_player;
}

void draw_image_at_pos(Gfx_Image *image, Vector2 pos)
{
	Vector2 size = v2(image->width, image->height);
	Matrix4 xform = m4_scalar(1.0);
	xform = m4_translate(xform, v3(pos.x, pos.y, 0.0f));
	xform = m4_translate(xform, v3(-size.x * 0.5, 0.0, 0.0f));
	draw_image_xform(image, xform, size, COLOR_WHITE);
}

int entry(int argc, char **argv)
{
	window.title = STR("Day and Night");
	window.width = 1280;
	window.height = 720;
	window.x = (1920 - window.width) / 2;
	window.y = (1080 - window.height) / 2;
	window.clear_color = hex_to_rgba(0x2A2A38ff);

	world = alloc(get_heap_allocator(), sizeof(World));

	Gfx_Image *player = load_image_from_disk(STR("assets/player.png"), get_heap_allocator());
	assert(player, "player.png not found");
	Gfx_Image *tree0 = load_image_from_disk(STR("assets/tree0.png"), get_heap_allocator());
	assert(tree0, "tree0.png not found");
	Gfx_Image *tree1 = load_image_from_disk(STR("assets/tree1.png"), get_heap_allocator());
	assert(tree1, "tree1.png not found");
	Gfx_Image *mineral0 = load_image_from_disk(STR("assets/mineral0.png"), get_heap_allocator());
	assert(mineral0, "mineral0.png not found");
	Gfx_Image *mineral1 = load_image_from_disk(STR("assets/mineral1.png"), get_heap_allocator());
	assert(mineral1, "mineral1.png not found");

	Entity *player_en = entity_create();
	setup_player(player_en);

	for (int i = 0; i < 10; i++)
	{
		Entity *rock = entity_create();
		setup_rock(rock);
		rock->pos = v2(get_random_float32_in_range(-50.0, 50.0), get_random_float32_in_range(-100.0, 100.0));
	}

	for (int i = 0; i < 15; i++)
	{
		Entity *tree = entity_create();
		setup_tree(tree);
		tree->pos = v2(get_random_float32_in_range(-50.0, 50.0), get_random_float32_in_range(-100.0, 100.0));
	}

	float seconds_counter = 0.0;
	s32 frame_count = 0;

	float64 last_time = os_get_elapsed_seconds();

	while (!window.should_close)
	{
		reset_temporary_storage();

		draw_frame_reset(&draw_frame);
		float zoom = 5.0;
		draw_frame.camera_xform = m4_make_scale(v3(1.0 / zoom, 1.0 / zoom, 1));

		float64 now = os_get_elapsed_seconds();
		float64 dt = now - last_time;
		last_time = now;

		os_update();

		for (int i = 0; i < MAX_ENTITIES_COUNT; i++)
		{
			Entity *cur_entity = &world->entitites[i];
			if (cur_entity->is_valid)
			{
				switch (cur_entity->arch)
				{
				case arch_mineral:
					draw_image_at_pos(mineral0, cur_entity->pos);
					break;
				case arch_tree:
					draw_image_at_pos(tree0, cur_entity->pos);
					break;
				case arch_player:
					draw_image_at_pos(player, cur_entity->pos);
					break;
				default:
					break;
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

		player_en->pos = v2_add(player_en->pos, v2_mulf(input_axis, 50.0 * dt));

		gfx_update();
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