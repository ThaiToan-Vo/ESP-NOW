#ifndef STRUCT_COMMON_H
#define STRUCT_COMMON_H
#include <stdint.h>

#define MAX_NODES 10

// Khai báo sự tồn tại của Queue cho toàn hệ thống
extern QueueHandle_t mqtt_to_espnow_queue;
extern QueueHandle_t espnow_to_mqtt_queue;

// dữ liệu đo được gửi từ Node lên Gateway qua ESP-NOW
typedef struct {
    char header[2];    // "N1" - Mật khẩu nhận diện
    uint8_t node_id;   // ID của Node (1, 2, 3...)
    float voltage;     // V
    float current;     // I
    float power;       // P
    float energy;      // Wh
    float power_fac;
} __attribute__((packed)) power_data_t;

// dữ liệu nhận được từ MQTT để gửi xuống node
typedef struct {
    uint8_t node_id;
    uint8_t gain;
    uint8_t energy_cmd; 
} __attribute__((packed)) node_ctrl_t;

// struct chỉ dùng để lưu MAC tại Gateway
typedef struct {
    uint8_t node_id;
    uint8_t mac[6];
    bool is_active;
} node_info_t;

#endif
