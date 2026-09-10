/*
  接线说明：
ISD1820
  VCC------5V
  GND------GND
  PLAYL----4

人体红外
  VCC-----5V
  GND-----GND
  OUT-----8

  程序功能：人体红外HC-SR501入侵语音报警器
*/
 
void setup() {
  //setup设定，只执行一次
  Serial.begin(115200);
  pinMode(8, INPUT); //定义D8作为输入（人体红外线SR-501）
  pinMode(4, OUTPUT); //定义D4作为输出（录放音ISD1820） 
  digitalWrite(4, LOW);
}
 
void loop() {
  //loop循环，重复执行不停止
  if (digitalRead(8) == HIGH) {
    Serial.println("有人进入，报警！");//侦测到有人经过
    digitalWrite(4, HIGH); //播放录音
  }
  else {
    Serial.println("平安");//无人经过
    digitalWrite(4, LOW);//保持低电位，不过可以省略
  }
  delay(1000);
}
