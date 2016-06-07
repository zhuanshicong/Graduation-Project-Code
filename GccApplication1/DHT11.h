#ifndef DHT11_H
#define DHT11_H
#define DHT_DATA_I DDRA &= ~(1<<PA0)
#define DHT_DATA_O DDRA |=  (1<<PA0)
#define DHT_L      PORTA &= ~(1<<PA0)
#define DHT_H      PORTA |=  (1<<PA0)
#define CHECK_DHT_H         (PINA&0x01)
#endif