#include <rtthread.h>
#include <rtdevice.h>
#include "screen.h"
#include <string.h>
#include <stdlib.h>
#include "data_manager.h"
#include <time.h>
#include <stdio.h>
#include "hid_device.h"
#include "event_bus.h"

#define SERIAL_RX_BUFFER_SIZE 1024
#define SERIAL_DEVICE_NAME "uart1"
#define SERIAL_TIMEOUT_MS (30000)
#define WATCHDOG_CHECK_INTERVAL_MS (10000)

static rt_device_t serial_device = RT_NULL;
static rt_sem_t rx_sem = RT_NULL;
static rt_timer_t watchdog_timer = RT_NULL;

static struct {
    rt_tick_t last_received_tick;
    bool connection_alive;
    uint32_t total_commands_received;
    uint32_t invalid_commands_count;
    uint32_t timeout_count;
} g_serial_status = {0};

static const char* weather_code_map[] = {
    [100] = "晴",
    [101] = "多云",
    [102] = "少云",
    [103] = "晴间多云",
    [104] = "阴",
    [150] = "晴",
    [151] = "多云",
    [152] = "少云",
    [153] = "晴间多云",
    [300] = "阵雨",
    [301] = "强阵雨",
    [302] = "雷阵雨",
    [303] = "强雷阵雨",
    [304] = "雷阵雨伴有冰雹",
    [305] = "小雨",
    [306] = "中雨",
    [307] = "大雨",
    [308] = "极端降雨",
    [309] = "毛毛雨/细雨",
    [310] = "暴雨",
    [311] = "大暴雨",
    [312] = "特大暴雨",
    [313] = "冻雨",
    [314] = "小到中雨",
    [315] = "中到大雨",
    [316] = "大到暴雨",
    [317] = "暴雨到大暴雨",
    [318] = "大暴雨到特大暴雨",
    [350] = "阵雨",
    [351] = "强阵雨",
    [399] = "雨",
    [400] = "小雪",
    [401] = "中雪",
    [402] = "大雪",
    [403] = "暴雪",
    [404] = "雨夹雪",
    [405] = "雨雪天气",
    [406] = "阵雨夹雪",
    [407] = "阵雪",
    [408] = "小到中雪",
    [409] = "中到大雪",
    [410] = "大到暴雪",
    [456] = "阵雨夹雪",
    [457] = "阵雪",
    [499] = "雪",
    [500] = "薄雾",
    [501] = "雾",
    [502] = "霾",
    [503] = "扬沙",
    [504] = "浮尘",
    [507] = "沙尘暴",
    [508] = "强沙尘暴",
    [509] = "浓雾",
    [510] = "强浓雾",
    [511] = "中度霾",
    [512] = "重度霾",
    [513] = "严重霾",
    [514] = "大雾",
    [515] = "特强浓雾",
    [900] = "热",
    [901] = "冷",
    [999] = "未知"
};
#define WEATHER_CODE_MAX 1000

static const char* city_code_map[] = {
    [0] = "杭州",
    [1] = "上海",
    [2] = "北京",
    [3] = "广州",
    [4] = "深圳",
    [5] = "成都",
    [6] = "重庆",
    [7] = "武汉",
    [8] = "西安",
    [9] = "南京",
    [10] = "天津",
    [11] = "苏州",
    [12] = "青岛",
    [13] = "厦门",
    [14] = "长沙",
    [15] = "石家庄",
    [16] = "唐山",
    [17] = "秦皇岛",
    [18] = "邯郸",
    [19] = "邢台",
    [20] = "保定",
    [21] = "张家口",
    [22] = "承德",
    [23] = "沧州",
    [24] = "廊坊",
    [25] = "衡水",
    [26] = "太原",
    [27] = "大同",
    [28] = "阳泉",
    [29] = "长治",
    [30] = "晋城",
    [31] = "朔州",
    [32] = "晋中",
    [33] = "运城",
    [34] = "忻州",
    [35] = "临汾",
    [36] = "吕梁",
    [37] = "呼和浩特",
    [38] = "包头",
    [39] = "乌海",
    [40] = "赤峰",
    [41] = "通辽",
    [42] = "鄂尔多斯",
    [43] = "呼伦贝尔",
    [44] = "巴彦淖尔",
    [45] = "乌兰察布",
    [46] = "沈阳",
    [47] = "大连",
    [48] = "鞍山",
    [49] = "抚顺",
    [50] = "本溪",
    [51] = "丹东",
    [52] = "锦州",
    [53] = "营口",
    [54] = "阜新",
    [55] = "辽阳",
    [56] = "盘锦",
    [57] = "铁岭",
    [58] = "朝阳",
    [59] = "葫芦岛",
    [60] = "长春",
    [61] = "吉林",
    [62] = "四平",
    [63] = "辽源",
    [64] = "通化",
    [65] = "白山",
    [66] = "松原",
    [67] = "白城",
    [68] = "哈尔滨",
    [69] = "齐齐哈尔",
    [70] = "鸡西",
    [71] = "鹤岗",
    [72] = "双鸭山",
    [73] = "大庆",
    [74] = "伊春",
    [75] = "佳木斯",
    [76] = "七台河",
    [77] = "牡丹江",
    [78] = "黑河",
    [79] = "绥化",
    [80] = "无锡",
    [81] = "徐州",
    [82] = "南通",
    [83] = "连云港",
    [84] = "扬州",
    [85] = "盐城",
    [86] = "淮安",
    [87] = "常州",
    [88] = "镇江",
    [89] = "泰州",
    [90] = "宿迁",
    [91] = "宁波",
    [92] = "温州",
    [93] = "嘉兴",
    [94] = "湖州",
    [95] = "绍兴",
    [96] = "金华",
    [97] = "衢州",
    [98] = "舟山",
    [99] = "台州",
    [100] = "丽水",
    [101] = "合肥",
    [102] = "芜湖",
    [103] = "蚌埠",
    [104] = "淮南",
    [105] = "马鞍山",
    [106] = "淮北",
    [107] = "铜陵",
    [108] = "安庆",
    [109] = "黄山",
    [110] = "滁州",
    [111] = "阜阳",
    [112] = "宿州",
    [113] = "六安",
    [114] = "亳州",
    [115] = "池州",
    [116] = "宣城",
    [117] = "福州",
    [118] = "莆田",
    [119] = "三明",
    [120] = "泉州",
    [121] = "漳州",
    [122] = "南平",
    [123] = "龙岩",
    [124] = "宁德",
    [125] = "南昌",
    [126] = "景德镇",
    [127] = "萍乡",
    [128] = "九江",
    [129] = "新余",
    [130] = "鹰潭",
    [131] = "赣州",
    [132] = "吉安",
    [133] = "宜春",
    [134] = "抚州",
    [135] = "上饶",
    [136] = "济南",
    [137] = "淄博",
    [138] = "枣庄",
    [139] = "东营",
    [140] = "烟台",
    [141] = "潍坊",
    [142] = "济宁",
    [143] = "泰安",
    [144] = "威海",
    [145] = "日照",
    [146] = "莱芜",
    [147] = "临沂",
    [148] = "德州",
    [149] = "聊城",
    [150] = "滨州",
    [151] = "菏泽",
    [152] = "郑州",
    [153] = "开封",
    [154] = "洛阳",
    [155] = "平顶山",
    [156] = "安阳",
    [157] = "鹤壁",
    [158] = "新乡",
    [159] = "焦作",
    [160] = "濮阳",
    [161] = "许昌",
    [162] = "漯河",
    [163] = "三门峡",
    [164] = "南阳",
    [165] = "商丘",
    [166] = "信阳",
    [167] = "周口",
    [168] = "驻马店",
    [169] = "黄石",
    [170] = "十堰",
    [171] = "宜昌",
    [172] = "襄阳",
    [173] = "鄂州",
    [174] = "荆门",
    [175] = "孝感",
    [176] = "荆州",
    [177] = "黄冈",
    [178] = "咸宁",
    [179] = "随州",
    [180] = "株洲",
    [181] = "湘潭",
    [182] = "衡阳",
    [183] = "邵阳",
    [184] = "岳阳",
    [185] = "常德",
    [186] = "张家界",
    [187] = "益阳",
    [188] = "郴州",
    [189] = "永州",
    [190] = "怀化",
    [191] = "娄底",
    [192] = "韶关",
    [193] = "汕头",
    [194] = "佛山",
    [195] = "江门",
    [196] = "湛江",
    [197] = "茂名",
    [198] = "肇庆",
    [199] = "惠州",
    [200] = "梅州",
    [201] = "汕尾",
    [202] = "河源",
    [203] = "阳江",
    [204] = "清远",
    [205] = "东莞",
    [206] = "中山",
    [207] = "潮州",
    [208] = "揭阳",
    [209] = "云浮",
    [210] = "南宁",
    [211] = "柳州",
    [212] = "桂林",
    [213] = "梧州",
    [214] = "北海",
    [215] = "防城港",
    [216] = "钦州",
    [217] = "贵港",
    [218] = "玉林",
    [219] = "百色",
    [220] = "贺州",
    [221] = "河池",
    [222] = "来宾",
    [223] = "崇左",
    [224] = "海口",
    [225] = "三亚",
    [226] = "三沙",
    [227] = "儋州",
    [228] = "自贡",
    [229] = "攀枝花",
    [230] = "泸州",
    [231] = "德阳",
    [232] = "绵阳",
    [233] = "广元",
    [234] = "遂宁",
    [235] = "内江",
    [236] = "乐山",
    [237] = "南充",
    [238] = "眉山",
    [239] = "宜宾",
    [240] = "广安",
    [241] = "达州",
    [242] = "雅安",
    [243] = "巴中",
    [244] = "资阳",
    [245] = "贵阳",
    [246] = "六盘水",
    [247] = "遵义",
    [248] = "安顺",
    [249] = "毕节",
    [250] = "铜仁",
    [251] = "昆明",
    [252] = "曲靖",
    [253] = "玉溪",
    [254] = "保山",
    [255] = "昭通",
    [256] = "丽江",
    [257] = "普洱",
    [258] = "临沧",
    [259] = "拉萨",
    [260] = "昌都",
    [261] = "山南",
    [262] = "日喀则",
    [263] = "那曲",
    [264] = "阿里",
    [265] = "林芝",
    [266] = "铜川",
    [267] = "宝鸡",
    [268] = "咸阳",
    [269] = "渭南",
    [270] = "延安",
    [271] = "汉中",
    [272] = "榆林",
    [273] = "安康",
    [274] = "商洛",
    [275] = "兰州",
    [276] = "嘉峪关",
    [277] = "金昌",
    [278] = "白银",
    [279] = "天水",
    [280] = "武威",
    [281] = "张掖",
    [282] = "平凉",
    [283] = "酒泉",
    [284] = "庆阳",
    [285] = "定西",
    [286] = "陇南",
    [287] = "西宁",
    [288] = "海东",
    [289] = "银川",
    [290] = "石嘴山",
    [291] = "吴忠",
    [292] = "固原",
    [293] = "中卫",
    [294] = "乌鲁木齐",
    [295] = "克拉玛依",
    [296] = "吐鲁番",
    [297] = "哈密",
    [298] = "昌吉",
    [299] = "博尔塔拉",
    [300] = "巴音郭楞",
    [301] = "阿克苏",
    [302] = "克孜勒苏",
    [303] = "喀什",
    [304] = "和田",
    [305] = "伊犁",
    [306] = "塔城",
    [307] = "阿勒泰",
    [308] = "香港",
    [309] = "澳门",
    [310] = "台北",
    [311] = "高雄",
    [312] = "台中",
    [313] = "台南",
    [999] = "未知"
};
#define CITY_CODE_MAX 314

static struct {
    char time_str[16];
    char date_str[16];
    char weekday_str[16];
    bool time_valid;
    
    int temperature;
    int weather_code;
    int humidity;
    int pressure;
    int city_code;
    bool weather_valid;
    
    char stock_name[64];
    float stock_price;
    float stock_change;
    bool stock_valid;
    
    float cpu_usage;
    float cpu_temp;
    float mem_usage;
    float gpu_usage;
    float gpu_temp;
    float net_up;
    float net_down;
    bool system_valid;
} g_finsh_data = {0};

static void update_connection_status(void)
{
    g_serial_status.last_received_tick = rt_tick_get();
    if (!g_serial_status.connection_alive) {
        g_serial_status.connection_alive = true;
    }
}

static void serial_watchdog_timer_cb(void *parameter)
{
    (void)parameter;
    
    rt_tick_t now = rt_tick_get();
    rt_tick_t timeout_ticks = rt_tick_from_millisecond(SERIAL_TIMEOUT_MS);
    
    if ((now - g_serial_status.last_received_tick) > timeout_ticks) {
        if (g_serial_status.connection_alive) {
            g_serial_status.connection_alive = false;
            g_serial_status.timeout_count++;
            
            g_finsh_data.time_valid = false;
            g_finsh_data.weather_valid = false;
            g_finsh_data.stock_valid = false;
            g_finsh_data.system_valid = false;
            
            data_manager_reset_all_data();
        }
    }
}

static void safe_strcpy(char *dest, const char *src, size_t dest_size)
{
    if (!dest || !src || dest_size == 0) return;
    
    size_t src_len = strlen(src);
    size_t copy_len = (src_len < dest_size - 1) ? src_len : (dest_size - 1);
    
    memcpy(dest, src, copy_len);
    dest[copy_len] = '\0';
}

static void remove_quotes(char *str)
{
    if (!str) return;
    
    int len = strlen(str);
    if (len >= 2 && str[0] == '"' && str[len-1] == '"') {
        str[len-1] = '\0';
        memmove(str, str+1, len-1);
    }
}

static void handle_time_data(const char *key, const char *value)
{
    if (strcmp(key, "time") == 0) {
        safe_strcpy(g_finsh_data.time_str, value, sizeof(g_finsh_data.time_str));
        g_finsh_data.time_valid = true;
        
        int hour, min, sec;
        if (sscanf(value, "%d:%d:%d", &hour, &min, &sec) == 3) {
            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);
            if (tm_info) {
                tm_info->tm_hour = hour;
                tm_info->tm_min = min;
                tm_info->tm_sec = sec;
                time_t new_time = mktime(tm_info);
                
                rt_device_t rtc = rt_device_find("rtc");
                if (rtc) {
                    rt_device_control(rtc, RT_DEVICE_CTRL_RTC_SET_TIME, &new_time);
                }
            }
        }
    }
    else if (strcmp(key, "date") == 0) {
        safe_strcpy(g_finsh_data.date_str, value, sizeof(g_finsh_data.date_str));
        int year, month, day;
        if (sscanf(value, "%d-%d-%d", &year, &month, &day) == 3) {
            time_t now = time(NULL);
            struct tm *tm_info = localtime(&now);
            if (tm_info) {
                tm_info->tm_year = year - 1900;
                tm_info->tm_mon = month - 1;
                tm_info->tm_mday = day;
                time_t new_time = mktime(tm_info);
                
                rt_device_t rtc = rt_device_find("rtc");
                if (rtc) {
                    rt_device_control(rtc, RT_DEVICE_CTRL_RTC_SET_TIME, &new_time);
                }
            }
        }
    }
    else if (strcmp(key, "weekday") == 0) {
        safe_strcpy(g_finsh_data.weekday_str, value, sizeof(g_finsh_data.weekday_str));
    }
}

static void handle_weather_data(const char *key, const char *value)
{
    if (strcmp(key, "temp") == 0) {
        g_finsh_data.temperature = atoi(value);
        g_finsh_data.weather_valid = true;
    }
    else if (strcmp(key, "weather_code") == 0) {
        g_finsh_data.weather_code = atoi(value);
    }
    else if (strcmp(key, "humidity") == 0) {
        g_finsh_data.humidity = atoi(value);
    }
    else if (strcmp(key, "pressure") == 0) {
        g_finsh_data.pressure = atoi(value);   
    }
    else if (strcmp(key, "city_code") == 0) {
        g_finsh_data.city_code = atoi(value);

    }
    
    if (g_finsh_data.weather_valid) {
        weather_data_t weather = {0};
        
        weather.temperature = (float)g_finsh_data.temperature;
        weather.humidity = (float)g_finsh_data.humidity;
        weather.pressure = g_finsh_data.pressure;
        weather.weather_code = g_finsh_data.weather_code;
        weather.valid = true;
        
        if (g_finsh_data.weather_code >= 0 && g_finsh_data.weather_code < WEATHER_CODE_MAX && weather_code_map[g_finsh_data.weather_code] != NULL) {
            safe_strcpy(weather.weather, weather_code_map[g_finsh_data.weather_code], sizeof(weather.weather));
        } else {
            safe_strcpy(weather.weather, "未知", sizeof(weather.weather));
        }
        
        if (g_finsh_data.city_code >= 0 && g_finsh_data.city_code < CITY_CODE_MAX && city_code_map[g_finsh_data.city_code] != NULL) {
            safe_strcpy(weather.city, city_code_map[g_finsh_data.city_code], sizeof(weather.city));
        } else {
            safe_strcpy(weather.city, "未知", sizeof(weather.city));
        }
        
        time_t now = time(NULL);
        if (now != (time_t)-1) {
            struct tm *tm_info = localtime(&now);
            if (tm_info) {
                snprintf(weather.update_time, sizeof(weather.update_time),
                        "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
            }
        }
        
        event_data_weather_t weather_event = { .weather = weather };
        event_bus_publish(EVENT_DATA_WEATHER_UPDATED, &weather_event, sizeof(weather_event),
                         EVENT_PRIORITY_NORMAL, MODULE_ID_SERIAL_COMM);
    }
}

static void handle_stock_data(const char *key, const char *value)
{
    if (strcmp(key, "stock_name") == 0) {
        safe_strcpy(g_finsh_data.stock_name, value, sizeof(g_finsh_data.stock_name));
        g_finsh_data.stock_valid = true;
    }
    else if (strcmp(key, "stock_price") == 0) {
        g_finsh_data.stock_price = atof(value);
    }
    else if (strcmp(key, "stock_change") == 0) {
        g_finsh_data.stock_change = atof(value);
    }
    
    if (g_finsh_data.stock_valid) {
        stock_data_t stock = {0};
        
        safe_strcpy(stock.name, g_finsh_data.stock_name, sizeof(stock.name));
        safe_strcpy(stock.symbol, "000001", sizeof(stock.symbol));
        stock.current_price = g_finsh_data.stock_price;
        stock.change_value = g_finsh_data.stock_change;
        
        if (stock.current_price > 0) {
            float prev_price = stock.current_price - stock.change_value;
            if (prev_price > 0) {
                stock.change_percent = (stock.change_value / prev_price) * 100.0f;
            }
        }
        
        stock.valid = true;
        
        time_t now = time(NULL);
        if (now != (time_t)-1) {
            struct tm *tm_info = localtime(&now);
            if (tm_info) {
                snprintf(stock.update_time, sizeof(stock.update_time),
                        "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
            }
        }
        
        event_data_stock_t stock_event = { .stock = stock };
        event_bus_publish(EVENT_DATA_STOCK_UPDATED, &stock_event, sizeof(stock_event),
                         EVENT_PRIORITY_NORMAL, MODULE_ID_SERIAL_COMM);
    }
}

static void handle_system_data(const char *key, const char *value)
{
    float val = atof(value);
    
    if (strcmp(key, "cpu") == 0) {
        g_finsh_data.cpu_usage = val;
        g_finsh_data.system_valid = true;
    }
    else if (strcmp(key, "cpu_temp") == 0) {
        g_finsh_data.cpu_temp = val;
    }
    else if (strcmp(key, "mem") == 0) {
        g_finsh_data.mem_usage = val;
    }
    else if (strcmp(key, "gpu") == 0) {
        g_finsh_data.gpu_usage = val;
    }
    else if (strcmp(key, "gpu_temp") == 0) {
        g_finsh_data.gpu_temp = val;
    }
    else if (strcmp(key, "net_up") == 0) {
        g_finsh_data.net_up = val;
    }
    else if (strcmp(key, "net_down") == 0) {
        g_finsh_data.net_down = val;
    }
    
    if (g_finsh_data.system_valid) {
        system_monitor_data_t sys_data = {0};
        
        sys_data.cpu_usage = g_finsh_data.cpu_usage;
        sys_data.cpu_temp = g_finsh_data.cpu_temp;
        sys_data.gpu_usage = g_finsh_data.gpu_usage;
        sys_data.gpu_temp = g_finsh_data.gpu_temp;
        sys_data.ram_usage = g_finsh_data.mem_usage;
        sys_data.net_upload_speed = g_finsh_data.net_up;
        sys_data.net_download_speed = g_finsh_data.net_down;
        sys_data.valid = true;
        
        time_t now = time(NULL);
        if (now != (time_t)-1) {
            struct tm *tm_info = localtime(&now);
            if (tm_info) {
                snprintf(sys_data.update_time, sizeof(sys_data.update_time),
                        "%02d:%02d:%02d", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
            }
        }
        
        event_data_system_t system_event = { .system = sys_data };
        event_bus_publish(EVENT_DATA_SYSTEM_UPDATED, &system_event, sizeof(system_event),
                         EVENT_PRIORITY_NORMAL, MODULE_ID_SERIAL_COMM);
    }
}

static void handle_finsh_key_value(const char *key, const char *value)
{
    if (!key || !value) return;
    
    if (strcmp(key, "time") == 0 || strcmp(key, "date") == 0 || strcmp(key, "weekday") == 0) {
        handle_time_data(key, value);
    }
    else if (strcmp(key, "temp") == 0 || strcmp(key, "weather_code") == 0 || 
             strcmp(key, "humidity") == 0 || strcmp(key, "pressure") == 0 || 
             strcmp(key, "city_code") == 0) {
        handle_weather_data(key, value);
    }
    else if (strcmp(key, "stock_name") == 0 || strcmp(key, "stock_price") == 0 || 
             strcmp(key, "stock_change") == 0) {
        handle_stock_data(key, value);
    }
    else if (strcmp(key, "cpu") == 0 || strcmp(key, "cpu_temp") == 0 || 
             strcmp(key, "mem") == 0 || strcmp(key, "gpu") == 0 || 
             strcmp(key, "gpu_temp") == 0 || strcmp(key, "net_up") == 0 || 
             strcmp(key, "net_down") == 0) {
        handle_system_data(key, value);
    }
    else {
        rt_kprintf("[Finsh] Unknown key: %s = %s\n", key, value);
    }
}

static void process_finsh_command(const char *cmd_str)
{
    if (!cmd_str) return;
    
    size_t cmd_len = strlen(cmd_str);
    if (cmd_len > SERIAL_RX_BUFFER_SIZE - 1) {
        g_serial_status.invalid_commands_count++;
        return;
    }
    
    char command[16] = {0};
    char key[32] = {0};
    char value[128] = {0};
    
    int parsed = sscanf(cmd_str, "%15s %31s %127[^\r\n]", command, key, value);
    
    if (parsed >= 3 && strcmp(command, "sys_set") == 0) {
        command[15] = '\0';
        key[31] = '\0';
        value[127] = '\0';
        
        remove_quotes(value);
        
        handle_finsh_key_value(key, value);
        
        g_serial_status.total_commands_received++;
        update_connection_status();
        
    } else {
        g_serial_status.invalid_commands_count++;
    }
}

static rt_err_t serial_rx_callback(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(rx_sem);
    return RT_EOK;
}

static void serial_rx_thread_entry(void *parameter)
{
    (void)parameter;
    
    char ch;
    static char line_buffer[SERIAL_RX_BUFFER_SIZE];
    static int line_index = 0;
    static uint32_t consecutive_errors = 0;
    static rt_tick_t last_hid_check = 0;
    
    g_serial_status.last_received_tick = rt_tick_get();
    g_serial_status.connection_alive = false;
    
    while (1) {
        rt_size_t read_bytes = 0;
        
        while (read_bytes != 1) {
            read_bytes = rt_device_read(serial_device, -1, &ch, 1);
            
            if (read_bytes != 1) {
                rt_err_t sem_result = rt_sem_take(rx_sem, RT_WAITING_FOREVER);
                if (sem_result != RT_EOK) {
                    consecutive_errors++;
                    
                    if (consecutive_errors > 50) {
                        rt_thread_mdelay(1000);
                        consecutive_errors = 0;
                    }
                    continue;
                }
            }
        }
        
        consecutive_errors = 0;
        
        rt_tick_t now = rt_tick_get();
        if ((now - last_hid_check) > rt_tick_from_millisecond(30000)) {
            if (hid_device_ready()) {
                int sem_count = hid_get_semaphore_count();
                if (sem_count > 1) {
                    hid_reset_semaphore();
                }
            }
            last_hid_check = now;
        }
        
        if (ch == '\n' || ch == '\r') {
            if (line_index > 0) {
                line_buffer[line_index] = '\0';
                
                if (line_index < SERIAL_RX_BUFFER_SIZE - 1) {            
                    if (consecutive_errors > 10) {
                        rt_thread_mdelay(100);
                        consecutive_errors = 0;
                    }
                    
                    if (strlen(line_buffer) > 0) {
                        process_finsh_command(line_buffer);
                    }
                    
                    consecutive_errors = 0;
                } else {
                    g_serial_status.invalid_commands_count++;
                    consecutive_errors++;
                }
                
                line_index = 0;
                memset(line_buffer, 0, sizeof(line_buffer));
            }
        } else if (ch >= 0x20 || ch == 0x09) {
            if (line_index < SERIAL_RX_BUFFER_SIZE - 2) {
                line_buffer[line_index++] = ch;
            } else {
                line_index = 0;
                memset(line_buffer, 0, sizeof(line_buffer));
                g_serial_status.invalid_commands_count++;
                consecutive_errors++;
            }
        }
    }
}

int serial_data_handler_init(void)
{
    rt_thread_t thread;
    
    serial_device = rt_device_find(SERIAL_DEVICE_NAME);
    if (!serial_device) {
        return -RT_ERROR;
    }
    
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    config.baud_rate = BAUD_RATE_1000000;
    rt_device_control(serial_device, RT_DEVICE_CTRL_CONFIG, &config);
    
    rt_err_t ret = rt_device_open(serial_device, RT_DEVICE_FLAG_INT_RX);
    if (ret != RT_EOK) {
        return ret;
    }
    
    rx_sem = rt_sem_create("finsh_rx_sem", 0, RT_IPC_FLAG_PRIO);
    if (!rx_sem) {
        rt_device_close(serial_device);
        return -RT_ENOMEM;
    }
    
    watchdog_timer = rt_timer_create("finsh_watchdog",
                                     serial_watchdog_timer_cb,
                                     RT_NULL,
                                     rt_tick_from_millisecond(WATCHDOG_CHECK_INTERVAL_MS),
                                     RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
    if (!watchdog_timer) {
        rt_sem_delete(rx_sem);
        rt_device_close(serial_device);
        return -RT_ENOMEM;
    }
    
    rt_timer_start(watchdog_timer);
    
    rt_device_set_rx_indicate(serial_device, serial_rx_callback);
    
    thread = rt_thread_create("finsh_rx",
                              serial_rx_thread_entry,
                              RT_NULL,
                              4096,
                              10, 10);
    if (thread) {
        rt_thread_startup(thread);
        return RT_EOK;
    }
    
    rt_timer_stop(watchdog_timer);
    rt_timer_delete(watchdog_timer);
    rt_sem_delete(rx_sem);
    rt_device_close(serial_device);
    return -RT_ERROR;
}

int serial_data_handler_deinit(void)
{
    if (watchdog_timer) {
        rt_timer_stop(watchdog_timer);
        rt_timer_delete(watchdog_timer);
        watchdog_timer = RT_NULL;
    }
    
    if (serial_device) {
        rt_device_close(serial_device);
        serial_device = NULL;
    }
    
    if (rx_sem) {
        rt_sem_delete(rx_sem);
        rx_sem = NULL;
    }
    return RT_EOK;
}