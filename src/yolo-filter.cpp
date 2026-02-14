#include <obs-module.h>
#include <graphics/graphics.h>
#include <graphics/texrender.h>
#include <string>

struct my_filter_data {
	obs_source_t *source;
	gs_texrender_t *texrender;

	std::string modelPath;
	bool enableInference;
	float confidenceThreshold;
	int inferenceInterval;
	int modelInputSize;
	bool renderBoxes;
	bool followMode;
};

static const char *my_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "YOLO 识别";
}

static void *my_filter_create(obs_data_t *settings, obs_source_t *source)
{
	blog(LOG_INFO, "[YOLO] ********** my_filter_create 开始！**********");
	
	void *mem = bmalloc(sizeof(struct my_filter_data));
	struct my_filter_data *filter = new (mem) my_filter_data();
	filter->source = source;
	filter->texrender = nullptr;

	obs_enter_graphics();
	filter->texrender = gs_texrender_create(GS_BGRA, GS_ZS_NONE);
	obs_leave_graphics();

	blog(LOG_INFO, "[YOLO] 创建成功！source = %p, texrender = %p", source, filter->texrender);

	obs_source_update(source, settings);
	
	blog(LOG_INFO, "[YOLO] ********** my_filter_create 结束！**********");
	return filter;
}

static void my_filter_destroy(void *data)
{
	blog(LOG_INFO, "[YOLO] my_filter_destroy 被调用！");
	struct my_filter_data *filter = (struct my_filter_data *)data;
	if (filter) {
		if (filter->texrender) {
			obs_enter_graphics();
			gs_texrender_destroy(filter->texrender);
			obs_leave_graphics();
		}
		filter->~my_filter_data();
		bfree(filter);
	}
}

static void my_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "enable_inference", false);
	obs_data_set_default_double(settings, "confidence_threshold", 0.5);
	obs_data_set_default_int(settings, "inference_interval", 1);
	obs_data_set_default_int(settings, "model_input_size", 640);
	obs_data_set_default_bool(settings, "render_boxes", true);
	obs_data_set_default_bool(settings, "follow_mode", false);
}

static obs_properties_t *my_filter_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *props = obs_properties_create();
	
	obs_properties_add_path(props, "model_path", "模型路径", OBS_PATH_FILE, "ONNX 文件 (*.onnx);;所有文件 (*.*)", NULL);
	obs_properties_add_bool(props, "enable_inference", "启用推理");
	obs_properties_add_float_slider(props, "confidence_threshold", "置信度阈值", 0.0, 1.0, 0.01);
	obs_properties_add_int_slider(props, "inference_interval", "推理间隔（帧）", 1, 30, 1);
	obs_properties_add_int_slider(props, "model_input_size", "模型输入尺寸", 320, 1280, 32);
	obs_properties_add_bool(props, "render_boxes", "渲染检测框");
	obs_properties_add_bool(props, "follow_mode", "跟随模式");
	obs_properties_add_text(props, "info", "YOLO 识别插件 - 开发中", OBS_TEXT_INFO);
	
	return props;
}

static void my_filter_update(void *data, obs_data_t *settings)
{
	struct my_filter_data *filter = (struct my_filter_data *)data;
	
	blog(LOG_INFO, "[YOLO] my_filter_update 被调用！");

	const char *modelPath = obs_data_get_string(settings, "model_path");
	filter->modelPath = modelPath ? modelPath : "";
	filter->enableInference = obs_data_get_bool(settings, "enable_inference");
	filter->confidenceThreshold = (float)obs_data_get_double(settings, "confidence_threshold");
	filter->inferenceInterval = (int)obs_data_get_int(settings, "inference_interval");
	filter->modelInputSize = (int)obs_data_get_int(settings, "model_input_size");
	filter->renderBoxes = obs_data_get_bool(settings, "render_boxes");
	filter->followMode = obs_data_get_bool(settings, "follow_mode");

	blog(LOG_INFO, "[YOLO] 模型路径: %s", filter->modelPath.c_str());
	blog(LOG_INFO, "[YOLO] 启用推理: %s", filter->enableInference ? "是" : "否");
}

static void my_filter_video_tick(void *data, float seconds)
{
	UNUSED_PARAMETER(seconds);
	struct my_filter_data *filter = (struct my_filter_data *)data;
	
	blog(LOG_INFO, "[YOLO] video_tick 被调用！");
}

static void my_filter_video_render(void *data, gs_effect_t *effect)
{
	struct my_filter_data *filter = (struct my_filter_data *)data;
	obs_source_t *target = obs_filter_get_target(filter->source);
	uint32_t width = obs_source_get_base_width(target);
	uint32_t height = obs_source_get_base_height(target);

	if (width > 0 && height > 0 && filter->texrender) {
		obs_enter_graphics();
		if (gs_texrender_begin(filter->texrender, width, height)) {
			gs_ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f, 100.0f);
			gs_clear(GS_CLEAR_COLOR, 0, 0.0f, 0);
			obs_source_video_render(target);
			gs_texrender_end(filter->texrender);
		}
		obs_leave_graphics();
	}

	obs_source_skip_video_filter(filter->source);
}

extern "C" struct obs_source_info yolo_filter_info = {
	.id             = "yolo_recognizer_filter",
	.type           = OBS_SOURCE_TYPE_FILTER,
	.output_flags   = OBS_SOURCE_VIDEO,
	.get_name       = my_filter_name,
	.create         = my_filter_create,
	.destroy        = my_filter_destroy,
	.get_defaults   = my_filter_defaults,
	.get_properties = my_filter_properties,
	.update         = my_filter_update,
	.video_tick     = my_filter_video_tick,
	.video_render   = my_filter_video_render,
};
