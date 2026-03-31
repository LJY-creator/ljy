#include <Arduino.h>
#include "driver/i2s.h"

// 喇叭引脚定义
#define BCLK 10
#define LRC  11
#define DIN  12

// 正弦波数据表（用于生成不同频率声音）
const int16_t sine_table[] = {
  0,402,804,1205,1604,2001,2395,2785,3171,3552,3926,
  4294,4654,5006,5349,5682,6005,6317,6617,6905,7180,
  7441,7688,7920,8137,8337,8521,8688,8837,8969,9082,
  9177,9253,9310,9348,9366,9366,9346,9307,9249,9172,
  9075,8960,8826,8673,8501,8311,8102,7876,7632,7370,
  7092,6797,6486,6160,5818,5462,5092,4708,4311,3901,
  3479,3045,2600,2144,1678,1203,720,229,-264,-755,-1244
};
#define SINE_LEN 128  // 正弦波数据长度

int mode = 0;          // 声音模式（0/1/2）
bool key_down = 0;     // 按键状态（防止误触）

// I2S初始化（配置音频参数，与喇叭模块匹配）
void i2s_init() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),  // 主机模式+发送模式
    .sample_rate = 44100,                                 // 采样率（标准音频采样率）
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,         // 16位采样（音质清晰）
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,          // 左右声道输出
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,     // 标准I2S通信格式
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,             // 中断优先级
    .dma_buf_count = 4,                                   // DMA缓冲区数量
    .dma_buf_len = 512,                                   // 每个缓冲区长度
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  // 引脚配置
  i2s_pin_config_t pin_config = {
    .bck_io_num = BCLK,
    .ws_io_num = LRC,
    .data_out_num = DIN,
    .data_in_num = I2S_PIN_NO_CHANGE 
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);  // 安装I2S驱动
  i2s_set_pin(I2S_NUM_0, &pin_config);                  // 配置I2S引脚
}

void setup() {
  Serial.begin(9600);         // 串口初始化（用于打印模式）
  pinMode(0, INPUT_PULLUP);   // BOOT键（GPIO0）设为上拉输入，用于切换模式
  i2s_init();                 // 初始化I2S音频
  Serial.println("喇叭已启动");
  Serial.println("按BOOT键切换声音模式（0/1/2）");
}

void loop() {
  static int saw = 0;         // 锯齿波变量
  static int p1 = 0, p2 = 0;  // 正弦波索引变量
  int16_t L = 0, R = 0;      // 左右声道音频数据

  // BOOT键切换模式（消抖处理，防止误触）
  if (digitalRead(0) == 0 && !key_down) {
    delay(20);  // 消抖延时
    if (digitalRead(0) == 0) {
      key_down = 1;
      mode = (mode + 1) % 3;  // 循环切换3种模式
      Serial.printf("当前模式：%d\n", mode);
    }
  }
  if (digitalRead(0) == 1) key_down = 0;  // 释放按键，重置状态

  // 模式0：右声道500Hz锯齿波（清晰单音）
  if (mode == 0) {
    saw += 70;
    L = 0;          // 左声道静音
    R = saw;        // 右声道输出锯齿波
  }
  // 模式1：拍频（1001Hz + 999Hz，有波动感）
  else if (mode == 1) {
    p1 = (p1 + 23) % SINE_LEN;
    p2 = (p2 + 22) % SINE_LEN;
    L = sine_table[p1];
    R = sine_table[p2];
  }
  // 模式2：左声道1kHz + 右声道4kHz（双频对比）
  else {
    p1 = (p1 + 23) % SINE_LEN;
    p2 = (p2 + 90) % SINE_LEN;
    L = sine_table[p1];
    R = sine_table[p2];
  }

  // 组合左右声道数据，发送到喇叭
  int32_t sample = (L << 16) | (R & 0xFFFF);
  size_t bytes_written;
  i2s_write(I2S_NUM_0, &sample, 4, &bytes_written, portMAX_DELAY);
}