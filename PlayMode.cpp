#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint room_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > room_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("Room.pnct"));
	room_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > room_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("Room.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = room_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = room_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

// Audio

Load< Sound::Sample > simon_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("simon.wav"));
});

Load< Sound::Sample > jump_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("jump.wav"));
});
Load< Sound::Sample > red_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("red.wav"));
});
Load< Sound::Sample > blue_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("blue.wav"));
});
Load< Sound::Sample > green_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("green.wav"));
});
Load< Sound::Sample > yellow_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("yellow.wav"));
});
Load< Sound::Sample > forward_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("forward.wav"));
});
Load< Sound::Sample > backward_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("backward.wav"));
});
Load< Sound::Sample > left_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("left.wav"));
});
Load< Sound::Sample > right_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("right.wav"));
});
Load< Sound::Sample > spin_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("spin.wav"));
});


static Sound::Sample const &command_sample(PlayMode::Command cmd) {
	switch (cmd) {
		case PlayMode::Cmd_Jump:     return *jump_sample;
		case PlayMode::Cmd_Red:      return *red_sample;
		case PlayMode::Cmd_Blue:     return *blue_sample;
		case PlayMode::Cmd_Green:    return *green_sample;
		case PlayMode::Cmd_Yellow:   return *yellow_sample;
		case PlayMode::Cmd_Forward:  return *forward_sample;
		case PlayMode::Cmd_Backward: return *backward_sample;
		case PlayMode::Cmd_Left:     return *left_sample;
		case PlayMode::Cmd_Right:    return *right_sample;
		case PlayMode::Cmd_Spin:     return *spin_sample;
		default: throw std::runtime_error("bad command");
	}
}


PlayMode::PlayMode() : scene(*room_scene) {
	static char const *tile_names[4] = { "Tile.Red", "Tile.Blue", "Tile.Green", "Tile.Yellow" };
	for (auto &transform : scene.transforms) {
		if (transform.name == "Player") player = &transform;
		else if (transform.name == tile_names[0]) tiles[0].transform = &transform;
		else if (transform.name == tile_names[1]) tiles[1].transform = &transform;
		else if (transform.name == tile_names[2]) tiles[2].transform = &transform;
		else if (transform.name == tile_names[3]) tiles[3].transform = &transform;
	}
	if (player == nullptr) throw std::runtime_error("Player not found.");
	if (tiles[0].transform == nullptr) throw std::runtime_error("Red Tile not found.");
	if (tiles[1].transform == nullptr) throw std::runtime_error("Blue Tile not found.");
	if (tiles[2].transform == nullptr) throw std::runtime_error("Green Tile not found.");
	if (tiles[3].transform == nullptr) throw std::runtime_error("Yellow Tile not found.");

	for (int i = 0; i < 4; i++) {
		tiles[i].center = glm::vec2(tiles[i].transform->position);
	}

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
	ground_eye_z = camera->transform->position.z;
	eye_z = ground_eye_z;


	//start music loop playing:
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_EVENT_KEY_DOWN) {
		if (evt.key.key == SDLK_ESCAPE) {
			SDL_SetWindowRelativeMouseMode(Mode::window, false);
			return true;
		} else if (evt.key.key == SDLK_A) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_D) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_W) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_S) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_SPACE) {
			if (on_ground) {
				z_velocity = jumpSpeed;
				on_ground = false;
				jumped = true;
			}
			return true;
		} else if (evt.key.key == SDLK_RETURN) {
			static std::mt19937 rng(std::random_device{}());
			issue_command(Command(rng() % CommandCount), (rng() % 2) == 0);
			return true;
		}
	} else if (evt.type == SDL_EVENT_KEY_UP) {
		if (evt.key.key == SDLK_A) {
			left.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_D) {
			right.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_W) {
			up.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_S) {
			down.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
		if (SDL_GetWindowRelativeMouseMode(Mode::window) == false) {
			SDL_SetWindowRelativeMouseMode(Mode::window, true);
			return true;
		}
	} else if (evt.type == SDL_EVENT_MOUSE_MOTION) {
		if (SDL_GetWindowRelativeMouseMode(Mode::window) == true) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			yaw   += -motion.x * camera->fovy * mouse_sen;
			float d = -motion.x * camera->fovy * mouse_sen;
			if (d * spin_accum < 0.0f) spin_accum = 0.0f;
			spin_accum += d;
			pitch +=  motion.y * camera->fovy * mouse_sen;
			pitch = glm::clamp(pitch, -1.4f, 1.4f);

			camera->transform->rotation =
				glm::angleAxis(yaw, glm::vec3(0.0f, 0.0f, 1.0f))
				* glm::angleAxis(pitch + 1.5707963f, glm::vec3(1.0f, 0.0f, 0.0f));
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	if (pending_sample != nullptr) {
		pending_delay -= elapsed;
		if (pending_delay <= 0.0f) {
			Sound::play(*pending_sample, 1.0f);
			pending_sample = nullptr;
		}
	}

	if (phase == Phase_Listening) {
		if (left.pressed)  hold_time[0] += elapsed;
		if (right.pressed) hold_time[1] += elapsed;
		if (up.pressed)    hold_time[2] += elapsed;
		if (down.pressed)  hold_time[3] += elapsed;
	}

	if (phase != Phase_Dead) {
		phase_timer -= elapsed;

		if (phase == Phase_Waiting && phase_timer <= 0.0f) {
			issue_random_command();
		} else if (phase == Phase_Speaking && phase_timer <= 0.0f) {
			start_window();
		} else if (phase == Phase_Listening) {
			bool did = command_performed();
			if (current_is_simon && did) {
				resolve(true);
			} else if (!current_is_simon && did) {
				resolve(false);
			} else if (phase_timer <= 0.0f) {
				resolve(current_is_simon ? false : true);
			}
		}
	}

	//move camera:
	{
		//combine inputs into a move:
		constexpr float PlayerSpeed = 12.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		glm::mat4x3 frame = camera->transform->make_parent_from_local();
		glm::vec3 frame_right = frame[0];
		glm::vec3 frame_forward = -frame[2];

		frame_right.z = 0.0f;
		frame_forward.z = 0.0f;
		if (frame_right != glm::vec3(0.0f)) frame_right = glm::normalize(frame_right);
		if (frame_forward != glm::vec3(0.0f)) frame_forward = glm::normalize(frame_forward);

		camera->transform->position += move.x * frame_right + move.y * frame_forward;

		//keep the player inside the room and at eye height:
		glm::vec3 &pos = camera->transform->position;
		pos.x = glm::clamp(pos.x, -14.0f, 14.0f);
		pos.y = glm::clamp(pos.y, -19.0f, 19.0f);
	}

	{ //Jump
		float support_z = 0.0f; //floor
		int t = tile_under_player();
		if (t != -1) support_z = tiles[t].top_z;

		float target_eye_z = support_z + eye_height;

		z_velocity += gravity * elapsed;
		eye_z += z_velocity * elapsed;

		if (eye_z <= target_eye_z) {
			eye_z = target_eye_z;
			z_velocity = 0.0f;
			on_ground = true;
		} else {
			on_ground = false;
		}
		camera->transform->position.z = eye_z;
	}

	{ //update listener to camera position:
		glm::mat4x3 frame = camera->transform->make_parent_from_local();
		glm::vec3 frame_right = frame[0];
		glm::vec3 frame_at = frame[3];
		Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
	}

	{
		glm::vec3 cam_pos = camera->transform->position;
		glm::vec3 offset = glm::vec3(std::sin(yaw), -std::cos(yaw), 0.0f);
		player->position = glm::vec3(cam_pos.x, cam_pos.y, cam_pos.z - 1.8f) + offset;
    	player->rotation = glm::angleAxis(yaw, glm::vec3(0.0f, 0.0f, 1.0f));
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	if (phase == Phase_Dead) {
		glClearColor(0.35f, 0.02f, 0.02f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	} else {
		glUseProgram(lit_color_texture_program->program);
		glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
		glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
		glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
		glUseProgram(0);

		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

		scene.draw(*camera);
	}

	static char const *cmd_names[CommandCount] = {"JUMP","RED","BLUE","GREEN","YELLOW","FORWARD","BACKWARD","LEFT","RIGHT","SPIN"};
	std::string status;
	if (phase == Phase_Dead) status = "DEAD";
	else status = std::string(current_is_simon ? "SIMON SAYS " : "") + cmd_names[current_command];
	

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;

		auto draw_text = [&](std::string const &text, float x, float y, float size) {
			lines.draw_text(text, glm::vec3(x, y, 0.0f),
				glm::vec3(size, 0.0f, 0.0f), glm::vec3(0.0f, size, 0.0f),
				glm::u8vec4(0x00, 0x00, 0x00, 0x00));
			float ofs = 2.0f / drawable_size.y;
			lines.draw_text(text, glm::vec3(x + ofs, y + ofs, 0.0f),
				glm::vec3(size, 0.0f, 0.0f), glm::vec3(0.0f, size, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		};

		draw_text("Score: " + std::to_string(score), -aspect + 0.1f * H, -1.0f + 0.5f * H, H);

		if (phase == Phase_Dead) {
			draw_text("GAME OVER", -aspect * 0.15f, 0.0f, H * 1.5f);
			draw_text("Final Score: " + std::to_string(score), -aspect * 0.2f, -0.15f, H * 1.5f);
		}
	}
	GL_ERRORS();
}

int PlayMode::tile_under_player() const {
	glm::vec2 p = glm::vec2(camera->transform->position);
	for (int i = 0; i < 4; ++i) {
		if (glm::length(p - tiles[i].center) < tiles[i].radius) return i;
	}
	return -1;
}

void PlayMode::issue_command(Command cmd, bool simon_says) {
	current_command = cmd;
	current_is_simon = simon_says;

	if (simon_says) {
		Sound::play(*simon_sample, 1.0f);
		pending_sample = &command_sample(cmd);
		pending_delay = simon_sample->data.size() / 48000.0f + 0.05f;
	} else {
		Sound::play(command_sample(cmd), 1.0f);
		pending_sample = nullptr;
	}
}

void PlayMode::issue_random_command() {
	static std::mt19937 rng(std::random_device{}());
	Command cmd = Command(rng() % CommandCount);
	bool simon = (rng() % 100) < 60; //60% "Simon says"
	issue_command(cmd, simon);

	phase = Phase_Speaking;
	phase_timer = (simon ? pending_delay : 0.0f) + command_sample(cmd).data.size() / 48000.0f;
}

void PlayMode::start_window() {
	hold_time[0] = hold_time[1] = hold_time[2] = hold_time[3] = 0.0f;
	spin_accum = 0.0f;
	jumped = false;
	phase = Phase_Listening;
	phase_timer = window_length;
}

void PlayMode::resolve(bool success) {
	if (!success) {
		phase = Phase_Dead;
		Sound::stop_all_samples();
		return;
	}
	score += 1;
	window_length = std::max(minWindow, window_length * 0.96f);
	gap_length = std::max(minGap,    gap_length * 0.95f);
	phase = Phase_Waiting;
	phase_timer = gap_length;
}

bool PlayMode::command_performed() const {
	switch (current_command) {
		case Cmd_Jump:     return jumped;
		case Cmd_Red:      return tile_under_player() == 0;
		case Cmd_Blue:     return tile_under_player() == 1;
		case Cmd_Green:    return tile_under_player() == 2;
		case Cmd_Yellow:   return tile_under_player() == 3;
		case Cmd_Left:     return hold_time[0] >= holdRequired;
		case Cmd_Right:    return hold_time[1] >= holdRequired;
		case Cmd_Forward:  return hold_time[2] >= holdRequired;
		case Cmd_Backward: return hold_time[3] >= holdRequired;
		case Cmd_Spin:     return std::abs(spin_accum) >= spinRequired;
		default: return false;
	}
}
