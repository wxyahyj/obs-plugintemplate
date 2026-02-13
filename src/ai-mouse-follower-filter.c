#include <obs-module.h>
#include <plugin-support.h>
#ifdef _WIN32
#include <Windows.h>
#endif

#define FILTER_NAME "AI Mouse Follower Filter"

// 滤镜设置结构体
typedef struct {
    bool enable_mouse_follow_test;
    uint64_t last_move_time;
    const char *filter_name;
} AIFilterData;

// 初始化滤镜
static void *filter_create(obs_data_t *settings, obs_source_t *source)
{
    AIFilterData *data = bzalloc(sizeof(AIFilterData));
    data->enable_mouse_follow_test = false;
    data->last_move_time = 0;
    data->filter_name = obs_source_get_name(source);
    return data;
}

// 销毁滤镜
static void filter_destroy(void *data)
{
    if (data) {
        bfree(data);
    }
}

// 从设置中更新滤镜数据
static void filter_update(void *data, obs_data_t *settings)
{
    AIFilterData *filter_data = (AIFilterData *)data;
    filter_data->enable_mouse_follow_test = obs_data_get_bool(settings, "enable_mouse_follow_test");
}

// 获取滤镜属性（UI 设置项）
static obs_properties_t *filter_properties(void *data)
{
    obs_properties_t *props = obs_properties_create();
    
    // 添加"Enable Mouse Follow Test"复选框
    obs_properties_add_bool(props, "enable_mouse_follow_test", "Enable Mouse Follow Test");
    
    return props;
}

// 获取默认设置
static void filter_defaults(obs_data_t *settings)
{
    obs_data_set_default_bool(settings, "enable_mouse_follow_test", false);
}

// 处理视频帧
static struct obs_source_frame *filter_video_render(void *data, struct obs_source_frame *frame)
{
    AIFilterData *filter_data = (AIFilterData *)data;
    
    // 检查是否启用鼠标跟随测试
    if (filter_data->enable_mouse_follow_test) {
        uint64_t current_time = os_gettime_ns();
        
        // 每 500 毫秒移动一次鼠标
        if (current_time - filter_data->last_move_time >= 500000000) { // 500ms = 500,000,000ns
#ifdef _WIN32
            // 移动鼠标到屏幕中心位置 (960, 540)，假设 1920x1080 分辨率
            SetCursorPos(960, 540);
            obs_log(LOG_INFO, "[AI Mouse Follower] Moved cursor to (960,540)");
#else
            obs_log(LOG_WARNING, "[AI Mouse Follower] Mouse follow test is only supported on Windows");
#endif
            filter_data->last_move_time = current_time;
        }
    }
    
    // 返回原始帧，不做修改
    return frame;
}

// 定义滤镜源信息
static struct obs_source_info filter_info = {
    .id = "ai_mouse_follower_filter",
    .type = OBS_SOURCE_TYPE_FILTER,
    .output_flags = OBS_SOURCE_OUTPUT_VIDEO,
    .create = filter_create,
    .destroy = filter_destroy,
    .update = filter_update,
    .get_properties = filter_properties,
    .get_defaults = filter_defaults,
    .video_render = filter_video_render,
    .name = FILTER_NAME,
    .description = "AI-powered mouse follower filter"
};

// 注册滤镜源
void register_ai_mouse_follower_filter(void)
{
    obs_register_source(&filter_info);
}
