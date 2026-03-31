#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include "driver/i2s.h"
#include <EEPROM.h>

#define I2S_BCLK 10
#define I2S_WS   11
#define I2S_DOUT 12

#define SD_CS 5
#define EEPROM_SIZE 512

char name[32] = {0};
bool has_name = false;

#define MAX_SONG 30
struct Song {
  char filename[64];
  uint32_t pos;
};
Song playlist[MAX_SONG];
int song_cnt = 0;
int curr_song = 0;
bool is_play = true;
File f;

void i2s_init() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 44100,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .dma_buf_count = 4,
    .dma_buf_len = 512,
  };
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);

  i2s_pin_config_t pin_conf = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE,
  };
  i2s_set_pin(I2S_NUM_0, &pin_conf);
}

void load_name() {
  EEPROM.get(0, has_name);
  if (has_name) EEPROM.get(1, name);
}

void save_name(const char *s) {
  has_name = true;
  EEPROM.put(0, has_name);
  strcpy(name, s);
  EEPROM.put(1, name);
  EEPROM.commit();
}

void scan_sd() {
  song_cnt = 0;
  File root = SD.open("/");
  while (1) {
    File file = root.openNextFile();
    if (!file) break;
    if (strstr(file.name(), ".mp3")) {
      strcpy(playlist[song_cnt].filename, file.name());
      playlist[song_cnt].pos = 0;
      song_cnt++;
    }
    file.close();
    if (song_cnt >= MAX_SONG) break;
  }
  root.close();
}

void play(int idx) {
  if (idx < 0 || idx >= song_cnt) return;
  if (f) f.close();

  curr_song = idx;
  f = SD.open(playlist[idx].filename);
  if (f) {
    f.seek(playlist[idx].pos);
    is_play = true;
    Serial.printf("▶️ 播放：%s\n", playlist[idx].filename);
  }
}

void setup() {
  Serial.begin(9600);
  EEPROM.begin(EEPROM_SIZE);
  SPI.begin(6,8,7,SD_CS);
  SD.begin(SD_CS);
  i2s_init();

  load_name();
  if (!has_name) {
    Serial.println("为我取名：");
    while (!Serial.available());
    String s = Serial.readStringUntil('\n');
    save_name(s.c_str());
  }
  Serial.printf("你好，我是：%s\n", name);

  scan_sd();
  Serial.printf("找到歌曲：%d 首\n", song_cnt);
  for (int i=0; i<song_cnt; i++) {
    Serial.printf("%d. %s\n", i+1, playlist[i].filename);
  }

  if (song_cnt > 0) play(0);
}

void cmd() {
  if (!Serial.available()) return;
  char c = Serial.read();
  if (c == ' ') is_play = !is_play;
  if (c == 'n') play(curr_song + 1);
  if (c == 'p') play(curr_song - 1);
  Serial.println(is_play ? "播放" : "暂停");
}

void loop() {
  cmd();
  if (!is_play || !f) return;

  uint8_t buf[1024];
  int len = f.read(buf, 1024);
  if (len <= 0) {
    playlist[curr_song].pos = 0;
    play(curr_song + 1);
    return;
  }
  playlist[curr_song].pos = f.position();
  size_t wr;
  i2s_write(I2S_NUM_0, buf, len, &wr, portMAX_DELAY);
}