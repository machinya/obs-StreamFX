// AUTOGENERATED COPYRIGHT HEADER START
// Copyright (C) 2023 Michael Fabian 'Xaymar' Dirks <info@xaymar.com>
// AUTOGENERATED COPYRIGHT HEADER END

#include "source-sink.hpp"
#include "strings.hpp"
#include <bitset>
#include <cstring>
#include <functional>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>
#include "obs/gs/gs-helper.hpp"
#include "obs/obs-source-tracker.hpp"
#include "obs/obs-tools.hpp"
#include "util/util-logging.hpp"

#ifdef _DEBUG
#define ST_PREFIX "<%s> "
#define D_LOG_ERROR(x, ...) P_LOG_ERROR(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_WARNING(x, ...) P_LOG_WARN(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_INFO(x, ...) P_LOG_INFO(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#define D_LOG_DEBUG(x, ...) P_LOG_DEBUG(ST_PREFIX##x, __FUNCTION_SIG__, __VA_ARGS__)
#else
#define ST_PREFIX "<source::sink> "
#define D_LOG_ERROR(...) P_LOG_ERROR(ST_PREFIX __VA_ARGS__)
#define D_LOG_WARNING(...) P_LOG_WARN(ST_PREFIX __VA_ARGS__)
#define D_LOG_INFO(...) P_LOG_INFO(ST_PREFIX __VA_ARGS__)
#define D_LOG_DEBUG(...) P_LOG_DEBUG(ST_PREFIX __VA_ARGS__)
#endif

#define ST_I18N "Source.Sink"
#define ST_I18N_VIDEO ST_I18N ".Video"
#define ST_KEY_VIDEO "Video"
#define ST_I18N_VIDEO_SOURCE ST_I18N_VIDEO ".Source"
#define ST_KEY_VIDEO_SOURCE ST_KEY_VIDEO ".Source"
#define ST_I18N_AUDIO ST_I18N ".Audio"
#define ST_KEY_AUDIO "Audio"
#define ST_I18N_AUDIO_SOURCE ST_I18N_AUDIO ".Source"
#define ST_KEY_AUDIO_SOURCE ST_KEY_AUDIO ".Source"

using namespace streamfx::source::sink;

sink_instance::sink_instance(obs_data_t* settings, obs_source_t* self) : obs::source_instance(settings, self)
{
	update(settings);
}

sink_instance::~sink_instance()
{
}

uint32_t sink_instance::get_width()
{
	return 1;
}

uint32_t sink_instance::get_height()
{
	return 1;
}

void sink_instance::load(obs_data_t* data)
{
	update(data);
}

void sink_instance::migrate(obs_data_t* data, uint64_t version) {}

void sink_instance::update(obs_data_t* data) {}

void sink_instance::save(obs_data_t* data) {}

void sink_instance::video_tick(float_t time) {}

void sink_instance::video_render(gs_effect_t* effect) {}

void sink_instance::enum_active_sources(obs_source_enum_proc_t cb, void* ptr) {}

void sink_instance::enum_all_sources(obs_source_enum_proc_t cb, void* ptr) {}

sink_factory::sink_factory()
{
	_info.id           = S_PREFIX "source-sink";
	_info.type         = OBS_SOURCE_TYPE_INPUT;
	_info.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_ASYNC | OBS_SOURCE_AUDIO;

	finish_setup();
}

sink_factory::~sink_factory() {}

const char* sink_factory::get_name()
{
	return D_TRANSLATE(ST_I18N);
}

void sink_factory::get_defaults2(obs_data_t* data)
{
}

obs_properties_t* sink_factory::get_properties2(sink_instance* data)
{
	obs_properties_t* pr = obs_properties_create();

	{
		auto grp = obs_properties_create();
		auto pgrp = obs_properties_add_group(pr, ST_KEY_VIDEO, D_TRANSLATE(ST_I18N_VIDEO), OBS_GROUP_CHECKABLE, grp);

		{
			auto p = obs_properties_add_list(grp, ST_KEY_VIDEO_SOURCE, D_TRANSLATE(ST_KEY_VIDEO_SOURCE), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
		}
	}

	{
		auto grp  = obs_properties_create();
		auto pgrp = obs_properties_add_group(pr, ST_KEY_AUDIO, D_TRANSLATE(ST_I18N_AUDIO), OBS_GROUP_CHECKABLE, grp);

		{
			auto p = obs_properties_add_list(grp, ST_KEY_AUDIO_SOURCE, D_TRANSLATE(ST_KEY_AUDIO_SOURCE), OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
		}
	}

	return pr;
}

std::shared_ptr<sink_factory> sink_factory::instance()
{
	static std::weak_ptr<sink_factory> winst;
	static std::mutex                  mtx;

	std::unique_lock<decltype(mtx)> lock(mtx);
	auto                            instance = winst.lock();
	if (!instance) {
		instance = std::shared_ptr<sink_factory>(new sink_factory());
		winst    = instance;
	}
	return instance;
}

static std::shared_ptr<sink_factory> loader_instance;

static auto loader = streamfx::component(
	"spoutsink::sink",
	[]() { // Initializer
		loader_instance = sink_factory::instance();
	},
	[]() { // Finalizer
		loader_instance.reset();
	},
	{});
