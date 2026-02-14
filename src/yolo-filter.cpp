#include <obs-module.h>
#include <util/base.h>
#include <onnxruntime_cxx_api.h>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <mutex>
#include <memory>

struct Detection {
	float x, y, w, h;
	float confidence;
	int class_id;
};

struct YoloFilterData {
	obs_source_t *context;

	std::mutex inference_mutex;
	std::unique_ptr<Ort::Env> ort_env;
	std::unique_ptr<Ort::Session> ort_session;
	std::unique_ptr<Ort::MemoryInfo> memory_info;

	bool model_loaded;
	std::string model_path;

	bool inference_enabled;
	float confidence_threshold;
	int inference_interval;
	int frame_counter;
	int target_class_id;
	bool render_boxes;
	bool follow_mode;
	int model_input_size;

	std::vector<Detection> detections;
};

static const char *coco_classes[] = {
	"person", "bicycle", "car", "motorcycle", "airplane", "bus", "train",
	"truck", "boat", "traffic light", "fire hydrant", "stop sign",
	"parking meter", "bench", "bird", "cat", "dog", "horse", "sheep",
	"cow", "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella",
	"handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball",
	"kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
	"tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon",
	"bowl", "banana", "apple", "sandwich", "orange", "broccoli", "carrot",
	"hot dog", "pizza", "donut", "cake", "chair", "couch", "potted plant",
	"bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote",
	"keyboard", "cell phone", "microwave", "oven", "toaster", "sink",
	"refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
	"hair drier", "toothbrush"
};

static const int num_coco_classes = 80;

static void *yolo_filter_create(obs_data_t *settings, obs_source_t *source)
{
	YoloFilterData *filter = new YoloFilterData();
	filter->context = source;
	filter->model_loaded = false;
	filter->inference_enabled = false;
	filter->confidence_threshold = 0.5f;
	filter->inference_interval = 5;
	filter->frame_counter = 0;
	filter->target_class_id = 0;
	filter->render_boxes = true;
	filter->follow_mode = false;
	filter->model_input_size = 640;

	try {
		filter->ort_env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "YoloFilter");
		filter->memory_info = std::make_unique<Ort::MemoryInfo>(
			Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));
	} catch (const std::exception &e) {
		blog(LOG_ERROR, "[YOLO] Failed to initialize ONNX Runtime: %s", e.what());
	}

	obs_source_update(source, settings);

	return filter;
}

static void yolo_filter_destroy(void *data)
{
	YoloFilterData *filter = static_cast<YoloFilterData *>(data);

	std::lock_guard<std::mutex> lock(filter->inference_mutex);

	filter->ort_session.reset();
	filter->memory_info.reset();
	filter->ort_env.reset();

	delete filter;
}

static bool load_model(YoloFilterData *filter, const std::string &path)
{
	std::lock_guard<std::mutex> lock(filter->inference_mutex);

	filter->ort_session.reset();

	if (path.empty()) {
		filter->model_loaded = false;
		return false;
	}

	try {
		Ort::SessionOptions session_options;
		session_options.SetIntraOpNumThreads(1);
		session_options.SetGraphOptimizationLevel(
			GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

#ifdef _WIN32
		std::wstring wpath(path.begin(), path.end());
		filter->ort_session = std::make_unique<Ort::Session>(
			*filter->ort_env, wpath.c_str(), session_options);
#else
		filter->ort_session = std::make_unique<Ort::Session>(
			*filter->ort_env, path.c_str(), session_options);
#endif

		filter->model_path = path;
		filter->model_loaded = true;
		blog(LOG_INFO, "[YOLO] Model loaded successfully: %s", path.c_str());
		return true;
	} catch (const std::exception &e) {
		blog(LOG_ERROR, "[YOLO] Failed to load model: %s", e.what());
		filter->model_loaded = false;
		return false;
	}
}

static void yolo_filter_update(void *data, obs_data_t *settings)
{
	YoloFilterData *filter = static_cast<YoloFilterData *>(data);

	const char *model_path = obs_data_get_string(settings, "model_path");
	if (model_path && model_path[0]) {
		if (filter->model_path != model_path) {
			load_model(filter, model_path);
		}
	}

	filter->inference_enabled = obs_data_get_bool(settings, "inference_enabled");
	filter->confidence_threshold = (float)obs_data_get_double(settings, "confidence_threshold");
	filter->inference_interval = (int)obs_data_get_int(settings, "inference_interval");
	filter->target_class_id = (int)obs_data_get_int(settings, "target_class");
	filter->render_boxes = obs_data_get_bool(settings, "render_boxes");
	filter->follow_mode = obs_data_get_bool(settings, "follow_mode");
	filter->model_input_size = (int)obs_data_get_int(settings, "model_input_size");
}

static void preprocess_image(const uint8_t *bgra_data, int width, int height,
			     int input_size, std::vector<float> &output)
{
	std::vector<uint8_t> rgb_data(width * height * 3);

	for (int i = 0; i < width * height; i++) {
		int idx = i * 4;
		rgb_data[i * 3 + 0] = bgra_data[idx + 2];
		rgb_data[i * 3 + 1] = bgra_data[idx + 1];
		rgb_data[i * 3 + 2] = bgra_data[idx + 0];
	}

	float scale = std::min((float)input_size / width, (float)input_size / height);
	int new_w = (int)(width * scale);
	int new_h = (int)(height * scale);

	std::vector<uint8_t> resized(input_size * input_size * 3, 114);
	float x_ratio = (float)width / new_w;
	float y_ratio = (float)height / new_h;

	int pad_left = (input_size - new_w) / 2;
	int pad_top = (input_size - new_h) / 2;

	for (int y = 0; y < new_h; y++) {
		for (int x = 0; x < new_w; x++) {
			int src_x = (int)(x * x_ratio);
			int src_y = (int)(y * y_ratio);
			src_x = std::min(std::max(src_x, 0), width - 1);
			src_y = std::min(std::max(src_y, 0), height - 1);

			int src_idx = (src_y * width + src_x) * 3;
			int dst_idx = ((pad_top + y) * input_size + (pad_left + x)) * 3;

			resized[dst_idx + 0] = rgb_data[src_idx + 0];
			resized[dst_idx + 1] = rgb_data[src_idx + 1];
			resized[dst_idx + 2] = rgb_data[src_idx + 2];
		}
	}

	output.resize(1 * 3 * input_size * input_size);
	for (int c = 0; c < 3; c++) {
		for (int y = 0; y < input_size; y++) {
			for (int x = 0; x < input_size; x++) {
				int src_idx = (y * input_size + x) * 3 + c;
				int dst_idx = c * input_size * input_size + y * input_size + x;
				output[dst_idx] = resized[src_idx] / 255.0f;
			}
		}
	}
}

static float iou(const Detection &a, const Detection &b)
{
	float x1 = std::max(a.x, b.x);
	float y1 = std::max(a.y, b.y);
	float x2 = std::min(a.x + a.w, b.x + b.w);
	float y2 = std::min(a.y + a.h, b.y + b.h);

	float w = std::max(0.0f, x2 - x1);
	float h = std::max(0.0f, y2 - y1);
	float inter = w * h;

	float area_a = a.w * a.h;
	float area_b = b.w * b.h;

	return inter / (area_a + area_b - inter);
}

static void nms(std::vector<Detection> &detections, float iou_threshold)
{
	std::sort(detections.begin(), detections.end(),
		  [](const Detection &a, const Detection &b) {
			  return a.confidence > b.confidence;
		  });

	std::vector<Detection> keep;
	std::vector<bool> suppressed(detections.size(), false);

	for (size_t i = 0; i < detections.size(); i++) {
		if (suppressed[i])
			continue;

		keep.push_back(detections[i]);

		for (size_t j = i + 1; j < detections.size(); j++) {
			if (suppressed[j])
				continue;
			if (detections[i].class_id != detections[j].class_id)
				continue;
			if (iou(detections[i], detections[j]) > iou_threshold) {
				suppressed[j] = true;
			}
		}
	}

	detections = std::move(keep);
}

static void postprocess_yolov8(const float *output_data, int batch_size, int num_classes,
				int num_detections, int input_size, int original_width,
				int original_height, float confidence_threshold,
				std::vector<Detection> &detections)
{
	detections.clear();

	float scale = std::min((float)input_size / original_width,
			       (float)input_size / original_height);
	int pad_left = (input_size - (int)(original_width * scale)) / 2;
	int pad_top = (input_size - (int)(original_height * scale)) / 2;

	for (int i = 0; i < num_detections; i++) {
		float max_conf = 0.0f;
		int class_id = 0;

		for (int c = 0; c < num_classes; c++) {
			float conf = output_data[(4 + c) * num_detections + i];
			if (conf > max_conf) {
				max_conf = conf;
				class_id = c;
			}
		}

		if (max_conf < confidence_threshold)
			continue;

		float cx = output_data[0 * num_detections + i];
		float cy = output_data[1 * num_detections + i];
		float w = output_data[2 * num_detections + i];
		float h = output_data[3 * num_detections + i];

		float x1 = cx - w / 2 - pad_left;
		float y1 = cy - h / 2 - pad_top;

		x1 /= scale;
		y1 /= scale;
		w /= scale;
		h /= scale;

		Detection det;
		det.x = x1;
		det.y = y1;
		det.w = w;
		det.h = h;
		det.confidence = max_conf;
		det.class_id = class_id;
		detections.push_back(det);
	}

	nms(detections, 0.45f);
}

static void run_inference(YoloFilterData *filter, const uint8_t *data, int width, int height)
{
	if (!filter->model_loaded || !filter->ort_session)
		return;

	std::vector<float> input_tensor;
	preprocess_image(data, width, height, filter->model_input_size, input_tensor);

	std::vector<int64_t> input_shape = {1, 3, (int64_t)filter->model_input_size,
					      (int64_t)filter->model_input_size};

	try {
		Ort::Value input_tensor_ort = Ort::Value::CreateTensor<float>(
			*filter->memory_info, input_tensor.data(), input_tensor.size(),
			input_shape.data(), input_shape.size());

		Ort::AllocatorWithDefaultOptions allocator;
		auto input_name = filter->ort_session->GetInputNameAllocated(0, allocator);
		auto output_name = filter->ort_session->GetOutputNameAllocated(0, allocator);

		const char *input_names[] = {input_name.get()};
		const char *output_names[] = {output_name.get()};

		auto output_tensors = filter->ort_session->Run(
			Ort::RunOptions{nullptr}, input_names, &input_tensor_ort, 1,
			output_names, 1);

		auto &output_tensor = output_tensors[0];
		auto output_info = output_tensor.GetTensorTypeAndShapeInfo();
		auto output_shape = output_info.GetShape();
		float *output_data = output_tensor.GetTensorMutableData<float>();

		int num_detections = 0;
		int num_classes = num_coco_classes;

		if (output_shape.size() == 3) {
			num_classes = (int)output_shape[1] - 4;
			num_detections = (int)output_shape[2];
		} else if (output_shape.size() == 2) {
			num_detections = (int)output_shape[0];
			num_classes = (int)output_shape[1] - 5;
		}

		std::vector<Detection> detections;
		postprocess_yolov8(output_data, 1, num_classes, num_detections,
				    filter->model_input_size, width, height,
				    filter->confidence_threshold, detections);

		std::lock_guard<std::mutex> lock(filter->inference_mutex);
		filter->detections = std::move(detections);
	} catch (const std::exception &e) {
		blog(LOG_ERROR, "[YOLO] Inference failed: %s", e.what());
	}
}

static void yolo_filter_video_render(void *data, gs_effect_t *effect)
{
	YoloFilterData *filter = static_cast<YoloFilterData *>(data);

	obs_source_process_filter_begin(filter->context, GS_RGBA, OBS_ALLOW_DIRECT_RENDERING);
	obs_source_process_filter_end(filter->context, effect, 0, 0);
}

static void yolo_filter_video_tick(void *data, float seconds)
{
	YoloFilterData *filter = static_cast<YoloFilterData *>(data);
	(void)seconds;

	if (!filter->inference_enabled)
		return;

	filter->frame_counter++;
	if (filter->frame_counter < filter->inference_interval)
		return;

	filter->frame_counter = 0;

	struct obs_source_frame *frame = obs_source_get_frame(filter->context);
	if (!frame)
		return;

	if (frame->format != VIDEO_FORMAT_BGRA && frame->format != VIDEO_FORMAT_RGBA) {
		obs_source_release_frame(filter->context, frame);
		return;
	}

	const uint8_t *data_ptr = frame->data[0];
	int width = (int)frame->width;
	int height = (int)frame->height;

	run_inference(filter, data_ptr, width, height);

	obs_source_release_frame(filter->context, frame);
}

static obs_properties_t *yolo_filter_properties(void *data)
{
	YoloFilterData *filter = static_cast<YoloFilterData *>(data);
	(void)filter;

	obs_properties_t *props = obs_properties_create();

	obs_properties_add_path(props, "model_path", obs_module_text("ModelPath"),
				OBS_PATH_FILE, "ONNX Files (*.onnx)", nullptr);

	obs_properties_add_bool(props, "inference_enabled", obs_module_text("EnableInference"));

	obs_properties_add_float_slider(props, "confidence_threshold",
					 obs_module_text("ConfidenceThreshold"), 0.0, 1.0, 0.01);

	obs_properties_add_int_slider(props, "inference_interval",
				       obs_module_text("InferenceInterval"), 1, 30, 1);

	obs_property_t *class_list = obs_properties_add_list(
		props, "target_class", obs_module_text("TargetClass"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	for (int i = 0; i < num_coco_classes; i++) {
		obs_property_list_add_int(class_list, coco_classes[i], i);
	}

	obs_properties_add_bool(props, "render_boxes", obs_module_text("RenderBoxes"));
	obs_properties_add_bool(props, "follow_mode", obs_module_text("FollowMode"));

	obs_property_t *size_list = obs_properties_add_list(
		props, "model_input_size", obs_module_text("ModelInputSize"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(size_list, "320x320", 320);
	obs_property_list_add_int(size_list, "416x416", 416);
	obs_property_list_add_int(size_list, "512x512", 512);
	obs_property_list_add_int(size_list, "640x640", 640);

	return props;
}

static void yolo_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, "model_path", "");
	obs_data_set_default_bool(settings, "inference_enabled", false);
	obs_data_set_default_double(settings, "confidence_threshold", 0.5);
	obs_data_set_default_int(settings, "inference_interval", 5);
	obs_data_set_default_int(settings, "target_class", 0);
	obs_data_set_default_bool(settings, "render_boxes", true);
	obs_data_set_default_bool(settings, "follow_mode", false);
	obs_data_set_default_int(settings, "model_input_size", 640);
}

static const char *yolo_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("YoloRecognizer");
}

struct obs_source_info yolo_filter_info = {
	"yolo_recognizer_filter",
	OBS_SOURCE_TYPE_FILTER,
	OBS_SOURCE_VIDEO,
	yolo_filter_name,
	yolo_filter_create,
	yolo_filter_destroy,
	yolo_filter_update,
	yolo_filter_defaults,
	yolo_filter_properties,
	NULL,
	NULL,
	yolo_filter_video_tick,
	yolo_filter_video_render,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	OBS_ICON_TYPE_UNKNOWN
};
