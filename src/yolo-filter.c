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
	blog(LOG_INFO, "[YOLO] ********** my_filter_create 开始！**********");
	
	struct my_filter_data *filter = (struct my_filter_data *)bzalloc(sizeof(struct my_filter_data));
	filter->source = source;

	blog(LOG_INFO, "[YOLO] 创建成功！source = %p", source);

	obs_source_update(source, settings);
	
	blog(LOG_INFO, "[YOLO] ********** my_filter_create 结束！**********");
	return filter;
}

static void my_filter_destroy(void *data)
{
	blog(LOG_INFO, "[YOLO] my_filter_destroy 被调用！");
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
	obs_properties_add_text(props, "info", "直接透传版本", OBS_TEXT_INFO);
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
	
	blog(LOG_INFO, "[YOLO] video_render 被调用！开始渲染！");
	
	obs_source_t *target = obs_filter_get_target(filter->source);
	uint32_t width = 0;
	uint32_t height = 0;

	if (target) {
		width = obs_source_get_base_width(target);
		height = obs_source_get_base_height(target);
		blog(LOG_INFO, "[YOLO] 目标源：%p, 宽=%u, 高=%u", target, width, height);
	}

	if (width == 0 || height == 0) {
		blog(LOG_INFO, "[YOLO] 宽或高为0，直接透传！");
		obs_source_skip_video_filter(filter->source);
		return;
	}

	if (obs_source_process_filter_begin(filter->source, GS_RGBA, OBS_ALLOW_DIRECT_RENDERING)) {
		blog(LOG_INFO, "[YOLO] 调用 obs_source_process_filter_end，宽=%u, 高=%u", width, height);
		obs_source_process_filter_end(filter->source, effect, width, height);
	}
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
