#include "World.h"
#include "PhongShader.h"
#include "TextureDebugShader.h"
#include "DebugCamera.h"
#include "Global.h"
#include "ArcheryCamera.h"
#include "GuardEntity.h"
#include "ProjectileEntity.h"
#include "ObstacleEntity.h"
#include "OctTree.h"
#include "DeferredShader.h"
#include "CollisionHandler.h"
#include "DebugShader.h"
#include <glm/gtx/rotate_vector.hpp>
#include "Skybox.h"
#include "SimpleTextureShader.h"
#include "TestSphere.h"
#include <functional>
#include <queue>
#include "LightEntity.h"
#include "DoorEntity.h"
#include "FrustrumCulling.h"
#include <iostream>
#include <fstream>
#include "ParticleShader.h"
#include "CinematicCamera.h"
#include "AudioManager.h"

#define FILE_TO_WORLD_SCALE 6.f

using namespace std;
using namespace glm;

bool keys[1024];
float seconds_passed = 0;
float x_offset;
float y_offset;

bool debug_enabled = false;
float screen_width = SCREEN_WIDTH;
float screen_height = SCREEN_HEIGHT;

mat4 projection = mat4(perspective((float)radians(PLAYER_FOV), screen_width / screen_height, PLAYER_NEAR, PLAYER_FAR));
mat4 guard_projection = mat4(perspective((float)radians(GUARD_FOV), screen_width / screen_height, GUARD_NEAR, GUARD_FAR));

static std::shared_ptr<Camera> camera;
static shared_ptr<DebugShader> debugShader;
bool time_stopped = false;
float game_speed = 1.0f;
static vector<std::function<void()>> debugShaderQueue;

float bow_strength = .5f;

World::World()
{
    debugShader = make_shared<DebugShader>("debugVert.glsl", "debugFrag.glsl");
	init();
    x_offset = 0;
    y_offset = 0;
	state = LEVEL1;
}

void World::init()
{
	debug_camera = make_shared<DebugCamera>();
	player_camera = make_shared<PlayerCamera>();
	archery_camera = make_shared<ArcheryCamera>();
	cinematic_camera = make_shared<CinematicCamera>();

    meshes.insert(pair<string, shared_ptr<MeshSet>>("tower", make_shared<MeshSet>(assetPath + "tower.dae")));
    meshes.insert(pair<string, shared_ptr<MeshSet>>("wall", make_shared<MeshSet>(assetPath + "wall.dae")));
    meshes.insert(pair<string, shared_ptr<MeshSet>>("interior_wall", make_shared<MeshSet>(assetPath + "interiorWall.dae")));
    meshes.insert(pair<string, shared_ptr<MeshSet>>("door", make_shared<MeshSet>(assetPath + "door.dae")));
    meshes.insert(pair<string, shared_ptr<MeshSet>>("ground", make_shared<MeshSet>(assetPath + "ground.dae")));
    meshes.insert(pair<string, shared_ptr<MeshSet>>("chewy", make_shared<MeshSet>(assetPath + "ninja_final3.dae")));
    meshes.insert(pair<string, shared_ptr<MeshSet>>("chewy_bb", make_shared<MeshSet>(assetPath + "ninja_boundingbox.dae")));
    meshes.insert(pair<string, shared_ptr<MeshSet>>("guard", make_shared<MeshSet>(assetPath + "samurai2.dae")));
	meshes.insert(pair<string, shared_ptr<MeshSet>>("guard_bb", make_shared<MeshSet>(assetPath + "samurai_bbox.obj")));
    meshes.insert(pair<string, shared_ptr<MeshSet>>("arrow", make_shared<MeshSet>(assetPath + "arrow.dae")));
    meshes.insert(pair<string, shared_ptr<MeshSet>>("arrow_bb", make_shared<MeshSet>(assetPath + "arrow_boundingbox.dae")));
    meshes.insert(pair<string, shared_ptr<MeshSet>>("unit_sphere", make_shared<MeshSet>(assetPath + "UnitSphere.obj")));
    meshes.insert(pair<string, shared_ptr<MeshSet>>("lantern", shared_ptr<MeshSet>(new MeshSet(assetPath + "lantern.dae"))));
    meshes.insert(pair<string, shared_ptr<MeshSet>>("lanternPole", shared_ptr<MeshSet>(new MeshSet(assetPath + "lanternPole.dae"))));
    meshes.insert(pair<string, shared_ptr<MeshSet>>("lanternPole_boundingbox", shared_ptr<MeshSet>(new MeshSet(assetPath + "lanternPole_boundingbox.dae"))));
    meshes.insert(pair<string, shared_ptr<MeshSet>>("closedBarrel", shared_ptr<MeshSet>(new MeshSet(assetPath + "closedBarrel.dae"))));
    meshes.insert(pair<string, shared_ptr<MeshSet>>("box", shared_ptr<MeshSet>(new MeshSet(assetPath + "Box.dae"))));
    meshes.insert(pair<string, shared_ptr<MeshSet>>("skybox", shared_ptr<MeshSet>(new MeshSet(assetPath + "skybox.dae", GL_LINEAR, GL_CLAMP_TO_EDGE))));
	meshes.insert(pair<string, shared_ptr<MeshSet>>("flowerPlanter", shared_ptr<MeshSet>(new MeshSet(assetPath + "flowerPlanter.dae"))));
	meshes.insert(pair<string, shared_ptr<MeshSet>>("bushes", shared_ptr<MeshSet>(new MeshSet(assetPath + "bushes.dae"))));
	meshes.insert(pair<string, shared_ptr<MeshSet>>("bushes_boundingbox", shared_ptr<MeshSet>(new MeshSet(assetPath + "bushes_boundingbox.dae"))));
	meshes.insert(pair<string, shared_ptr<MeshSet>>("statue", shared_ptr<MeshSet>(new MeshSet(assetPath + "statue.dae"))));
    
    chewy = std::make_shared<ChewyEntity>(vec3(0.f), meshes.at("chewy"), player_camera, archery_camera);
    chewy->setup_entity_box(meshes.at("chewy_bb"));
	chewy->list = SET_HIDE(chewy->list);

	

	//x_offset = -30.0f;
	player_camera->movement(chewy);
	/*cinematic_camera->init({ player_camera->cameraPosition, vec3(-5.0, 32.0, 140.0), vec3(23.f,32.f, 215.f), vec3(110.f, 32.f, 230.f), 
			vec3(217.f, 32.f, 215.f), vec3(235.f, 32.f, 117.f), vec3(227.f, 32.f, 15.f), vec3(120.f, 32.f, 2.f), player_camera->cameraPosition }, 
		{ player_camera->lookAtPoint, vec3(0.f, 32.0f, 140.f), vec3(27.f, 32.f, 215.f), vec3(110.f, 32.f, 223.f), vec3(213.f, 32.f, 213.f), vec3(230.f, 32.f, 117.f),
			vec3(223.f, 32.f, 18.f), vec3(120.f, 32.f, 7.f), player_camera->lookAtPoint },
		40.f);*/
	cinematic_camera->init({ player_camera->cameraPosition, player_camera->cameraPosition, player_camera->cameraPosition, player_camera->cameraPosition},
	{ player_camera->lookAtPoint, player_camera->lookAtPoint, player_camera->lookAtPoint, player_camera->lookAtPoint, }, 10.f);

	_skybox = std::make_shared<Skybox>(Skybox(&camera, meshes.at("skybox")));
	_skybox->setScale(750.f);
	_skybox->list = UNSET_OCTTREE((_skybox->list));

    shared_ptr<GameEntity> tower(new ObstacleEntity(vec3(0.0, 0.0, 0.0), meshes.at("tower")));
    tower->setScale(30.0f);
    tower->list = UNSET_OCTTREE((tower->list));
    tower->collision_response = false;

    float wall_scale = 30.f;
    shared_ptr<GameEntity> wall_left(new ObstacleEntity(vec3(120.f, 0.f, 240.f), meshes.at("wall")));
    wall_left->setScale(wall_scale);
    wall_left->bounding_box.half_height = wall_left->bounding_box.half_height + 12.f;
    shared_ptr<GameEntity> wall_right(new ObstacleEntity(vec3(120.f, 0.f, 0.f), meshes.at("wall")));
    wall_right->setScale(wall_scale);
    wall_right->bounding_box.half_height = wall_right->bounding_box.half_height + 12.f;
    shared_ptr<GameEntity> wall_front(new ObstacleEntity(vec3(0.f, 0.f, 120.f), meshes.at("wall")));
    wall_front->setScale(wall_scale);
    wall_front->setRotations(vec3(0.f, M_PI_2, 0.f));
    wall_front->swap_bounding_box_width_depth();
    wall_front->bounding_box.half_height = wall_front->bounding_box.half_height + 12.f;
    shared_ptr<GameEntity> wall_back(new ObstacleEntity(vec3(240.f, 0.f, 120.f), meshes.at("wall")));
    wall_back->setScale(wall_scale);
    wall_back->setRotations(vec3(0.f, M_PI_2, 0.f));
    wall_back->swap_bounding_box_width_depth();
    wall_back->bounding_box.half_height = wall_back->bounding_box.half_height + 12.f;
    shared_ptr<GameEntity> ground(new ObstacleEntity(vec3(120.f, -12.0f, 120.f), meshes.at("ground")));
    ground->setScale(30.f);

	camera = cinematic_camera;
	cinematic_camera->in_use = true;

    // Add these into a persistent entities list?
    // or you can just remove everything after their index since
    // they're pushed first (for swapping levels)
	entities.push_back(chewy);
	entities.push_back(tower);
    entities.push_back(wall_left);
    entities.push_back(wall_right);
    entities.push_back(wall_front);
    entities.push_back(wall_back);
    entities.push_back(ground);

    setup_next_courtyard();

	hud = HUD(chewy);

	shaders.insert(pair<string, shared_ptr<Shader>>("phongShader", make_shared<PhongShader>("phongVert.glsl", "phongFrag.glsl")));

	shaders.insert(pair<string, shared_ptr<Shader>>("defShader", make_shared<DeferredShader>("DeferredVertShader.glsl", "DeferredFragShader.glsl", _skybox)));

	shaders.insert(pair<string, shared_ptr<Shader>>("debugShader", debugShader));

	shaders.insert(pair<string, shared_ptr<Shader>>("simpleShader", make_shared<SimpleTextureShader>("simpleVert.glsl", "simpleFrag.glsl")));

	//shared_ptr<Shader> textDebugShader(new TextureDebugShader());
	//shaders.insert(pair<string, shared_ptr<Shader>>("textureDebugShader", textDebugShader));

	//AudioManager::instance()->playAmbient(assetPath + "ninjatune.mp3", 0.5f);
}

void World::setup_next_courtyard()
{
    // remove all non-persistent entities
    entities.erase(entities.begin() + 7, entities.end());
    
    // JUST FOR DEMO
    if (current_courtyard == 3)
        current_courtyard = 1;

    switch (current_courtyard)
    {
    case 1:
        setup_level(assetPath + "first_courtyard.txt");
        setup_guard(assetPath + "first_courtyard_guard.txt");
        setup_guard(assetPath + "first_courtyard_second_guard.txt");
        setup_guard(assetPath + "first_courtyard_third_guard.txt");
        setup_guard(assetPath + "first_courtyard_fourth_guard.txt");
        break;
    case 2:
        setup_level(assetPath + "second_courtyard.txt");
        setup_guard(assetPath + "second_courtyard_first_guard.txt");
        setup_guard(assetPath + "second_courtyard_second_guard.txt");
        setup_guard(assetPath + "second_courtyard_third_guard.txt");
        break;
    case 3:
        break;
    case 4:
        break;
    case 5:
        break;
    }

    current_courtyard++;
}

void World::setup_level(string file_path)
{
    ifstream level_file;
    level_file.open(file_path);

    string current_line;
    int current_row = 0;
    int height_level = -1;

    while (!level_file.eof()) // runs through every line
    {
        getline(level_file, current_line);
        if (current_line == "________________________________________")
        {
            current_row = 0;
            height_level++;
        }
        for (int i = 0; i < current_line.length(); i++)
        {
            glm::vec3 world_position = FILE_TO_WORLD_SCALE * vec3(i, height_level, current_row) + vec3(FILE_TO_WORLD_SCALE / 2.f, 0.f, FILE_TO_WORLD_SCALE / 2.f);
            setup_token(current_line.at(i), world_position);
        }
        current_row++;
    }
    level_file.close();
}

void World::setup_token(char obj_to_place, glm::vec3 placement_position)
{
    vec3 rots = vec3(0.f);
    bool flag = false; // used for door orientation and accessability
    switch (obj_to_place)
    {
    case 'C': // set chewy's position
        chewy->setPosition(placement_position + vec3(0.f, 10.f, 0.f));
        break;
    case 'D': // door
        if (placement_position.z < 120.f)
            placement_position.z -= 2.65f;
        else
        {
            placement_position.z += 2.65f;
            rots = vec3(0.f, M_PI, 0.f);
            flag = true;
        }
        entities.push_back(std::make_shared<DoorEntity>(DoorEntity(placement_position, meshes.at("door"), flag, this)));
        entities.back()->setScale(3.f);
        entities.back()->setRotations(rots);
        break;
    case 'F': // statue and flower bed
        entities.push_back(std::make_shared<ObstacleEntity>(ObstacleEntity(placement_position, meshes.at("flowerPlanter"))));
        entities.back()->setScale(6.f);
        entities.back()->list = SET_HIDE((entities.back()->list));
		entities.push_back(std::make_shared<ObstacleEntity>(ObstacleEntity(placement_position + vec3(0.f, .94483f * 6.f, 0.f), meshes.at("bushes"))));
		entities.back()->setup_entity_box(meshes.at("bushes_boundingbox"));
		entities.back()->setScale(6.f);
		entities.back()->list = SET_HIDE((entities.back()->list));
        entities.push_back(std::make_shared<ObstacleEntity>(ObstacleEntity(placement_position, meshes.at("statue"))));
        entities.back()->setScale(6.f);
        entities.back()->list = SET_HIDE((entities.back()->list));
        entities.back()->setRotations(vec3(0.f, M_PI, 0.f));
        break;
    case 'n': // static guard facing north
        entities.push_back(std::shared_ptr<GuardEntity>(new GuardEntity(placement_position, meshes.at("guard"), vec3(0.0f, 0.f, -1.f))));
        break;
    case 's': // static guard facing south
        entities.push_back(std::shared_ptr<GuardEntity>(new GuardEntity(placement_position, meshes.at("guard"), vec3(0.0f, 0.f, 1.f))));
        break;
    case 'e': // static guard facing east
        entities.push_back(std::shared_ptr<GuardEntity>(new GuardEntity(placement_position, meshes.at("guard"), vec3(1.0f, 0.f, 0.f))));
        break;
    case 'w': // static guard facing west
        entities.push_back(std::shared_ptr<GuardEntity>(new GuardEntity(placement_position, meshes.at("guard"), vec3(-1.0f, 0.f, 0.f))));
        break;
    case 'O': // barrel
        entities.push_back(std::make_shared<ObstacleEntity>(ObstacleEntity(placement_position, meshes.at("closedBarrel"))));
        entities.back()->setScale(3.f);
        entities.back()->list = SET_HIDE((entities.back()->list));
        break;
    case 'W': // interior wall
        entities.push_back(std::make_shared<ObstacleEntity>(ObstacleEntity(placement_position, meshes.at("interior_wall"))));
        entities.back()->setScale(3.f);
        // this will rotate alternating so that the textures line up correctly
        if (((((int)placement_position.x / (int)FILE_TO_WORLD_SCALE) % 2) && (((int)placement_position.z / (int)FILE_TO_WORLD_SCALE) % 2)) ||
            (!(((int)placement_position.x / (int)FILE_TO_WORLD_SCALE) % 2) && !(((int)placement_position.z / (int)FILE_TO_WORLD_SCALE) % 2)))
            entities.back()->setRotations(vec3(0.f, M_PI_2, 0.f));
        entities.back()->list = SET_HIDE((entities.back()->list));
        break;
    case 'X': // crate
        entities.push_back(std::make_shared<ObstacleEntity>(ObstacleEntity(placement_position, meshes.at("box"))));
        entities.back()->setScale(3.f);
        entities.back()->list = SET_HIDE((entities.back()->list));
        break;
    case 'l': // Lantern Pole with Lantern
        entities.push_back(std::make_shared<ObstacleEntity>(ObstacleEntity(placement_position, meshes.at("lanternPole"))));
        entities.back()->setup_entity_box(meshes.at("lanternPole_boundingbox"));
        entities.push_back(std::make_shared<LightEntity>(LightEntity(placement_position + vec3(0.f, 5.9f, 0.8f), meshes.at("lantern"), 300.f, meshes.at("unit_sphere"))));
        rots = entities.back()->getRotations();
        rots.y = M_PI_2;
        entities.back()->setRotations(rots);
        break;
    }
}

void World::setup_guard(string file_path)
{
    ifstream level_file;
    level_file.open(file_path);

    string current_line;
    int current_row = 0;

    vec3 control_points[10];
    vec3 starting_position;
	bool linear = false;

    while (!level_file.eof()) // runs through every line
    {
        getline(level_file, current_line);

        for (int i = 0; i < current_line.length(); i++)
        {
            glm::vec3 world_position = FILE_TO_WORLD_SCALE * vec3(i, 0.f, current_row) + vec3(FILE_TO_WORLD_SCALE / 2.f, 0.f, FILE_TO_WORLD_SCALE / 2.f);
            switch (current_line.at(i))
            {
            case 'G':
                starting_position = world_position;
                break;
			case 'g':
				starting_position = world_position;
				linear = true;
				break;
            case '0':
                control_points[0] = world_position;
                break;
            case '1':
                control_points[1] = world_position;
                break;
            case '2':
                control_points[2] = world_position;
                break;
            case '3':
                control_points[3] = world_position;
                break;
            case '4':
                control_points[4] = world_position;
                break;
            case '5':
                control_points[5] = world_position;
                break;
            case '6':
                control_points[6] = world_position;
                break;
            case '7':
                control_points[7] = world_position;
                break;
            case '8':
                control_points[8] = world_position;
                break;
            case '9':
                control_points[9] = world_position;
                break;
            }
        }
        current_row++;
    }

    vector<vec3> spline_points;
    spline_points.push_back(starting_position);
    for (int i = 0; i < 10; i++)
    {
        if (control_points[i] != vec3(0))
            spline_points.push_back(glm::vec3(control_points[i]));
        else
            break;
    }
	shared_ptr<GuardEntity> guard_ent = make_shared<GuardEntity>(starting_position, meshes["guard"], spline_points, 4.f, linear);
	guard_ent->setup_entity_box(meshes["guard_bb"]);
	entities.push_back(guard_ent);
    level_file.close();
}

void World::shootArrows()
{

	static bool held = false;
	bool shot = false;
	for (auto it = entities.begin(); it != entities.end(); ++it) {
		if (typeid(*(*it)) == typeid(ProjectileEntity)) {
			shot = true;
		}
	}
	if (keys[GLFW_KEY_E] && archery_camera->in_use && !held && !shot)
	{
		held = true;
	}
	if (held && !keys[GLFW_KEY_E])
	{
		entities.push_back(make_shared<ProjectileEntity>(meshes["arrow"], archery_camera));
        entities.back()->setup_entity_box(meshes.at("arrow_bb"));
		held = false;
	}
}

void World::draw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(0);
	static bool usePhong = false;

	if (keys[GLFW_KEY_6])
	{
		//usePhong = true;
        setup_next_courtyard();
	}
	if (keys[GLFW_KEY_7])
	{
		usePhong = false;
	}


	if (usePhong) {
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glDepthMask(GL_TRUE);


		glUseProgram(shaders.at("simpleShader")->getProgramID());
		glViewport(0, 0, screen_width, screen_height);
		shaders.at("simpleShader")->draw(camera->getViewMatrix(), _skybox);
	}

	shared_ptr<DebugCamera> d_test = dynamic_pointer_cast<DebugCamera>(camera);
	vector<shared_ptr<GameEntity>> in_view;
    if (d_test != nullptr)
    {
        in_view = get_objects_in_view(entities, player_camera->getViewMatrix());
	}
	else
	{
		in_view = get_objects_in_view(entities, camera->getViewMatrix());
	}

	shared_ptr<PlayerCamera> p_test = dynamic_pointer_cast<PlayerCamera>(camera);
	if (p_test != nullptr)
	{
		p_test->reorient(entities, chewy);
	}
	glUseProgram(0);


	if (usePhong) {
		glUseProgram(shaders.at("phongShader")->getProgramID());
		glViewport(0, 0, screen_width, screen_height);
		for (int i = 0; i < in_view.size(); i++)
			shaders.at("phongShader")->draw(camera->getViewMatrix(), in_view.at(i));
	}
	//otherwise deferred rendering
	else {
		vector<Light*> lights;
		for (int i = 0; i < entities.size(); i++) {
			//even if lantern culled still need light from it
			if (typeid(*entities[i]) == typeid(LightEntity) && SHOULD_DRAW(entities[i]->list)) {
				shared_ptr<LightEntity> le = dynamic_pointer_cast<LightEntity>(entities[i]);
				lights.push_back(&le->light);
			}
			//if there's an arrow have archery camera follow it and make game slow-mo
			if (typeid(*entities[i]) == typeid(ProjectileEntity)) {
				archery_camera->cameraPosition = entities[i]->bounding_box.center - archery_camera->cameraFront * 4.0f;
			}

			if (!SHOULD_DRAW(entities[i]->list)) {
				entities.erase(entities.begin() + i);
				i--;
			}
		}
		for (int i = 0; i < in_view.size(); i++)
		{
			if (!SHOULD_DRAW(in_view[i]->list)) {
				in_view.erase(in_view.begin() + i);
				i--;
			}
		}
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shaders.at("defShader")->getProgramID());
		glViewport(0, 0, screen_width, screen_height);
		shaders.at("defShader")->draw(camera, in_view, lights);
	}

    glUseProgram(0);
	if (debug_enabled)
	{
		glUseProgram(debugShader->getProgramID());
		for (int i = 0; i < in_view.size(); i++)
		{
			EntityBox box = in_view.at(i)->bounding_box;
			shared_ptr<vector<pair<vec3, vec3>>> points = box.get_line_segments();
			for (int j = 0; j < points->size(); j++)
			{
				draw_line(points->at(j).first, points->at(j).second, vec3(1.f, 0.f, 0.f));
			}
		}
		glUseProgram(0);
	}

	glUseProgram(debugShader->getProgramID());
	for (int i = debugShaderQueue.size() - 1; i >= 0; i--)
	{
		debugShaderQueue.at(i)();
	}
	if (!time_stopped)
	{
		debugShaderQueue.clear();
	}
	glUseProgram(0);

	hud.draw();
}

void World::key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (action == GLFW_PRESS)
        keys[key] = true;
    else if (action == GLFW_RELEASE)
        keys[key] = false;
}

void World::mouse_callback(GLFWwindow* window, double x_position, double y_position)
{
    x_offset = x_position;
    y_offset = -1.f * y_position;

    glfwSetCursorPos(window, 0, 0);
}

void World::change_camera()
{
    if (keys[GLFW_KEY_1])
    {
        camera = debug_camera;
        debug_camera->in_use = true;
        player_camera->in_use = false;
        archery_camera->in_use = false;

    }
    if (keys[GLFW_KEY_2])
    {
        camera = player_camera;
        debug_camera->in_use = false;
        player_camera->in_use = true;
        archery_camera->in_use = false;
    }
    if (keys[GLFW_KEY_3])
    {
        camera = archery_camera;
        debug_camera->in_use = false;
        player_camera->in_use = false;
        archery_camera->in_use = true;
    }
}

void World::enable_debugging()
{
	if (keys[GLFW_KEY_Z])
		debug_enabled = true;
	if (keys[GLFW_KEY_X])
		debug_enabled = false;
}
void World::cancel_cinematic()
{
	if (keys[GLFW_KEY_0])
	{
		cinematic_runthrough = true;
		camera = player_camera;
		player_camera->in_use = true;
	}
}
void World::update_key_callbacks()
{
    camera->movement(chewy);
	if (!cinematic_runthrough)
	{
		cancel_cinematic();
		if (cinematic_camera->pathing.done)
		{
			cinematic_runthrough = true;
			camera = player_camera;
			player_camera->in_use = true;
		}
	}
	else
	{
		change_camera();
		enable_debugging();
		stop_time();
	}
    x_offset = 0;
    y_offset = 0;
}

void World::stop_time()
{
	if (keys[GLFW_KEY_T])
		time_stopped = true;
	if (keys[GLFW_KEY_Y])
		time_stopped = false;
}

void World::update()
{
	static float start_time = 0.0;

	shootArrows();

	for (int i = 0; i < entities.size(); i++)
	{
		entities[i]->update();
		shared_ptr<GuardEntity> guard_temp = dynamic_pointer_cast<GuardEntity>(entities[i]);
		if (guard_temp != nullptr && guard_temp->check_view(chewy, entities) && state != SPOTTED) {
			//AudioManager::instance()->playAmbient(assetPath + "jons_breakthrough_performance.wav", 3.0f);
			AudioManager::instance()->play3DLoop(assetPath + "jons_breakthrough_performance.wav", guard_temp->getPosition(), false);
			state = SPOTTED;
		}
	}
	if (!time_stopped)
	{
		seconds_passed = (glfwGetTime() - start_time) * game_speed;
	}
	else
	{
		seconds_passed = 0.f;
	}
	start_time = glfwGetTime();
	Voxel vox(vec3(0, 0.f, 0.f), vec3(250.f, 250.f, 250.f));
	OctTree* world_oct_tree = new OctTree(vox, entities);
	world_oct_tree->handle_collisions();
	delete world_oct_tree;
    update_key_callbacks();
	_skybox->update();

	AudioManager::instance()->updateListener(camera->cameraPosition, camera->cameraFront, camera->cameraUp);
}

void World::scroll_callback(GLFWwindow* window, double x_pos, double y_pos)
{
	if (dynamic_pointer_cast<PlayerCamera>(camera))
	{
		shared_ptr<PlayerCamera> radius_changer = dynamic_pointer_cast<PlayerCamera>(camera);
		if (radius_changer)
			radius_changer->update_radius(y_pos);
	}
	else if (dynamic_pointer_cast<ArcheryCamera>(camera))
	{
		bow_strength -= y_pos * .05;
		bow_strength = bow_strength < 0.01 ? 0.01 : bow_strength;
		bow_strength = bow_strength > 1.0 ? 1.0 : bow_strength;
	}
}

void World::draw_line(vec3 p1, vec3 p2, vec3 color)
{
	glUseProgram(debugShader->getProgramID());
	debugShaderQueue.push_back([=](){debugShader->drawLine(p1, p2, color, camera->getViewMatrix()); });
	glUseProgram(0);
}

void World::draw_point(vec3 p, vec3 color, float radius)
{
	glUseProgram(debugShader->getProgramID());
	debugShaderQueue.push_back([=](){debugShader->drawPoint(p, color, radius, camera->getViewMatrix());  });
	glUseProgram(0);
}

void World::draw_box(shared_ptr<EntityBox> box, vec3 color)
{
	shared_ptr<vector<pair<vec3, vec3>>> points = box->get_line_segments();
	for (int j = 0; j < points->size(); j++)
	{
		draw_line(points->at(j).first, points->at(j).second, vec3(1.f, 0.f, 0.f));
	}
}

World::~World() {
}