// 1. 每秒发送一次 "Hello World\r\n"
// 2. 收到回车（\r 或 \n）时，输出三行固定字符串

unsigned long lastSendTime = 0;
const unsigned long sendInterval = 1000; // 1Hz

void setup() {
  Serial.begin(115200);  // 初始化串口
  Serial.println("UART Ready. Type something and press Enter.");
}

void loop() {
  // 任务1：每秒发送 Hello World
  if (millis() - lastSendTime >= sendInterval) {
    lastSendTime = millis();
    Serial.print("Hello World\r\n"); 
  
  }

  // 2：接收用户输入，检测回车
  if (Serial.available()) {
    char c = Serial.read();
    // 检测回车
    if (c == '\r' || c == '\n') {
      // 输出固定内容
      Serial.print("GEL37KXHDU9G\r\n");
      Serial.print("FXLKNKWHVURC\r\n");
      Serial.print("nCE4K7KEYCUPQ\r\n");
    }
    
  }
}