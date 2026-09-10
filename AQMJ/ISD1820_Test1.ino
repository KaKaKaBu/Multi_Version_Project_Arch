/*
  接线说明：
  VCC------5V
  GND------GND
  REC------3
  PLAYL----4

  程序功能：录音10秒，播放10秒
*/
 
int Rec = 3;//定义录音接脚为D3
int Play = 4;//定义播放接脚为D4
 
void setup() {
  pinMode(Rec, OUTPUT);//设置为输出
  pinMode(Play, OUTPUT);
}
 
void loop() {
  digitalWrite(Rec, HIGH);//打开录音，延时10秒
  delay(10000);
  digitalWrite(Rec, LOW);
  delay(50);
  digitalWrite(Play, HIGH);//播放录音10秒
  delay(10000);
  digitalWrite(Play, LOW);
  delay(50);
}
