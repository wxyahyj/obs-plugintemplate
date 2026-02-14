#include <obs-module.h>
#include <stdlib.h>

struct my_filter_data {
	obs_source_t *source;
};

static const char *my_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "YOLO 识别";
}

static void *my_filter_create(obs_data_t *settings, obs_source_t *source)
{
	struct my_filter_data *filter = (struct my_filter_data *)bzalloc(sizeof(struct my_filter_data));
	filter->source = source;

	blog(LOG_INFO, "[YOLO] 创建成功！");

	obs_source_update(source, settings);
	return filter;
}

static void my_filter_destroy(void *data)
{
	struct my_filter_data *filter = (struct my_filter_data *)data;
	if (filter)
		bfree(filter);
}

static void my_filter_defaults(obs_data_t *settings)
{
	UNUSED_PARAMETER(settings);
}

static obs_properties_t *my_filter_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *props = obs_properties_create();
	obs_properties_add_text(props, "info", "skip video filter 测试版", OBS_TEXT_INFO);
	return props;
}

static void my_filter_update(void *data, obs_data_t *settings)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(settings);
}

static void my_filter_video_render(void *data, gs_effect_t *effect)
{
	struct my_filter_data *filter = (struct my_filter_data *)data;
	
	blog(LOG_INFO, "[YOLO] video_render 被调用！");
	
	obs_source_skip_video_filter(filter->source);
}

struct obs_source_info yolo_filter_info = {
	.id             = "yolo_recognizer_filter",
	.type           = OBS_SOURCE_TYPE_FILTER,
	.output_flags   = OBS_SOURCE_VIDEO,
	.get_name       = my_filter_name,
	.create         = my_filter_create,
	.destroy        = my_filter_destroy,
	.get_defaults   = my_filter_defaults,
	.get_properties = my_filter_properties,
	.update         = my_filter_update,
	.video_render   = my_filter_video_render,
};
