/**
 * YOLO 识别滤镜 - 最简单的透传版本
 * 完全参考 OBS 官方最简单的滤镜实现
 */

#include <obs-module.h>
#include <util/base.h>

struct YoloFilterData {
	obs_source_t *context;
};

static const char *yolo_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("YoloRecognizer");
}

static void *yolo_filter_create(obs_data_t *settings, obs_source_t *source)
{
	YoloFilterData *filter = new YoloFilterData();
	filter->context = source;
	
	blog(LOG_INFO, "[YOLO] 滤镜创建成功！");
	
	obs_source_update(source, settings);
	return filter;
}

static void yolo_filter_destroy(void *data)
{
	if (!data)
		return;
	
	YoloFilterData *filter = static_cast<YoloFilterData *>(data);
	blog(LOG_INFO, "[YOLO] 滤镜销毁");
	delete filter;
}

static void yolo_filter_defaults(obs_data_t *settings)
{
	UNUSED_PARAMETER(settings);
}

static obs_properties_t *yolo_filter_properties(void *data)
{
	UNUSED_PARAMETER(data);
	
	obs_properties_t *props = obs_properties_create();
	obs_properties_add_text(props, "info", "透传版本（视频正常显示）", OBS_TEXT_INFO);
	return props;
}

static void yolo_filter_update(void *data, obs_data_t *settings)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(settings);
}

/**
 * ============================================
 * 最简单的视频渲染 - 完全参考官方示例
 * ============================================
 */
static void yolo_filter_video_render(void *data, gs_effect_t *effect)
{
	YoloFilterData *filter = static_cast<YoloFilterData *>(data);
	
	if (!obs_source_process_filter_begin(filter->context, GS_RGBA, OBS_NO_DIRECT_RENDERING))
		return;
	
	obs_source_process_filter_end(filter->context, effect, 0, 0);
}

extern "C" {
struct obs_source_info yolo_filter_info = {
	.id = "yolo_recognizer_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = yolo_filter_name,
	.create = yolo_filter_create,
	.destroy = yolo_filter_destroy,
	.get_defaults = yolo_filter_defaults,
	.get_properties = yolo_filter_properties,
	.update = yolo_filter_update,
	.video_render = yolo_filter_video_render,
	.icon_type = OBS_ICON_TYPE_UNKNOWN
};
}
