from machine import Pin, I2C
from ssd1306 import SSD1306_I2C
import time

i2c = I2C(1, scl=Pin(7), sda=Pin(6), freq=200000)
oled = SSD1306_I2C(128, 32, i2c, 0x3c)

#sec min hour week day mout year
NowTime = b'\x30\x15\x07\x02\x07\x06\x21'
w  = ["Sunday","Monday","Tuesday","Wednesday","Thurday","Friday","Saturday"];

bus = I2C(1,scl=Pin(7),sda=Pin(6))

address = 0x68
register = 0x00

def ds3231SetTime():
    bus.writeto_mem(int(address),int(register),NowTime)

def ds3231ReadTime():
    return bus.readfrom_mem(int(address),int(register),7);

ds3231SetTime()

while 1:
    t = ds3231ReadTime()
    a = t[0]&0x7F  #sec
    b = t[1]&0x7F  #min
    c = t[2]&0x3F  #hour
    d = t[3]&0x07  #week
    e = t[4]&0x3F  #day
    f = t[5]&0x1F  #mouth
    print("20%x/%02x/%02x %02x:%02x:%02x %s" %(t[6],t[5],t[4],t[2],t[1],t[0],w[t[3]-1]))

    oled.fill(0)
    oled.text("%02x:%02x:%02x" %(t[2],t[1],t[0]), 27, 2)
    oled.text("%02x/%02x/20%x" %(t[4],t[5],t[6]), 20, 13)
    oled.text((w[t[3]-1]), 34, 24)
    oled.show()
    
    time.sleep(1)

