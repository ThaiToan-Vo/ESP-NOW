#include "app_espnow.h"
#include "struct_common.h"


// receive callback function
uint8_t led_trigger = 0;
#define TAG "ESPNOW_MODULE"

/*
    Struct node_info_t: dùng để lưu trữ node id và mac của các node
    Hàm update_node_directory: dùng để kiểm tra xem node ID và mac đã tồn tại chưa, nếu chưa thì cập nhật vào
    Hàm get_mac_by_node_id: dùng để lấy mac từ node id
    
    
    Flow rec callback:
    - Bên node và gateway có chung struct power_data_t, ngoài chứa các dữ liệu về V I P thì còn có header để nhận diện
    là "N1", và node_id của chính node đó.
    - Đầu tiên lấy thông tin từ struct power_data_t
    - Kiểm tra header từ gói tin
    - Sau khi nhận gói tin đầu tiên chứa header và node_id, gateway kiểm tra xem đã có mac trong ram chưa,
    nếu chưa sẽ addpeer với node
    - Sau đó lưu node_id và mac bằng update_node_directory


*/




/*
    Cách hoạt động của hàm update_node_directory:
    - VÒng for thứ nhất: 
        + Với node[i].is_active đã được set = 1 thì có nghĩa là đã được add vào danh bạ 
        + Khi kiểm tra nếu node đã được add vào danh bạ thì chỉ cần cập nhật lại mac để tránh khi reset
    - VÒng for thứ hai:
        + Nếu node chưa được add thì is_active = 0
        + Lưu mac và set is_active = 1 để tránh ghi đè làm tràn mảng
*/


// Mảng lưu trữ danh sách các Node đã kết nối
static node_info_t node_directory[MAX_NODES];


void update_node_directory(uint8_t id, const uint8_t *mac) 
{
    // 1. Kiểm tra xem Node ID này đã tồn tại chưa để cập nhật MAC mới (nếu thay chip)
    for (int i = 0; i < MAX_NODES; i++) 
    {
        if (node_directory[i].is_active && node_directory[i].node_id == id) 
        {
            memcpy(node_directory[i].mac, mac, 6);
            return;
        }
    }

    // 2. Nếu chưa có, tìm chỗ trống để thêm mới
    for (int i = 0; i < MAX_NODES; i++) 
    {
        if (!node_directory[i].is_active) 
        {
            node_directory[i].node_id = id;
            memcpy(node_directory[i].mac, mac, 6);
            node_directory[i].is_active = true;
            ESP_LOGI("DIR", "Da luu Node ID %d vao danh ba", id);
            return;
        }
    }
}

/*
    Nếu node đã lưu trong danh bạ thì trả về mac của node đó
*/
bool get_mac_by_node_id(uint8_t id, uint8_t *out_mac) 
{
    for (int i = 0; i < MAX_NODES; i++) 
    {
        if (node_directory[i].is_active && node_directory[i].node_id == id) 
        {
            memcpy(out_mac, node_directory[i].mac, 6);
            return true;
        }
    }
    return false; // Không tìm thấy
}


void recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    power_data_t *incoming = (power_data_t *)data;
    if (incoming->header[0] == 'N' && incoming->header[1] == '1')
    {
        if (!esp_now_is_peer_exist(info->src_addr)) // kiểm tra mac này đã tồn tại chưa, nếu chưa thì add lại
        {
            esp_now_peer_info_t peer = {0};
            memcpy(peer.peer_addr, info->src_addr, 6); // Lấy MAC trực tiếp từ "phong bì" gói tin
            peer.channel = 0;
            // peer.ifidx = WIFI_IF_STA;
            peer.encrypt = false;

            esp_err_t ret = esp_now_add_peer(&peer);
            if (ret == ESP_OK) 
                {
                    ESP_LOGI("ESPNOW", "Tu dong add peer N1 thanh cong!");
                }
            
        }
        update_node_directory(incoming->node_id, info->src_addr);
        led_trigger = 1;
        ESP_LOGI("ESPNOW", "Nhan duoc lenh N1, trigger LED!");
    }
    else if (len >= sizeof(power_data_t)) 
    {
            // Đây là gói tin "Dữ liệu đo điện"
            ESP_LOGI("GATEWAY", "Nhan du lieu tu Node %d: V=%.1f, I=%.2f", 
                     incoming->node_id, incoming->voltage, incoming->current);
            
            // Tiến hành đẩy dữ liệu này lên MQTT qua task mqtt send
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xQueueSendFromISR(espnow_to_mqtt_queue, incoming, &xHigherPriorityTaskWoken);
            if (xHigherPriorityTaskWoken == pdTRUE) {
                portYIELD_FROM_ISR();
            }
    }
    
}

// send callback function
void send_cb(const esp_now_send_info_t *info, esp_now_send_status_t status)
{
    
}

void espnow_init(void)
{
    esp_now_init(); // hàm khởi tạo ESP NOW
    esp_now_register_send_cb(send_cb);
    esp_now_register_recv_cb(recv_cb);
}