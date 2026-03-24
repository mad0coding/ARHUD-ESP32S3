#include "UdpImg.h"

#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"

// --- 配置区 ---
#define WIFI_SSID      "LandE A55"
#define WIFI_PASS      "13524600"
#define HOST_IP_ADDR   "192.168.38.196"  // 你的电脑IP
#define PORT           8081

// #define IMG_WIDTH      640
// #define IMG_HEIGHT     480
#define IMG_WIDTH      320
#define IMG_HEIGHT     240
#define PIXELS_PER_PKT 320  // 每个包发送的像素数

static const char *TAG = "UDP_SENDER";

// 协议头部
const uint8_t HEADER[] = {0x55, 0x01};

// 模拟一帧图像的缓冲区 (RGB888)
uint8_t frame_buffer[3]; 

void udp_client_task(void *pvParameters) {
    char rx_buffer[128];
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);

    while (1) {
        int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }

        int frame_count = 0;
        int square_x = 0;
        int square_y = 0;
        int square_size = 60;

        while (1) {
            // 更新方块位置 (简单地让它斜着跑)
            square_x = (square_x + 5) % (IMG_WIDTH - square_size);
            square_y = (square_y + 2) % (IMG_HEIGHT - square_size);
            
            // 遍历全图并分包发送
            // 注意：为了性能，这里我们不真的准备全图buffer，而是实时计算
            for (int i = 0; i < IMG_WIDTH * IMG_HEIGHT; i += PIXELS_PER_PKT) {
                int current_batch = PIXELS_PER_PKT;
                if (i + current_batch > IMG_WIDTH * IMG_HEIGHT) {
                    current_batch = (IMG_WIDTH * IMG_HEIGHT) - i;
                }

                int start_x = i % IMG_WIDTH;
                int start_y = i / IMG_WIDTH;

                // 构造数据包
                // Header(2) + Count(2) + X(2) + Y(2) + Data(current_batch * 3)
                int pkt_size = 8 + current_batch * 3;
                uint8_t *packet = malloc(pkt_size);
                
                packet[0] = 0x55;
                packet[1] = 0x01;
                // 大端序转换
                packet[2] = (current_batch >> 8) & 0xFF;
                packet[3] = current_batch & 0xFF;
                packet[4] = (start_x >> 8) & 0xFF;
                packet[5] = start_x & 0xFF;
                packet[6] = (start_y >> 8) & 0xFF;
                packet[7] = start_y & 0xFF;

                // 填充像素数据：如果在方块范围内则设为彩色，否则黑色
                for (int p = 0; p < current_batch; p++) {
                    int curr_p_idx = i + p;
                    int px = curr_p_idx % IMG_WIDTH;
                    int py = curr_p_idx / IMG_WIDTH;

                    int offset = 8 + p * 3;
                    if (px >= square_x && px < square_x + square_size &&
                        py >= square_y && py < square_y + square_size) {
                        packet[offset]     = (frame_count * 2) % 255; // R
                        packet[offset + 1] = 255 - (frame_count % 255); // G
                        packet[offset + 2] = 150; // B
                    } else {
                        packet[offset] = packet[offset+1] = packet[offset+2] = 0;
                    }
                }

                sendto(sock, packet, pkt_size, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
                free(packet);
                
                // 微秒级延迟，防止发送过快冲垮网络栈
                // usleep(10);
            }
            frame_count++;
            vTaskDelay(pdMS_TO_TICKS(10)); // 控制帧率
        }

        if (sock != -1) {
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

// --- 以下是标准的 WiFi 连接代码 ---
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "retry to connect to the AP");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}

void udp_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // 启动 UDP 发送任务
    xTaskCreate(udp_client_task, "udp_sender", 4096, NULL, 5, NULL);
}



