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

// Load< Sound::Sample > dusty_floor_sample(LoadTagDefault, []() -> Sound::Sample const * {
// 	return new Sound::Sample(data_path("dusty-floor.opus"));
// });


// Load< Sound::Sample > honk_sample(LoadTagDefault, []() -> Sound::Sample const * {
// 	return new Sound::Sample(data_path("honk.wav"));
// });


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
			}
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

	int t = tile_under_player();
	static char const *names[4] = { "RED", "BLUE", "GREEN", "YELLOW" };
	std::string status = (t == -1 ? "none" : names[t]);

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
		lines.draw_text("Mouse motion rotates camera; WASD moves, Tile: " + status,
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Mouse motion rotates camera; WASD moves, Tile: " + status,
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
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
