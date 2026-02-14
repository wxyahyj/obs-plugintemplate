#include <obs-module.h>
#include <graphics/graphics.h>
#include <stdlib.h>

struct yolo_filter_data {
	obs_source_t *context;
};

static const char *yolo_filter_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "YOLO 识别";
}

static void *yolo_filter_create(obs_data_t *settings, obs_source_t *context)
{
	struct yolo_filter_data *filter = bzalloc(sizeof(struct yolo_filter_data));
	filter->context = context;
	
	UNUSED_PARAMETER(settings);
	
	blog(LOG_INFO, "[YOLO Filter] Created successfully");
	
	return filter;
}

static void yolo_filter_destroy(void *data)
{
	struct yolo_filter_data *filter = data;
	
	blog(LOG_INFO, "[YOLO Filter] Destroyed");
	
	bfree(filter);
}

static void yolo_filter_video_render(void *data, gs_effect_t *effect)
{
	struct yolo_filter_data *filter = data;
	
	// 步骤1: 开始处理 - 获取父源的纹理
	if (!obs_source_process_filter_begin(filter->context, GS_RGBA,
	                                     OBS_NO_DIRECT_RENDERING)) {
		return;
	}
	
	// 步骤2: 结束处理 - 使用默认effect渲染到输出
	// 这里使用OBS内置的default effect来直接绘制纹理
	obs_source_process_filter_end(filter->context, 
	                              obs_get_base_effect(OBS_EFFECT_DEFAULT), 
	                              0, 0);
}

static void yolo_filter_defaults(obs_data_t *settings)
{
	UNUSED_PARAMETER(settings);
	// 在这里可以设置默认参数
}

static obs_properties_t *yolo_filter_properties(void *data)
{
	UNUSED_PARAMETER(data);
	
	obs_properties_t *props = obs_properties_create();
	
	obs_properties_add_text(props, "info", 
	                       "YOLO识别滤镜 - 当前为透传模式", 
	                       OBS_TEXT_INFO);
	
	return props;
}

static void yolo_filter_update(void *data, obs_data_t *settings)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(settings);
	// 在这里处理设置更新
}

struct obs_source_info yolo_filter_info = {
	.id = "yolo_recognizer_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = yolo_filter_get_name,
	.create = yolo_filter_create,
	.destroy = yolo_filter_destroy,
	.video_render = yolo_filter_video_render,
	.get_defaults = yolo_filter_defaults,
	.get_properties = yolo_filter_properties,
	.update = yolo_filter_update,
};