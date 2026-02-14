#include <obs-module.h>
#include <graphics/graphics.h>
#include <stdlib.h>

struct yolo_filter_data {
	obs_source_t *context;
	int render_count;
};

static const char *yolo_filter_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "YOLO 识别 (调试版)";
}

static void *yolo_filter_create(obs_data_t *settings, obs_source_t *context)
{
	struct yolo_filter_data *filter = bzalloc(sizeof(struct yolo_filter_data));
	filter->context = context;
	filter->render_count = 0;
	
	UNUSED_PARAMETER(settings);
	
	blog(LOG_INFO, "[YOLO Filter DEBUG] ===== CREATE CALLED =====");
	blog(LOG_INFO, "[YOLO Filter DEBUG] Context: %p", context);
	
	return filter;
}

static void yolo_filter_destroy(void *data)
{
	struct yolo_filter_data *filter = data;
	
	blog(LOG_INFO, "[YOLO Filter DEBUG] ===== DESTROY CALLED =====");
	blog(LOG_INFO, "[YOLO Filter DEBUG] Total renders: %d", filter->render_count);
	
	bfree(filter);
}

static void yolo_filter_video_render(void *data, gs_effect_t *effect)
{
	struct yolo_filter_data *filter = data;
	filter->render_count++;
	
	// 只在前10帧打印日志，避免刷屏
	if (filter->render_count <= 10) {
		blog(LOG_INFO, "[YOLO Filter DEBUG] ===== RENDER #%d =====", filter->render_count);
		blog(LOG_INFO, "[YOLO Filter DEBUG] Effect param: %p", effect);
	}
	
	// 检查能否开始处理
	bool begin_result = obs_source_process_filter_begin(filter->context, 
	                                                     GS_RGBA,
	                                                     OBS_NO_DIRECT_RENDERING);
	
	if (filter->render_count <= 10) {
		blog(LOG_INFO, "[YOLO Filter DEBUG] Begin result: %s", begin_result ? "SUCCESS" : "FAILED");
	}
	
	if (!begin_result) {
		if (filter->render_count <= 10) {
			blog(LOG_ERROR, "[YOLO Filter DEBUG] Begin failed! Returning early.");
		}
		return;
	}
	
	// 获取默认effect
	gs_effect_t *default_effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
	
	if (filter->render_count <= 10) {
		blog(LOG_INFO, "[YOLO Filter DEBUG] Default effect: %p", default_effect);
	}
	
	// 结束处理
	obs_source_process_filter_end(filter->context, default_effect, 0, 0);
	
	if (filter->render_count <= 10) {
		blog(LOG_INFO, "[YOLO Filter DEBUG] End called successfully");
	}
}

static void yolo_filter_defaults(obs_data_t *settings)
{
	UNUSED_PARAMETER(settings);
	blog(LOG_INFO, "[YOLO Filter DEBUG] ===== DEFAULTS CALLED =====");
}

static obs_properties_t *yolo_filter_properties(void *data)
{
	UNUSED_PARAMETER(data);
	
	blog(LOG_INFO, "[YOLO Filter DEBUG] ===== PROPERTIES CALLED =====");
	
	obs_properties_t *props = obs_properties_create();
	obs_properties_add_text(props, "info", 
	                       "YOLO识别滤镜 - 调试版本 (查看OBS日志)", 
	                       OBS_TEXT_INFO);
	
	return props;
}

static void yolo_filter_update(void *data, obs_data_t *settings)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(settings);
	blog(LOG_INFO, "[YOLO Filter DEBUG] ===== UPDATE CALLED =====");
}

struct obs_source_info yolo_filter_info = {
	.id = "yolo_recognizer_filter_debug",
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