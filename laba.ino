#include <Arduino.h>
#include "driver/i2s.h"
#include <math.h>

// 引脚
#define BCLK 4
#define LRC  5
#define DIN  6

// 音频参数
#define SAMPLE_RATE 16000
#define AMP 2000        // 幅度

// 全局变量
int mode = 0;      // 0:右500Hz锯齿波, 1:左1001+右999, 2:左1k+右4k
int sub = 0;       // 仅在mode=2时：0立体声,1仅左,2仅右

// 相位（角度）
float phL=0, phR=0, phSaw=0;
float stepL, stepR, stepSaw;

void setup() {
  Serial.begin(115200);
  
  // ----- I2S初始化（固定写法，不用理解细节）-----
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .dma_buf_count = 4,
    .dma_buf_len = 256,
  };
  i2s_pin_config_t pin = {BCLK, LRC, DIN, I2S_PIN_NO_CHANGE};
  i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin);
  // --------------------------------------------
  
  // 初始化锯齿波步长
  stepSaw = 2*3.14159 * 500.0 / SAMPLE_RATE;
  
  Serial.println("命令: 0=锯齿波(右), 1=差频, 2=1k+4k, 在2下 l=左,r=右,s=立体声");
}

void loop() {
  //串口命令处理 
  if (Serial.available()) {
    char c = Serial.read();
    if (c == '0') {
      mode = 0; sub = 0;
      stepSaw = 2*3.14159 * 500.0 / SAMPLE_RATE;
      phL = phR = phSaw = 0;
      Serial.println("模式0: 右声道500Hz锯齿波");
    }
    else if (c == '1') {
      mode = 1; sub = 0;
      stepL = 2*3.14159 * 1001.0 / SAMPLE_RATE;
      stepR = 2*3.14159 * 999.0 / SAMPLE_RATE;
      phL = phR = phSaw = 0;
      Serial.println("模式1: 左1001Hz + 右999Hz正弦波");
    }
    else if (c == '2') {
      mode = 2; sub = 0;
      stepL = 2*3.14159 * 1000.0 / SAMPLE_RATE;
      stepR = 2*3.14159 * 4000.0 / SAMPLE_RATE;
      phL = phR = phSaw = 0;
      Serial.println("模式2: 左1kHz+右4kHz (输入 l/r/s 切换)");
    }
    else if (mode == 2 && c == 'l') { sub = 1; Serial.println("仅左声道 1kHz"); }
    else if (mode == 2 && c == 'r') { sub = 2; Serial.println("仅右声道 4kHz"); }
    else if (mode == 2 && c == 's') { sub = 0; Serial.println("立体声"); }
    // 清空缓冲区
    while (Serial.available()) Serial.read();
  }
  
  // ----- 生成左右声道的值（范围 -1 到 1）-----
  float left = 0, right = 0;
  
  if (mode == 0) {
    // 右声道锯齿波
    right = (phSaw / (2*3.14159)) * 2.0 - 1.0;
    phSaw += stepSaw;
    if (phSaw >= 2*3.14159) phSaw -= 2*3.14159;
  }
  else if (mode == 1) {
    // 左1001Hz，右999Hz
    left = sinf(phL);   phL += stepL;   if (phL >= 2*3.14159) phL -= 2*3.14159;
    right = sinf(phR);  phR += stepR;   if (phR >= 2*3.14159) phR -= 2*3.14159;
  }
  else { // mode == 2
    // 先计算两个正弦波
    float l = sinf(phL);  phL += stepL;  if (phL >= 2*3.14159) phL -= 2*3.14159;
    float r = sinf(phR);  phR += stepR;  if (phR >= 2*3.14159) phR -= 2*3.14159;
    if (sub == 1) { left = l; right = 0; }
    else if (sub == 2) { left = 0; right = r; }
    else { left = l; right = r; }
  }
  
  // 转换为16位整数并发送到I2S
  int16_t samples[2] = { (int16_t)(left * AMP), (int16_t)(right * AMP) };
  size_t written;
  i2s_write(I2S_NUM_0, samples, sizeof(samples), &written, portMAX_DELAY);
}