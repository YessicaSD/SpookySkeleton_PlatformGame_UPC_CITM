#include "p2Defs.h"
#include "p2Log.h"
#include "j1App.h"
#include "j1Render.h"
#include "j1Textures.h"
#include "j1Map.h"
#include <math.h>

j1Map::j1Map() : j1Module(), map_loaded(false)
{
	name.create("map");
}

// Destructor
j1Map::~j1Map()
{}

// Called before render is available
bool j1Map::Awake(pugi::xml_node& config)
{
	LOG("Loading Map Parser");
	bool ret = true;

	folder.create(config.child("folder").child_value());

	return ret;
}

void j1Map::Draw()
{
	if(map_loaded == false)
		return;


	

	// TODO 5: Prepare the loop to draw all tilesets + Blit
	
	for (p2List_item<TileSet*>* item_tileset = data.tilesets.start;item_tileset;item_tileset=item_tileset->next)
	{
		for (p2List_item<MapLayer*>* item_layer = data.layers.start; item_layer; item_layer = item_layer->next)
		{
			MapLayer* layer = item_layer->data;

			for (uint row = 0; row<data.height; row++)
			{
				for (uint column = 0; column<data.width; column++)
				{

					if (layer->tiledata[Get(column, row)] != 0)
					{

						iPoint mapPoint = MapToWorld(column, row);
						SDL_Rect section = item_tileset->data->GetTileRect(layer->tiledata[Get(column, row)]);
 						App->render->Blit(item_tileset->data->texture, mapPoint.x, mapPoint.y, &section, 1.0f);

					}

				}
			}

		}
	}
	p2List_item<Collision*>* item_coll = data.collisions.start;
	p2List_item<Object*>* object_rect = item_coll->data->object.start;
	Collision* coll_rect = item_coll->data;
	while (object_rect != NULL)
	{
		coll_rect->rect.w = object_rect->data->width;
		coll_rect->rect.h = object_rect->data->height;
		coll_rect->rect.x = object_rect->data->x;
		coll_rect->rect.y = object_rect->data->y;
		App->render->DrawQuad(coll_rect->rect, 255, 0, 0, 75);
		object_rect = object_rect->next;
	}
	
	
		// TODO 9: Complete the draw function

}


iPoint j1Map::MapToWorld(int x, int y) const
{
	iPoint ret;

	ret.x = x * data.tile_width;
	ret.y = y * data.tile_height;

	return ret;
}

SDL_Rect TileSet::GetTileRect(int id) const
{
	int relative_id = id - firstgid;
	SDL_Rect rect;
	rect.w = tile_width;
	rect.h = tile_height;
	rect.x = margin + ((rect.w + spacing) * (relative_id % num_tiles_width));
	rect.y = margin + ((rect.h + spacing) * (relative_id / num_tiles_width));
	return rect;
}

// Called before quitting
bool j1Map::CleanUp()
{
	LOG("Unloading map");

	// Remove all tilesets
	p2List_item<TileSet*>* item;
	item = data.tilesets.start;

	while(item != NULL)
	{
		RELEASE(item->data);
		item = item->next;
	}
	data.tilesets.clear();

	// TODO 2: clean up all layer data
	// Remove all layers
	
	data.layers.clear();

	// Clean up the pugui tree
	map_file.reset();
	
	return true;
}

// Load new map
bool j1Map::Load(const char* file_name)
{
	bool ret = true;
	p2SString tmp("%s%s", folder.GetString(), file_name);

	pugi::xml_parse_result result = map_file.load_file(tmp.GetString());

	if(result == NULL)
	{
		LOG("Could not load map xml file %s. pugi error: %s", file_name, result.description());
		ret = false;
	}

	// Load general info ----------------------------------------------
	if(ret == true)
	{
		ret = LoadMap();
	}

	// Load all tilesets info ----------------------------------------------
	pugi::xml_node tileset;
	for(tileset = map_file.child("map").child("tileset"); tileset && ret; tileset = tileset.next_sibling("tileset"))
	{
		TileSet* set = new TileSet();

		if(ret == true)
		{
			ret = LoadTilesetDetails(tileset, set);
		}

		if(ret == true)
		{
			ret = LoadTilesetImage(tileset, set);
		}

		data.tilesets.add(set);
	}

	// TODO 4: Iterate all layers and load each of them --Done
	// Load layer info ----------------------------------------------

	for (pugi::xml_node layer = map_file.child("map").child("layer"); layer; layer = layer.next_sibling("layer"))
	{
		MapLayer* set = new MapLayer();

		LoadLayer(layer, set);

		data.layers.add(set);
		
	}
	
	//Load Collision info
	pugi::xml_node collision;
	for (collision = map_file.child("map").child("objectgroup"); collision && ret; collision = collision.next_sibling("objectgroup"))
	{
		Collision*	coll = new Collision();
		ret = LoadCollision(collision, coll);
		if (ret == true)
		{
			data.collisions.add(coll);
		}
	}
	
	if(ret == true)
	{
		LOG("Successfully parsed map XML file: %s", file_name);
		LOG("width: %d height: %d", data.width, data.height);
		LOG("tile_width: %d tile_height: %d", data.tile_width, data.tile_height);

		p2List_item<TileSet*>* item = data.tilesets.start;
		while(item != NULL)
		{
			TileSet* s = item->data;
			LOG("Tileset ----");
			LOG("name: %s firstgid: %d", s->name.GetString(), s->firstgid);
			LOG("tile width: %d tile height: %d", s->tile_width, s->tile_height);
			LOG("spacing: %d margin: %d", s->spacing, s->margin);
			item = item->next;
		}

		// TODO 4: Add info here about your loaded layers
		// Adapt this vcode with your own variables
		
		p2List_item<MapLayer*>* item_layer = data.layers.start;
		while(item_layer != NULL)
		{
			MapLayer* l = item_layer->data;
			LOG("Layer ----");
			LOG("name: %s", l->name.GetString());
			LOG("tile width: %d tile height: %d", l->width, l->height);
			item_layer = item_layer->next;
		}

		p2List_item<Collision*>* item_coll = data.collisions.start;
		while (item_coll != NULL)
		{
			//Collision* c = item_coll->data;
			LOG("Collision ----");
			LOG("name: %s", item_coll->data->name.GetString());
			p2List_item<Object*>* item_obj = item_coll->data->object.start;
			while (item_obj != NULL)
			{
				LOG("collision width: %d collision height: %d", item_obj->data->width, item_obj->data->height);
				item_obj = item_obj->next;
			}
			item_coll = item_coll->next;
		}
	}

	map_loaded = ret;

	return ret;
}

// Load map general properties
bool j1Map::LoadMap()
{
	bool ret = true;
	pugi::xml_node map = map_file.child("map");

	if(map == NULL)
	{
		LOG("Error parsing map xml file: Cannot find 'map' tag.");
		ret = false;
	}
	else
	{
		data.width = map.attribute("width").as_uint();
		data.height = map.attribute("height").as_uint();
		data.tile_width = map.attribute("tilewidth").as_uint();
		data.tile_height = map.attribute("tileheight").as_uint();
		/*p2SString bg_color(map.attribute("backgroundcolor").as_string());*/

		/*data.background_color.r = 0;
		data.background_color.g = 0;
		data.background_color.b = 0;
		data.background_color.a = 0;*/

	/*	if(bg_color.Length() > 0)
		{
			p2SString red, green, blue;
			bg_color.SubString(1, 2, red);
			bg_color.SubString(3, 4, green);
			bg_color.SubString(5, 6, blue);

			int v = 0;

			sscanf_s(red.GetString(), "%x", &v);
			if(v >= 0 && v <= 255) data.background_color.r = v;

			sscanf_s(green.GetString(), "%x", &v);
			if(v >= 0 && v <= 255) data.background_color.g = v;

			sscanf_s(blue.GetString(), "%x", &v);
			if(v >= 0 && v <= 255) data.background_color.b = v;
		}*/

		p2SString orientation(map.attribute("orientation").as_string());

		if(orientation == "orthogonal")
		{
			data.type = MAPTYPE_ORTHOGONAL;
		}
		else if(orientation == "isometric")
		{
			data.type = MAPTYPE_ISOMETRIC;
		}
		else if(orientation == "staggered")
		{
			data.type = MAPTYPE_STAGGERED;
		}
		else
		{
			data.type = MAPTYPE_UNKNOWN;
		}
	}

	return ret;
}

bool j1Map::LoadTilesetDetails(pugi::xml_node& tileset_node, TileSet* set)
{
	bool ret = true;
	set->name.create(tileset_node.attribute("name").as_string());
	set->firstgid = tileset_node.attribute("firstgid").as_int();
	set->tile_width = tileset_node.attribute("tilewidth").as_int();
	set->tile_height = tileset_node.attribute("tileheight").as_int();
	set->margin = tileset_node.attribute("margin").as_int();
	set->spacing = tileset_node.attribute("spacing").as_int();
	pugi::xml_node offset = tileset_node.child("tileoffset");

	if(offset != NULL)
	{
		set->offset_x = offset.attribute("x").as_int();
		set->offset_y = offset.attribute("y").as_int();
	}
	else
	{
		set->offset_x = 0;
		set->offset_y = 0;
	}

	return ret;
}

bool j1Map::LoadTilesetImage(pugi::xml_node& tileset_node, TileSet* set)
{
	bool ret = true;
	pugi::xml_node image = tileset_node.child("image");

	if(image == NULL)
	{
		LOG("Error parsing tileset xml file: Cannot find 'image' tag.");
		ret = false;
	}
	else
	{
		set->texture = App->tex->Load(PATH(folder.GetString(), image.attribute("source").as_string()));
		int w, h;
		SDL_QueryTexture(set->texture, NULL, NULL, &w, &h);
		set->tex_width = image.attribute("width").as_uint();

		if(set->tex_width <= 0)
		{
			set->tex_width = w;
		}

		set->tex_height = image.attribute("height").as_uint();

		if(set->tex_height <= 0)
		{
			set->tex_height = h;
		}

		set->num_tiles_width = (set->tex_width - 2 * set->margin) / (set->tile_width /*+ set->spacing*/);
		set->num_tiles_height = (set->tex_height - 2 * set->margin) / (set->tile_height /*+ set->spacing*/);
	}

	return ret;
}

bool j1Map::LoadCollision(pugi::xml_node& node, Collision* collision)
{
	bool ret = true;
	collision->name = node.attribute("name").as_string();
	for (pugi::xml_node object_node = node.child("object"); object_node != NULL; object_node = object_node.next_sibling("object"))
	{
		Object* item_object = new Object;
		item_object->obj_id = object_node.attribute("id").as_int();
		item_object->width = object_node.attribute("width").as_float();
		item_object->height = object_node.attribute("height").as_float();
		item_object->x = object_node.attribute("x").as_float();
		item_object->y = object_node.attribute("y").as_float();

		collision->object.add(item_object);
		LOG("Perfect parsing of collision.tmx: Found the collisions");
	}

	return ret;
}

// TODO 3: Create the definition for a function that loads a single layer
bool j1Map::LoadLayer(pugi::xml_node& node, MapLayer* layer)
{
	layer->name = node.attribute("name").as_string();
	layer->width = node.attribute("width").as_uint();
	layer->height = node.attribute("height").as_uint();

	layer->tiledata = new uint[layer->width*layer->height];
	memset(layer->tiledata, 0u, sizeof(uint)*layer->height*layer->width);

	int i=0;
	for (pugi::xml_node tileset = node.child("data").child("tile"); tileset; tileset = tileset.next_sibling("tile"))
	{

		layer->tiledata[i] = tileset.attribute("gid").as_uint();
		
		LOG("%u", layer->tiledata[i]);
		++i;
	}
	return true;
}