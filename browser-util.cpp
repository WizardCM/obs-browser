#include "browser-util.hpp"
#include <obs-frontend-api.h>
#include <obs.hpp>

using namespace json11;

void parse_browser_message(const std::string &name, Json &json)
{
	if (name == "getCurrentScene") {
		OBSSource current_scene = obs_frontend_get_current_scene();
		obs_source_release(current_scene);

		if (!current_scene)
			return;

		const char *name = obs_source_get_name(current_scene);
		if (!name)
			return;

		json = Json::object{
			{"name", name},
			{"width", (int)obs_source_get_width(current_scene)},
			{"height", (int)obs_source_get_height(current_scene)}};

	} else if (name == "getStatus") {
		json = Json::object{
			{"recording", obs_frontend_recording_active()},
			{"streaming", obs_frontend_streaming_active()},
			{"recordingPaused", obs_frontend_recording_paused()},
			{"replaybuffer", obs_frontend_replay_buffer_active()},
			{"virtualcam", obs_frontend_virtualcam_active()}};

	} else if (name == "saveReplayBuffer") {
		obs_frontend_replay_buffer_save();
	}
}
