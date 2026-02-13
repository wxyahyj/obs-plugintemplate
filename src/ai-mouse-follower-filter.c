#include <obs-module.h>
#include <plugin-support.h>
#include <util/threading.h>
#ifdef _WIN32
#include <Windows.h>
#endif

// 滤镜设置结构体
typedef struct {
    bool enable_mouse_follow_test;
    uint64_t last_move_time;
    const char *filter_name;
} AIFilterData;

// get_name 回调（必须返回 const char*）
static const char *ai_mouse_follower_get_name(void *unused)
{
    UNUSED_PARAMETER(unused);
    return "AI Mouse Follower Filter";
}

// 可选：get_description（如果想加描述）
static const char *ai_mouse_follower_get_description(void *unused)
{
    UNUSED_PARAMETER(unused);
    return "Test filter that moves mouse cursor";
}

// 初始化滤镜
static void *create(obs_data_t *settings, obs_source_t *source)
{
    AIFilterData *data = bzalloc(sizeof(AIFilterData));
    data->enable_mouse_follow_test = false;
    data->last_move_time = 0;
    data->filter_name = obs_source_get_name(source);
    return data;
}

// 销毁滤镜
static void destroy(void *data)
{
    if (data) {
        bfree(data);
    }
}

// 从设置中更新滤镜数据
static void update(void *data, obs_data_t *settings)
{
    AIFilterData *filter_data = (AIFilterData *)data;
    filter_data->enable_mouse_follow_test = obs_data_get_bool(settings, "enable_mouse_follow_test");
}

// 获取滤镜属性（UI 设置项）
static obs_properties_t *get_properties(void *data)
{
    obs_properties_t *props = obs_properties_create();
    
    // 添加"Enable Mouse Follow Test"复选框
    obs_properties_add_bool(props, "enable_mouse_follow_test", "Enable Mouse Follow Test");
    
    return props;
}

// 获取默认设置
static void get_defaults(obs_data_t *settings)
{
    obs_data_set_default_bool(settings, "enable_mouse_follow_test", false);
}

// 处理视频帧
static struct obs_source_frame *video_render(void *data, struct obs_source_frame *frame)
{
    AIFilterData *filter_data = (AIFilterData *)data;
    
    // 检查是否启用鼠标跟随测试
    if (filter_data->enable_mouse_follow_test) {
        uint64_t current_time = os_gettime_ns();
        
        // 每 500 毫秒移动一次鼠标
        if (current_time - filter_data->last_move_time >= 500000000ULL) { // 500ms = 500,000,000ns
#ifdef _WIN32
            // 移动鼠标到屏幕中心位置 (960, 540)，假设 1920x1080 分辨率
            SetCursorPos(960, 540);
            blog(LOG_INFO, "[AI Mouse Follower] Moved cursor to (960,540)");
#else
            blog(LOG_WARNING, "[AI Mouse Follower] Mouse follow test is only supported on Windows");
#endif
            filter_data->last_move_time = current_time;
        }
    }
    
    // 返回原始帧，不做修改
    return frame;
}

// 注册滤镜源
void register_ai_mouse_follower_filter(void)
{
    struct obs_source_info info = {};
    info.id             = "ai_mouse_follower_filter";  // 唯一 ID
    info.type           = OBS_SOURCE_TYPE_FILTER;       // 滤镜类型
    info.output_flags   = OBS_SOURCE_VIDEO;             // 视频滤镜
    info.get_name       = ai_mouse_follower_get_name;   // 用函数指针
    info.get_description = ai_mouse_follower_get_description;  // 可选

    info.create         = create;                       // 你的 create 函数
    info.destroy        = destroy;
    info.update         = update;
    info.get_properties = get_properties;
    info.get_defaults   = get_defaults;
    info.video_render   = video_render;  // 处理视频帧

    obs_register_source(&info);
}
