#include "World.h"
#include "ChewyEntity.h"
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
#include "FrustrumCulling.h"
#include "CollisionHandler.h"
#include "DebugShader.h"
#include <glm/gtx/rotate_vector.hpp>
#include "Skybox.h"
#include "SimpleTextureShader.h"

using namespace std;
using namespace glm;

bool keys[1024];
float seconds_passed = 0;
float x_offset;
float y_offset;
float screen_width = 1280;
float screen_height = 720;

static std::shared_ptr<Camera> camera;
static shared_ptr<DebugShader> debugShader;

World::World()
{
    debugShader = shared_ptr<DebugShader>(new DebugShader("debugVert.glsl", "debugFrag.glsl"));
	init();
    x_offset = 0;
    y_offset = 0;
}

void World::init()
{
	debug_camera = shared_ptr<Camera>(new DebugCamera());
    player_camera = shared_ptr<Camera>(new PlayerCamera());
    archery_camera = shared_ptr<Camera>(new ArcheryCamera());

    meshes.insert(pair<string, shared_ptr<MeshSet>>("chewy", shared_ptr<MeshSet>(new MeshSet(assetPath + "ninja_final2.dae"))));
	shared_ptr<GameEntity> chewy(new ChewyEntity(vec3(0.0, 0.0, 0.0), meshes.at("chewy"), player_camera));
	meshes.insert(pair<string, shared_ptr<MeshSet>>("guard", shared_ptr<MeshSet>(new MeshSet(assetPath + "samurai.dae"))));
	shared_ptr<GameEntity> guard(new GuardEntity(vec3(40.0, 0.0, -2.0), meshes.at("guard")));
	meshes.insert(pair<string, shared_ptr<MeshSet>>("arrow", shared_ptr<MeshSet>(new MeshSet(assetPath + "arrow.dae"))));
	shared_ptr<GameEntity> arrow(new ProjectileEntity(vec3(40.0f, 15.0f, -2.0f), meshes.at("arrow"), chewy, archery_camera));
    meshes.insert(pair<string, shared_ptr<MeshSet>>("tower", shared_ptr<MeshSet>(new MeshSet(assetPath + "tower.dae"))));

    shared_ptr <GameEntity> tower(new ObstacleEntity(vec3(0.0, 0.0, 0.0), meshes.at("tower")));
    tower->scale = 30.0f;
	meshes.insert(pair<string, shared_ptr<MeshSet>>("lantern", shared_ptr<MeshSet>(new MeshSet(assetPath + "lantern.dae"))));

    shared_ptr <GameEntity> lantern(new ObstacleEntity(vec3(30.0, 16.0, 31.5), meshes.at("lantern")));
    lantern->rotations.y = M_PI_2;
	meshes.insert(pair<string, shared_ptr<MeshSet>>("lanternPole", shared_ptr<MeshSet>(new MeshSet(assetPath + "lanternPole.dae"))));
    shared_ptr <GameEntity> lantern_pole(new ObstacleEntity(vec3(30.0, 0.0, 30.0), meshes.at("lanternPole")));
	meshes.insert(pair<string, shared_ptr<MeshSet>>("closedBarrel", shared_ptr<MeshSet>(new MeshSet(assetPath + "closedBarrel.dae"))));
    shared_ptr <GameEntity> cBarrel(new ObstacleEntity(vec3(48.0, 0.0, 30.0), meshes.at("closedBarrel")));
    cBarrel->scale = 3.0f;
	meshes.insert(pair<string, shared_ptr<MeshSet>>("openBarrel", shared_ptr<MeshSet>(new MeshSet(assetPath + "openBarrel.dae"))));
    shared_ptr <GameEntity> oBarrel(new ObstacleEntity(vec3(30.0, 0.0, 40.0), meshes.at("openBarrel")));
    oBarrel->scale = 3.0f;
	meshes.insert(pair<string, shared_ptr<MeshSet>>("box", shared_ptr<MeshSet>(new MeshSet(assetPath + "Box.dae"))));
    shared_ptr <GameEntity> box1(new ObstacleEntity(vec3(50.0, 0.0, 20.0), meshes.at("box")));
    box1->scale = 3.0f;
    shared_ptr <GameEntity> box2(new ObstacleEntity(vec3(50.0, 7.0, 20.0), meshes.at("box")));
    box2->scale = 3.0f;
    shared_ptr <GameEntity> box3(new ObstacleEntity(vec3(50.0, 14.0, 20.0), meshes.at("box")));
    box3->scale = 3.0f;
	meshes.insert(pair<string, shared_ptr<MeshSet>>("skybox", shared_ptr<MeshSet>(new MeshSet(assetPath + "skybox.dae"))));
	_skybox = std::make_shared<Skybox>(Skybox(&camera, meshes.at("skybox")));
	_skybox->scale = 750.f;

    camera = player_camera;
    player_camera->in_use = true;
	entities.push_back(chewy);
	entities.push_back(guard);
	entities.push_back(arrow);
    //entities.push_back(tower);
    entities.push_back(lantern);
    entities.push_back(lantern_pole);
    entities.push_back(oBarrel);
    entities.push_back(cBarrel);
    entities.push_back(box1);
    entities.push_back(box2);
    entities.push_back(box3);



	shared_ptr<Shader> phongShader(new PhongShader("phongVert.glsl", "phongFrag.glsl"));
	shaders.insert(pair<string, shared_ptr<Shader>>("phongShader", phongShader));

	shared_ptr<Shader> defShader(new DeferredShader("DeferredVertShader.glsl", "DeferredFragShader.glsl"));
	shaders.insert(pair<string, shared_ptr<Shader>>("defShader", defShader));

    shaders.insert(pair<string, shared_ptr<Shader>>("debugShader", debugShader));

	shared_ptr<Shader> simpleShader(new SimpleTextureShader("simpleVert.glsl", "simpleFrag.glsl"));
	shaders.insert(pair<string, shared_ptr<Shader>>("simpleShader", simpleShader));

	//shared_ptr<Shader> textDebugShader(new TextureDebugShader());
	//shaders.insert(pair<string, shared_ptr<Shader>>("textureDebugShader", textDebugShader));
}

void World::draw()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, screen_width, screen_height);

	//glUseProgram(0);
	glUseProgram(shaders.at("simpleShader")->getProgramID());
	glViewport(0, 0, screen_width, screen_height);
	shaders.at("simpleShader")->draw(camera->getViewMatrix(), _skybox);

	shared_ptr<DebugCamera> c_test = dynamic_pointer_cast<DebugCamera>(camera);
	vector<shared_ptr<GameEntity>> culled;
    if (c_test != nullptr)
    {
        culled = cull_objects(entities, camera->getViewMatrix());
		//glUseProgram(0);
        glUseProgram(shaders.at("debugShader")->getProgramID());
        //shared_ptr<Shader> d_test = shaders.at("debugShader");
        for (int i = 0; i < culled.size(); i++)
        {
            shared_ptr<BoundingBox> box = culled.at(i)->getTransformedOuterBoundingBox();
			shared_ptr<vector<pair<vec3, vec3>>> points = box->get_points();
			for (int j = 0; j < points->size(); j++)
			{
				debugShader->drawLine(points->at(j).first, points->at(j).second, vec3(1.f, 0.f, 0.f), camera->getViewMatrix());
			}
            vector<pair<glm::vec3, glm::vec3>> planes = box->getPlanes();
            for (int i = 0; i < planes.size(); i++)
            {
                debugShader->drawLine(planes.at(i).first, planes.at(i).first + planes.at(i).second, vec3(0, 1.f, 0), camera->getViewMatrix());
            }
        }
	}
	else
	{
		culled = cull_objects(entities, camera->getViewMatrix());
		
	}
	
	//glUseProgram(0);
    glUseProgram(shaders.at("phongShader")->getProgramID());
    glViewport(0, 0, screen_width, screen_height);
	for (int i = 0; i < culled.size(); i++)
		shaders.at("phongShader")->draw(camera->getViewMatrix(), culled.at(i));

    glUseProgram(0);
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

void World::update_key_callbacks()
{
    camera->movement(entities.at(0));
    change_camera();

    x_offset = 0;
    y_offset = 0;
}

void World::update()
{
    static float start_time = 0.0;

    OctTree* world_oct_tree = new OctTree(Voxel(vec3(-1000.f, -1000.f, -1000.f), vec3(1000.f, 1000.f, 1000.f)), entities, nullptr);
    collision_handler(world_oct_tree->collision_pairs);

	float end_time = glfwGetTime();
	for (int i = 0; i < entities.size(); i++) 
		entities[i]->update();
	seconds_passed = end_time - start_time;
	start_time = glfwGetTime();

    update_key_callbacks();
	_skybox->update();
}

void World::scroll_callback(GLFWwindow* window, double x_pos, double y_pos)
{
    shared_ptr<PlayerCamera> radius_changer = dynamic_pointer_cast<PlayerCamera>(camera);
    if (radius_changer)
        radius_changer->update_radius(y_pos);
}

void World::draw_line(vec3 p1, vec3 p2, vec3 color)
{
    debugShader->drawLine(p1, p2, color, camera->getViewMatrix());
}