// AUTOGENERATED COPYRIGHT HEADER START
// Copyright (C) 2019-2023 Michael Fabian 'Xaymar' Dirks <info@xaymar.com>
// AUTOGENERATED COPYRIGHT HEADER END

#include "obs-source-tracker.hpp"
#include "obs/obs-tools.hpp"
#include "plugin.hpp"
#include "util/util-logging.hpp"

#include "warning-disable.hpp"
#include <mutex>
#include <stdexcept>
#include "warning-enable.hpp"

#ifdef _DEBUG
#define ST_PREFIX "<%s> "
#define D_LOG_ERROR(x, ...) P_LOG_ERROR(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_WARNING(x, ...) P_LOG_WARN(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_INFO(x, ...) P_LOG_INFO(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_DEBUG(x, ...) P_LOG_DEBUG(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#else
#define ST_PREFIX "<obs::source_tracker> "
#define D_LOG_ERROR(...) P_LOG_ERROR(ST_PREFIX __VA_ARGS__)
#define D_LOG_WARNING(...) P_LOG_WARN(ST_PREFIX __VA_ARGS__)
#define D_LOG_INFO(...) P_LOG_INFO(ST_PREFIX __VA_ARGS__)
#define D_LOG_DEBUG(...) P_LOG_DEBUG(ST_PREFIX __VA_ARGS__)
#endif

streamfx::obs::source_tracker::source_tracker() : _sources(), _mutex()
{
	auto osi = obs_get_signal_handler();
	if (osi) {
		signal_handler_connect(osi, "source_create", &source_create_handler, this);
		signal_handler_connect(osi, "source_destroy", &source_destroy_handler, this);
		signal_handler_connect(osi, "source_rename", &source_rename_handler, this);
	} else {
		D_LOG_WARNING("No global signal handler was present at initialization.", nullptr)
	}

	// Enumerate all current sources and filters.
	obs_enum_all_sources(
		[](void* param, obs_source_t* source) {
			auto* self = reinterpret_cast<::streamfx::obs::source_tracker*>(param);
			self->insert_source(source);
			return true;
		},
		this);
}

streamfx::obs::source_tracker::~source_tracker()
{
	auto osi = obs_get_signal_handler();
	if (osi) {
		signal_handler_disconnect(osi, "source_create", &source_create_handler, this);
		signal_handler_disconnect(osi, "source_destroy", &source_destroy_handler, this);
		signal_handler_disconnect(osi, "source_rename", &source_rename_handler, this);
	}

	this->_sources.clear();
}

void streamfx::obs::source_tracker::enumerate(enumerate_cb_t ecb, filter_cb_t fcb)
{
	// Need func-local copy, otherwise we risk corruption if a new source is created or destroyed.
	decltype(_sources) _clone;
	{
		std::lock_guard<decltype(_mutex)> lock(_mutex);
		_clone = _sources;
	}

	for (auto kv : _clone) {
		auto wsource = kv.second;
		try {
			auto source = wsource.lock();

			if (fcb) {
				if (fcb(kv.first, source)) {
					continue;
				}
			}

			if (ecb) {
				if (ecb(kv.first, source)) {
					break;
				}
			}
		} catch (...) {
			continue;
		}
	}
}

void streamfx::obs::source_tracker::insert_source(obs_source_t* source)
{
	const char* name = obs_source_get_name(source);

	// Don't track unnamed sources.
	if (!name || (strnlen(name, 1) == 0)) {
		D_LOG_DEBUG("Unnamed source '0x%08zX' left untracked.", source);
		return;
	}

	// Insert the newly tracked source into the map.
	std::lock_guard<decltype(_mutex)> lock(_mutex);
	_sources.emplace(std::string{name}, ::streamfx::obs::weak_source{source});
}

void streamfx::obs::source_tracker::remove_source(obs_source_t* source)
{
	std::lock_guard<decltype(_mutex)> lock(_mutex);
	const char*                       name = obs_source_get_name(source);

	// Try and find the source by name.
	if (name) {
		if (auto kv = _sources.find(std::string{name}); kv != _sources.end()) {
			_sources.erase(kv);
			return;
		}
	}

	// Try and find the source by pointer.
	for (auto kv = _sources.begin(); kv != _sources.end(); kv++) {
		if (kv->second == source) {
			_sources.erase(kv);
			return;
		}
	}

	// If we're still here, there's something wrong.
	if (name) {
		D_LOG_ERROR("Attempt to remove untracked source '0x%08zX' with name %s failed.", source, name);
		throw std::runtime_error("Failed to find given source.");
	}
}

void streamfx::obs::source_tracker::rename_source(std::string_view old_name, std::string_view new_name, obs_source_t* source)
{
	if (old_name == new_name) {
		throw std::runtime_error("New and old name are identical.");
	}

	std::lock_guard<decltype(_mutex)> lock(_mutex);

	// Remove the previously tracked entry.
	if (auto kv = _sources.find(std::string{old_name}); kv != _sources.end()) {
		_sources.erase(kv);
	}

	// And then add the new entry.
	_sources.emplace(std::string{new_name}, ::streamfx::obs::weak_source{source});
}

bool streamfx::obs::source_tracker::filter_sources(std::string, ::streamfx::obs::source source)
{
	return (obs_source_get_type(source) != OBS_SOURCE_TYPE_INPUT);
}

bool streamfx::obs::source_tracker::filter_audio_sources(std::string, ::streamfx::obs::source source)
{
	uint32_t flags = obs_source_get_output_flags(source);
	return !(flags & OBS_SOURCE_AUDIO) || (obs_source_get_type(source) != OBS_SOURCE_TYPE_INPUT);
}

bool streamfx::obs::source_tracker::filter_video_sources(std::string, ::streamfx::obs::source source)
{
	uint32_t flags = obs_source_get_output_flags(source);
	return !(flags & OBS_SOURCE_VIDEO) || (obs_source_get_type(source) != OBS_SOURCE_TYPE_INPUT);
}

bool streamfx::obs::source_tracker::filter_transitions(std::string, ::streamfx::obs::source source)
{
	return (obs_source_get_type(source) != OBS_SOURCE_TYPE_TRANSITION);
}

bool streamfx::obs::source_tracker::filter_scenes(std::string, ::streamfx::obs::source source)
{
	return (obs_source_get_type(source) != OBS_SOURCE_TYPE_SCENE);
}

void streamfx::obs::source_tracker::source_create_handler(void* ptr, calldata_t* data) noexcept
{
	auto* self = reinterpret_cast<streamfx::obs::source_tracker*>(ptr);
	try {
		obs_source_t* source = nullptr;
		if (calldata_get_ptr(data, "source", &source); !source) {
			throw std::runtime_error("Missing 'source' parameter.");
		}

		self->insert_source(source);
	} catch (const std::exception& ex) {
		DLOG_ERROR("Event 'source_create' caused exception: %s", ex.what());
	} catch (...) {
		DLOG_ERROR("Event 'source_create' caused unknown exception.", nullptr);
	}
}

void streamfx::obs::source_tracker::source_destroy_handler(void* ptr, calldata_t* data) noexcept
{
	auto* self = reinterpret_cast<streamfx::obs::source_tracker*>(ptr);
	try {
		obs_source_t* source = nullptr;
		if (calldata_get_ptr(data, "source", &source); !source) {
			throw std::runtime_error("Missing 'source' parameter.");
		}

		self->remove_source(source);
	} catch (const std::exception& ex) {
		DLOG_ERROR("Event 'source_destroy' caused exception: %s", ex.what());
	} catch (...) {
		DLOG_ERROR("Event 'source_destroy' caused unknown exception.", nullptr);
	}
}

void streamfx::obs::source_tracker::source_rename_handler(void* ptr, calldata_t* data) noexcept
{
	auto* self = reinterpret_cast<streamfx::obs::source_tracker*>(ptr);
	try {
		obs_source_t* source = nullptr;
		if (calldata_get_ptr(data, "source", &source); !source) {
			throw std::runtime_error("Missing 'source' parameter.");
		}

		const char* old_name = nullptr;
		if (calldata_get_string(data, "prev_name", &old_name); !old_name) {
			throw std::runtime_error("Missing 'prev_name' parameter.");
		}

		const char* new_name = nullptr;
		if (calldata_get_string(data, "new_name", &new_name); !new_name) {
			throw std::runtime_error("Missing 'new_name' parameter.");
		}

		self->rename_source(old_name, new_name, source);
	} catch (...) {
		DLOG_ERROR("Unexpected exception in function '%s'.", __FUNCTION_NAME__);
	}
}

std::shared_ptr<streamfx::obs::source_tracker> streamfx::obs::source_tracker::instance()
{
	static std::weak_ptr<streamfx::obs::source_tracker> winst;
	static std::mutex                                   mtx;

	std::unique_lock<decltype(mtx)> lock(mtx);
	auto                            instance = winst.lock();
	if (!instance) {
		instance = std::shared_ptr<streamfx::obs::source_tracker>(new streamfx::obs::source_tracker());
		winst    = instance;
	}
	return instance;
}

static std::shared_ptr<streamfx::obs::source_tracker> loader_instance;

static auto loader = streamfx::component(
	"core::source_tracker",
	[]() { // Initializer
		loader_instance = streamfx::obs::source_tracker::instance();
	},
	[]() { // Finalizer
		loader_instance.reset();
	},
	{"core::threadpool"});
