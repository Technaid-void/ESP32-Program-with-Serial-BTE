void restartbluetooth()
{
  restartbluetooth(true);
}

void restartbluetooth(bool flush)
{
  if(flush)
    SerialBT.flush();
  SerialBT.end();
  waiting_for_password = true;
  delay(200);
  SerialBT.begin("MyESP32");
  delay(200);


}