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

	enum Command : uint8_t {
		Cmd_Jump = 0,
		Cmd_Red,
		Cmd_Blue,
		Cmd_Green,
		Cmd_Yellow,
		Cmd_Forward,
		Cmd_Backward,
		Cmd_Left,
		Cmd_Right,
		Cmd_Spin,
		CommandCount
	};

	Command current_command = Cmd_Jump;
	bool current_is_simon = false;

	Sound::Sample const *pending_sample = nullptr;
	float pending_delay = 0.0f;

	// Command Loop

	enum Phase : uint8_t {
		Phase_Waiting,
		Phase_Speaking,
		Phase_Listening,
		Phase_Dead
	};
	Phase phase = Phase_Waiting;

	float phase_timer = 0.0f;   //counts down
	float window_length = 2.5f; //current response window
	float gap_length = 1.0f;    //pause between commands
	uint32_t score = 0;

	float hold_time[4] = {0.0f, 0.0f, 0.0f, 0.0f}; //left, right, forward, back
	float spin_accum = 0.0f;
	bool jumped = false;

	float holdRequired = 0.5f;
	float spinRequired = 4.0f;
	float minWindow = 0.9f;
	float minGap = 0.25f;

	void issue_command(Command cmd, bool simon);
	void issue_random_command();
	void start_window();
	void resolve(bool success);
	bool command_performed() const;

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
	
	//camera:
	Scene::Camera *camera = nullptr;

};
