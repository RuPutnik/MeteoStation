#ifndef GENERAL_H
#define GENERAL_H

#include <SPI.h>
#include <RF24.h>

#define LOOP_DELAY_MSEC            10
#define SETUP_DELAY                2000

#define SERIAL_SPEED               9600
#define RADIO_CHANNEL_NUMBER       110
#define DATA_PACKET_LENGTH		   sizeof(MeteoDataPacket)
#define ACTSERV_PACKET_LENGTH	   sizeof(ActionServicePacket)
#define MAX_PACKET_NUMBER		   1000000

enum MODULE_ID: uint8_t
{
  CENTRAL_MODULE_ID     = 0,
  INTERNAL_MODULE_ID    = 1,
  EXTERNAL_MODULE_ID    = 2,
  INCORRECT_MODULE_ID   = 3
};

enum TYPE_PACKET: uint8_t
{
  DATA    = 1,
  CONTROL = 2,
  SERVICE = 3,
  UNKNOWN = 4
};

enum COMMANDS_TYPE: uint8_t
{
  RESTART_ALL          = 1,
  STOP_START_SEND      = 2,
  CHANGE_SEND_INTERVAL = 3,
  GET_TIME_INTERVAL    = 4,
  GET_LIFE_TIME        = 5, //в миллисек.
  HEARTBEAT            = 6
};

enum SERVICE_MSG_TYPE: uint8_t
{
  START_MODULE_SUCCESS = 1,
  SUCCESS_GET_COMMAND  = 2,
  ERROR_START_DETECTOR = 3,
  REPORT_TIME_INTERVAL = 4,
  REPORT_LIFE_TIME     = 5,
  GET_ERROR_COMMAND    = 6
};

inline float normalize(int value){
  return value / 1024.0;  
}

inline uint32_t calcCheckSum(void* data, unsigned int byteLength){
  uint32_t crc = 0;
  for(unsigned int i = 0; i < byteLength - sizeof(uint32_t); i++){
    crc += (reinterpret_cast<uint8_t*>(data)[i] * 211);  //Calc CRC
  }
  return crc;   
}

#pragma pack(push, 1)
struct HeaderPacket
{
	unsigned int type: 4;
	unsigned int dest: 4;
	unsigned int sender: 4;
	uint32_t numPacket: 20;
};

struct MeteoDataPacket : HeaderPacket
{
	float val1;
	float val2;
	float val3;
	float val4;
	float val5;
	float val6;
	uint32_t ckSum;
};

struct ActionServicePacket : HeaderPacket
{
	uint16_t id;
	float valueParam;
	uint32_t ckSum;
};
#pragma pack(pop)

#endif