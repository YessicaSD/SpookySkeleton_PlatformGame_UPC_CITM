#include "j1Scene.h"
#include "Brofiler/Brofiler.h"

#include "j1App.h"

#include "p2Defs.h"
#include "p2Log.h"

#include "j1Input.h"
#include "j1Textures.h"
#include "j1Audio.h"
#include "j1Render.h"
#include "j1Window.h"
#include "j1Map.h"
#include "j1Pathfinding.h"
#include "j1Gui.h"

#include "UiItem_Label.h"
#include "UiItem.h"
#include "ModuleFadeToBack.h"

j1Scene::j1Scene() : j1Module()
{
	name.create("scene");
}

// Destructor
j1Scene::~j1Scene()
{
}

// Called before render is available
bool j1Scene::Awake(pugi::xml_node& node)
{
	LOG("Loading Scene");
	bool ret = true;
	const char* path = node.child("idle").child_value();
	
	pugi::xml_parse_result result = sceneFile.load_file(path);
	if (result == NULL)
	{
		LOG("ERROR ENTITIES LOADING FILE %s", result.description());
		return ret = false;
	}
	uint width, height;

	sceneNode = sceneFile.child("scene");
	horizontalScreenDivision = App->win->width * 0.125F;

	LoadStartMenu(node);
	
	return ret;
}

// Called before the first frame
bool j1Scene::Start()
{
	switch (state)
	{
	case SceneState::STARTMENU:
	{
		App->win->scale = 1.0F;
		if(Background==nullptr)
			Background = App->tex->Load(backgroundPath);
		App->audio->PlayMusic(mainMusicStartMenu);
		if(fx_death_aux==0)
			fx_death_aux = App->audio->LoadFx(findSfxPath("test"));
	}
		break;
	case SceneState::GAME:
	{	//Pick level node-----------------------------------------
		pugi::xml_node entitiesNode;
		if (loadingSaveFile == true)
		{
			loadingSaveFile = false;
		}
		else
		{
			pugi::xml_node levelNode = sceneNode.child("level");
			if (levelNode == NULL)
			{
				LOG("ERROR ENTITIES LOADING FILE");
				return false;
			}

			for (uint i = 1; i < num_thismaplvl; ++i)
			{
				levelNode = levelNode.next_sibling("level");
			}
			//Load enemies -----------------------------------------
			entitiesNode = levelNode.child("entities");
			if (entitiesNode == nullptr)
				return false;
			LoadEntities(entitiesNode);
		}
		uint numEnemies = entitiesArrayInfo.Count();
		if (numEnemies > 0)
		{
			for (uint i = 0; i < numEnemies; ++i)
			{
				App->entity->AddEntity(entitiesArrayInfo[i]);
			}
		}
	}
			
		break;
	case SceneState::PAUSE:
		
		break;
	case SceneState::SETTING:
		break;
	default:
		break;
	}
	
	return true;
}

// Called each loop iteration
bool j1Scene::PreUpdate(float dt)
{
	BROFILER_CATEGORY("PreUpdate_Scene.cpp", Profiler::Color::Salmon)
		this->dt = dt;
	return true;
}

bool j1Scene::Update(float dt)
{
	BROFILER_CATEGORY("Update_Scene.cpp", Profiler::Color::Coral)
		switch (state)
		{
		case SceneState::STARTMENU:
			
			App->render->Blit(Background, 0, 0, NULL, SDL_FLIP_NONE, 0.0F);
			break;

		case SceneState::GAME:
			DebugControls();

			//Load and save game---------------------------------------------------
			if (App->input->GetKey(SDL_SCANCODE_F6) == KEY_DOWN)
				App->LoadGame();

			if (App->input->GetKey(SDL_SCANCODE_F5) == KEY_DOWN)
				App->SaveGame();
			break;

		case SceneState::PAUSE:
			break;
		case SceneState::SETTING:
			break;
		case SceneState::MAX_STATES:
			break;
		default:
			break;
		}
	
	AudioControl();
	return true;
}

// Called each loop iteration
bool j1Scene::PostUpdate()
{
	bool ret = true;

	if (App->input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN)
		ret = false;

	if (exitGame)
		return false;
	return ret;
}

void j1Scene::DebugControls()
{
	//Camera controls -----------------------------------------------------
	if (App->input->GetKey(SDL_SCANCODE_KP_8) == KEY_REPEAT)
		App->render->camera.y -= 5;

	if (App->input->GetKey(SDL_SCANCODE_KP_5) == KEY_REPEAT)
		App->render->camera.y += 5;

	if (App->input->GetKey(SDL_SCANCODE_KP_4) == KEY_REPEAT)
		App->render->camera.x -= 5;

	if (App->input->GetKey(SDL_SCANCODE_KP_6) == KEY_REPEAT)
		App->render->camera.x += 5;

	if (App->input->GetKey(SDL_SCANCODE_F1))
	{
		App->fade->FadeToBlack(1);
	}
	if (App->input->GetKey(SDL_SCANCODE_F2))
	{
		App->fade->FadeToBlack(App->scene->num_thismaplvl);
	}
	if (App->input->GetKey(SDL_SCANCODE_F3))
	{
		App->fade->FadeToBlack(2);

	}
}
void j1Scene::CameraLogic(float dt)
{
	iPoint offset(App->win->width*0.5F, App->win->height*0.5F);

	iPoint playerPivotPos;
	playerPivotPos.x = App->entity->entity_player->position.x * (int)App->win->GetScale(); 
	playerPivotPos.y = App->entity->entity_player->position.y * (int)App->win->GetScale();

	/*fPoint targetPos;
	targetPos.x = (float)(playerPivotPos.x - offset.x);
	targetPos.y = (float)(playerPivotPos.y - offset.y);*/

	fPoint targetPos;
	targetPos.x = (float)(0 - offset.x);
	targetPos.y = (float)(0 - offset.y);

	fPoint cameraPos;

	cameraPos.x += (targetPos.x + App->render->camera.x) * 3 * dt;
	cameraPos.y += (targetPos.y - App->render->camera.y) * 3 * dt;


	App->render->camera.x = cameraPos.x;
	App->render->camera.y = cameraPos.y;

	LOG("CAMERA POS X=%i,Y=%i", App->render->camera.x, App->render->camera.y);
	
	
}
// Called before quitting
bool j1Scene::CleanUp()
{
	LOG("Freeing scene");
	App->tex->UnLoad(Background);
	App->win->scale = 2.0F;
	App->entity->DestroyAllEntities();

	return true;
}

bool j1Scene::Load(pugi::xml_node& node)
{
	LoadEntities(node.child("level").child("entities"));
	return true;
}

bool j1Scene::Save(pugi::xml_node & node) const
{	
	
	pugi::xml_node nodeEntities = node.append_child("level").append_child("entities");
	
	for (p2List_item<j1Entity*>* enemiesIterator = App->entity->list_Entities.start; enemiesIterator; enemiesIterator = enemiesIterator->next)
	{
		pugi::xml_node entity;
		switch (enemiesIterator->data->type)
		{
		case PLAYER:
			entity = nodeEntities.append_child("player");
			entity.append_attribute("x").set_value((int)enemiesIterator->data->position.x);
			entity.append_attribute("y").set_value((int)enemiesIterator->data->position.y);
			break;
		case ENEMY_BAT:
			entity = nodeEntities.append_child("bat");
			entity.append_attribute("x").set_value((int)enemiesIterator->data->position.x);
			entity.append_attribute("y").set_value((int)enemiesIterator->data->position.y);
			break;
		case ENEMI_ZOMBIE:
			entity = nodeEntities.append_child("zombie");
			entity.append_attribute("x").set_value((int)enemiesIterator->data->position.x);
			entity.append_attribute("y").set_value((int)enemiesIterator->data->position.y);
			break;
		default:
			break;
		}
	}
	return true;
}

void j1Scene::AudioControl()
{
	if (App->input->GetKey(SDL_SCANCODE_KP_PLUS) == KEY_DOWN)
		App->audio->SetVolume(MIX_MAX_VOLUME / 16);

	if (App->input->GetKey(SDL_SCANCODE_KP_MINUS) == KEY_DOWN)
		App->audio->SetVolume(-(MIX_MAX_VOLUME / 16));

	if (App->input->GetKey(SDL_SCANCODE_P) == KEY_DOWN)
		App->audio->SetFxVolume(MIX_MAX_VOLUME / 16);

	if (App->input->GetKey(SDL_SCANCODE_O) == KEY_DOWN)
		App->audio->SetFxVolume(-(MIX_MAX_VOLUME / 16));

	if (App->input->GetKey(SDL_SCANCODE_Y) == KEY_DOWN)
		App->audio->PlayFx(fx_death_aux);

}

bool j1Scene::LoadStartMenu(pugi::xml_node& nodeScene)
{
	pugi::xml_node startMenuNode = nodeScene.child("StartMenu");

	backgroundPath = startMenuNode.child("background").child_value();
	mainMusicStartMenu = startMenuNode.child("music").child_value();
	for(pugi::xml_node sfxNode = startMenuNode.child("soundEffects").child("sfx"); sfxNode ; sfxNode = sfxNode.next_sibling("sfx"))
	{
		sfx newSfx(sfxNode.attribute("name").as_string(), sfxNode.child_value());
		arraySfx.PushBack(newSfx);
		
	} 
	startMenupanel = App->Gui->AddEmptyElement({ 0,0 });
	LoadUiElement(startMenupanel, startMenuNode.child("gui").child("startMenuPanel"));

	/*
	
	UiItem_Label* label = App->Gui->AddLabel("Play", { 62,32,28,255 }, App->Gui->arrayFonts[COPPERPLATE_B_I_48], { 45,60 }, buttonPlay);
	SDL_Color color = { 113,57,36,255 };
	label->ChangeTextureHover(NULL, &color, NULL);

	buttonPlay->AddFuntion(mapOfFuntions["FadeToScene"]);
	thisMenuItems.add(label);
	thisMenuItems.add(App->Gui->AddButton({ 392,565,252,146 }, (const SDL_Rect*)&ButtonFrames[0], NULL, , (const SDL_Rect*)&ButtonFrames[2], (const SDL_Rect*)&ButtonFrames[1]));


	ButtonFrames[0] = { 186,0,70,70 };
	ButtonFrames[1] = { 256,0,70,70 };
	ButtonFrames[2] = { 627,0,70,70 };
	UiItem_Button* buttonExit = App->Gui->AddButton({ 915,31,70,70 }, (const SDL_Rect*)&ButtonFrames[0], NULL, , (const SDL_Rect*)&ButtonFrames[1], (const SDL_Rect*)&ButtonFrames[2]);
	thisMenuItems.add(buttonExit);
	buttonExit->AddFuntion(mapOfFuntions["ExitGame"]);
	
	ButtonFrames[0] = { 0,0,93,93 };
	ButtonFrames[1] = { 93,0,93,93 };
	thisMenuItems.add(App->Gui->AddButton({ 51,35,65,65 }, (const SDL_Rect*)&ButtonFrames[0], NULL, , (const SDL_Rect*)&ButtonFrames[1], (const SDL_Rect*)&ButtonFrames[1], { 14,12 }));


	label = App->Gui->AddLabel("Continue", { 62,32,28,255 }, App->Gui->arrayFonts[COPPERPLATE_B_I_24], { 440,635 }, NULL);
	label->ChangeTextureHover(NULL, &color, NULL);
	thisMenuItems.add(label);
*/

	return true;
}
void j1Scene::LoadUiElement(UiItem*parent, pugi::xml_node node)
{
	for (pugi::xml_node imageNode = node.child("images").child("image"); imageNode; imageNode = imageNode.next_sibling("image"))
	{
		SDL_Rect hitBox = { imageNode.child("hitbox").attribute("x").as_int(), imageNode.child("hitbox").attribute("y").as_int(), imageNode.child("hitbox").attribute("w").as_int(), imageNode.child("hitbox").attribute("h").as_int() };
		SDL_Rect sectionIdle = { imageNode.child("idleSec").attribute("x").as_int(), imageNode.child("idleSec").attribute("y").as_int(), imageNode.child("idleSec").attribute("w").as_int(), imageNode.child("idleSec").attribute("h").as_int() };
		UiItem*newElement = App->Gui->AddImage(hitBox, &sectionIdle, parent);
		if (imageNode.child("childs"))
		{
			LoadUiElement(newElement, imageNode.child("childs"));
		}
	}
	for (pugi::xml_node buttonNode = node.child("buttons").child("button"); buttonNode; buttonNode = buttonNode.next_sibling("image"))
	{
		SDL_Rect hitBox = { buttonNode.child("hitbox").attribute("x").as_int(), buttonNode.child("hitbox").attribute("y").as_int(), buttonNode.child("hitbox").attribute("w").as_int(), buttonNode.child("hitbox").attribute("h").as_int() };
		SDL_Rect sectionIdle = { buttonNode.child("idleSec").attribute("x").as_int(), buttonNode.child("idleSec").attribute("y").as_int(), buttonNode.child("idleSec").attribute("w").as_int(), buttonNode.child("idleSec").attribute("h").as_int() };
		pugi::xml_node hoverSecNode = buttonNode.child("hoverSec");
		SDL_Rect* sectionHove = nullptr;
		if (hoverSecNode)
		{
			SDL_Rect hover = { hoverSecNode.attribute("x").as_int(), hoverSecNode.attribute("y").as_int(), hoverSecNode.attribute("w").as_int(), hoverSecNode.attribute("h").as_int() };
			sectionHove = &hover;
		}
		pugi::xml_node clickSecNode = buttonNode.child("clickSec");
		SDL_Rect* sectionClick = nullptr;
		if (clickSecNode)
		{
			SDL_Rect click = { clickSecNode.attribute("x").as_int(), clickSecNode.attribute("y").as_int(), clickSecNode.attribute("w").as_int(), clickSecNode.attribute("h").as_int() };
			sectionClick = &click;
		}
		p2SString funtionPath = buttonNode.attribute("funtion").as_string();
		UiItem*newElement = App->Gui->AddButton(hitBox, &sectionIdle, funtionPath, parent, sectionClick, sectionHove);
		if (buttonNode.child("childs"))
		{
			LoadUiElement(newElement, buttonNode.child("childs"));
		}
	}
	for (pugi::xml_node labelNode = node.child("labels").child("label"); labelNode; labelNode = labelNode.next_sibling("image"))
	{
		p2SString text = labelNode.attribute("text").as_string();
		SDL_Color color = { labelNode.child("color").attribute("r").as_uint(),labelNode.child("color").attribute("g").as_uint(),labelNode.child("color").attribute("b").as_uint(),labelNode.child("color").attribute("a").as_uint() };
		p2SString font=labelNode.attribute("font").as_string();

		App->Gui->AddLabel(text.GetString(), color, App->font->getFont(font), {labelNode.attribute("pos_x").as_int(),labelNode.attribute("pos_y").as_int()},parent);
	}
}
void j1Scene::LoadEntities(const pugi::xml_node& entitiesNode)
{
	entitiesArrayInfo.Clear();
	//players---------------------------
	for (pugi::xml_node playerNode = entitiesNode.child("player"); playerNode != NULL; playerNode = playerNode.next_sibling("player"))
	{
		entitiesArrayInfo.PushBack(EntitiesInfo(PLAYER, { playerNode.attribute("x").as_float(),playerNode.attribute("y").as_float() }));
	}
	//bats-----------------------------
	for (pugi::xml_node entityNode = entitiesNode.child("bat"); entityNode; entityNode = entityNode.next_sibling("bat"))
	{
		entitiesArrayInfo.PushBack(EntitiesInfo(ENEMY_BAT, { entityNode.attribute("x").as_float(),entityNode.attribute("y").as_float() }));

	}
	//zombies---------------------------
	for (pugi::xml_node entityNode = entitiesNode.child("zombie"); entityNode; entityNode = entityNode.next_sibling("zombie"))
	{
		entitiesArrayInfo.PushBack(EntitiesInfo(ENEMI_ZOMBIE, { entityNode.attribute("x").as_float(),entityNode.attribute("y").as_float() }));
	}
	//coins---------------------------
	for (pugi::xml_node entityNode = entitiesNode.child("coin"); entityNode; entityNode = entityNode.next_sibling("coin"))
	{
		entitiesArrayInfo.PushBack(EntitiesInfo(COIN, { entityNode.attribute("x").as_float(),entityNode.attribute("y").as_float() }));
	}
}
const char* j1Scene::findSfxPath(const char* name)
{
	uint size = arraySfx.Count();
	for (uint num = 0; num < size; ++num)
	{
		
		if (arraySfx[num].name == name)
			return arraySfx[num].path.GetString();
	}
	return "";
}


