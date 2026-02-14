/**
 * YOLO 识别滤镜 - 基础透传版本
 * 先确保渲染链路 100% 通畅，不黑屏
 */

#include <obs-module.h>
#include <util/base.h>

/**
 * 滤镜数据结构体
 * 暂时只保留最基础的字段
 */
struct YoloFilterData {
	obs_source_t *context;  // 源上下文，必须要有
};

/**
 * 获取滤镜名称
 */
static const char *yolo_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("YoloRecognizer");
}

/**
 * 创建滤镜实例
 * 这是滤镜生命周期的开始
 */
static void *yolo_filter_create(obs_data_t *settings, obs_source_t *source)
{
	// 分配数据结构
	YoloFilterData *filter = new YoloFilterData();
	
	// 保存源上下文 - 这很重要！
	filter->context = source;
	
	// 记录日志，方便调试
	blog(LOG_INFO, "[YOLO] 滤镜创建成功！");
	
	// 应用默认设置
	obs_source_update(source, settings);
	
	return filter;
}

/**
 * 销毁滤镜实例
 * 清理资源
 */
static void yolo_filter_destroy(void *data)
{
	if (!data)
		return;
	
	YoloFilterData *filter = static_cast<YoloFilterData *>(data);
	
	// 记录日志
	blog(LOG_INFO, "[YOLO] 滤镜销毁");
	
	// 释放内存
	delete filter;
}

/**
 * 设置默认值
 */
static void yolo_filter_defaults(obs_data_t *settings)
{
	// 暂时不需要任何设置
	UNUSED_PARAMETER(settings);
}

/**
 * 创建属性面板
 */
static obs_properties_t *yolo_filter_properties(void *data)
{
	UNUSED_PARAMETER(data);
	
	// 创建属性面板
	obs_properties_t *props = obs_properties_create();
	
	// 暂时只加一个提示信息
	obs_properties_add_text(props, "info", "透传版本（视频正常显示）", OBS_TEXT_INFO);
	
	return props;
}

/**
 * 更新设置
 */
static void yolo_filter_update(void *data, obs_data_t *settings)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(settings);
	
	// 暂时不需要处理设置
}

/**
 * ============================================
 * 最关键的函数：视频渲染
 * ============================================
 * 这是黑屏问题的核心！
 */
static void yolo_filter_video_render(void *data, gs_effect_t *effect)
{
	// 1. 检查参数有效性
	if (!data || !effect) {
		blog(LOG_WARNING, "[YOLO] 渲染参数无效");
		return;
	}
	
	YoloFilterData *filter = static_cast<YoloFilterData *>(data);
	
	if (!filter->context) {
		blog(LOG_WARNING, "[YOLO] 源上下文无效");
		return;
	}
	
	// 2. 开始处理滤镜 - 这是必须的！
	// 参数说明：
	//   - filter->context: 源上下文
	//   - GS_RGBA: 颜色格式
	//   - OBS_NO_DIRECT_RENDERING: 不使用直接渲染（最安全）
	obs_source_process_filter_begin(filter->context, GS_RGBA, OBS_NO_DIRECT_RENDERING);
	
	// 3. 结束处理滤镜 - 这也是必须的！
	// 这会把上游的视频帧绘制出来
	obs_source_process_filter_end(filter->context, effect, 0, 0);
	
	// 注意：
	// begin 和 end 必须成对出现！
	// 这两个调用是 OBS 滤镜透传的核心！
}

/**
 * ============================================
 * 注册滤镜信息
 * ============================================
 */
extern "C" {
struct obs_source_info yolo_filter_info = {
	// 滤镜的唯一 ID
	.id = "yolo_recognizer_filter",
	
	// 类型：视频滤镜
	.type = OBS_SOURCE_TYPE_FILTER,
	
	// 输出标志：视频源
	.output_flags = OBS_SOURCE_VIDEO,
	
	// 各个回调函数
	.get_name = yolo_filter_name,
	.create = yolo_filter_create,
	.destroy = yolo_filter_destroy,
	.get_defaults = yolo_filter_defaults,
	.get_properties = yolo_filter_properties,
	.update = yolo_filter_update,
	.video_render = yolo_filter_video_render,  // 最重要的渲染函数！
	
	// 图标类型
	.icon_type = OBS_ICON_TYPE_UNKNOWN
};
}
