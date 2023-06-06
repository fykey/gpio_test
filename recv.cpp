#include <pigpio.h>
#include <cstring>
#include <ctime>
#include <unistd.h>
#define GPS_DEVICE "/dev/serial0"
#define LORA_DEVICE "/dev/serial1"
#define BAUD_RATE 115200 //9600から115200に変えた LR2の仕様かな
// LoRaモジュールのピン番号
#define LoRa_Rst 16//14
#define LoRa_RX 14//13
#define LoRa_TX 15//12
void LoRa_reset()
{
  gpioWrite(LoRa_Rst, 0);
  gpioDelay(100000);
  pigpio.gpioWrite(LoRa_Rst, 1);
  gpioDelay(1500000);
}

void LoRa_send(int serialPort, const char* msg)
{
  size_t len = strlen(msg);
  for (size_t i = 0; i < len; i++)
  {
    serWriteByte(serialPort, msg[i]);
  }
}
int LoRa_recv(int serialPort, char* buf, int bufSize)
{
  int bytesRead = serRead(serialPort, buf, bufSize - 1);
  if (bytesRead >= 0)
  {
    buf[bytesRead] = '\0';
  }
  return bytesRead;
}
void setLoRaMode(int serialPort, int bw, int sf)
{
  char buf[64];
  LoRa_send(serialPort, "config\r\n");//通信モードから設定モードへの切り替え(LRをリセット)
  time_sleep(0.2);
  LoRa_reset();
  time_sleep(1.5);
  while (true)
  {
    LoRa_recv(serialPort, buf, sizeof(buf));
    gpioDelay(100000);
    if (strstr(buf, "Mode"))
    {
      std::cout << buf << std::endl;
      break;
    }
  }
sprintf(buf, "bw %d\r\n", bw);
  LoRa_send(serialPort, buf);
  sprintf(buf, "sf %d\r\n", sf);
  LoRa_send(serialPort, buf);
  LoRa_send(serialPort, "q 2\r\n");
  LoRa_send(serialPort, "w\r\n");
  LoRa_reset();
  std::cout << "LoRa module set to new mode" << std::endl;
  time_sleep(1.0);
}
int main()
{
  if (gpioInitialise() < 0)
  {
    std::cerr << "Failed to initialize pigpio" << std::endl;
    return 1;
  }
  //int loraSerial = gpioSerialReadOpen(14, 9600, 0); // GPIO 14 を使用してシリアルポートを開きます
  //int loraSerial = serOpen(LORA_DEVICE, BAUD_RATE, 0); // GPIO 14 を使用してシリアルポートを開きます
  int loraSerial = serOpen(const_cast<char*>(LORA_DEVICE), BAUD_RATE, 0);
  if (loraSerial < 0)
  {
    std::cerr << "Failed to open serial port" << std::endl;
    gpioTerminate();
    return 1;
  }
  setLoRaMode(loraSerial, 4, 7);//PC側の設定に合わせてbwを4 sfを7に
  // LoRaモジュールのモードを設定（例: バンド幅3, スプレッドファクタ12）
  //const char* gpsData = "GPS_VALUE_HERE";
  while (true)
  {
    char loraRecv[64];
    while (true)
    {
      int bytesRead = LoRa_recv(loraSerial, loraRecv, sizeof(loraRecv));
      gpioDelay(2);
      if (strstr(loraRecv, "OK"))
      {
        std::cout << "LoRa response: " << loraRecv << std::endl;
        break;
      }
      else if (bytesRead > 0)
      {
        std::cout << "Received data: " << loraRecv << std::endl;
      }
      else{
      std::cout<<"communication error: "<<loraRecv << std::endl;
      }
    }
    time_sleep(1.0);
  }
  
  gpioSerialReadClose(loraSerial);
  gpioTerminate();
  return 0;
}