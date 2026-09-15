#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	struct Tile {
		Scene::Transform *transform = nullptr;
		glm::vec2 center = glm::vec2(0.0f);
		float radius = 2.5f;
		float top_z = 0.3f;
	};
	Tile tiles[4];
	int tile_under_player() const;

	Scene::Transform *player = nullptr;

	float gravity = -25.0f;
	float jumpSpeed = 10.0f;
	float yaw = 0.0f;
	float pitch = 0.0f;
	float ground_eye_z = 4.0f;
	float eye_height = 4.0f;
	float eye_z = 4.0f;
	float z_velocity = 0.0f;
	bool on_ground = true;
	float mouse_sen = 2.5f;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;
	//car honk sound:
	std::shared_ptr< Sound::PlayingSample > honk_oneshot;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
