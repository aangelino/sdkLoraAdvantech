/**
 * @file main.cpp
 *
 * @brief Lora Node SDK Sample
 *
 * example show how to set/save config and read/send sensor data via LoRa periodically;
 *
 * @author AdvanWISE
*/


#include "mbed.h"
#include "node_api.h"

#define NODE_DEBUG(x,args...) node_printf_to_serial(x,##args)

#define NODE_DEEP_SLEEP_MODE_SUPPORT 0  ///< Flag to Enable/Disable deep sleep mode
#define NODE_ACTIVE_PERIOD_IN_SEC 10    ///< Period time to read/send sensor data
#define NODE_ACTIVE_TX_PORT 1          ///< Lora Port to send data


extern Serial debug_serial; ///< Debug serial port
extern Serial m2_serial;    ///< M2 serial port

typedef enum

{
	NODE_STATE_INIT,            ///< Node init state
	NODE_STATE_LOWPOWER = 1,    ///< Node low power state
	NODE_STATE_ACTIVE,          ///< Node active state
	NODE_STATE_TX,              ///< Node tx state
	NODE_STATE_RX_DONE,         ///< Node rx done state
}node_state_t;

struct node_api_ev_rx_done node_rx_done_data;
volatile node_state_t node_state = NODE_STATE_INIT;
static char node_class=1;

static unsigned int  node_sensor_temp_hum=0; ///<Temperature and humidity sensor global

I2C i2c(PC_1, PC_0); ///<i2C define

/** @brief print message via serial
 *
 *  @param format message to print
 *  @returns 0 on success
 */
int node_printf_to_serial(const char * format, ...)
{
	unsigned int i;
	va_list ap;

	char buf[512 + 1];
	memset(buf, 0, 512+1);

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), (char *)format, ap);
	va_end(ap);

	for(i=0; i < strlen(buf); i++)
	{
		debug_serial.putc(buf[i]);
	}

	return 0;
}



/** @brief Temperature and humidity sensor read
 *
 */
static unsigned int hdc1510_sensor(void)
{
	char data_write[3];
	char data_read[4];

	#define HDC1510_REG_TEMP  0x0
	#define HDC1510_ADDR 0x80

	data_write[0]=HDC1510_REG_TEMP;
	i2c.write(HDC1510_ADDR, data_write, 1, 1);
	Thread::wait(50);
	i2c.read(HDC1510_ADDR, data_read, 4, 0);
	float tempval = (float)((data_read[0] << 8 | data_read[1]) * 165.0 / 65536.0 - 40.0);

	/*Temperature*/
	int ss = tempval*100;
	unsigned int yy=0;
	//printf("Temperature: %.2f C\r\n",tempval );
	/*Humidity*/
	float hempval = (float)((data_read[2] << 8 | data_read[3]) * 100.0 / 65536.0);
	yy=hempval*100;
	// printf("Humidity: %.2f %\r\n",hempval);

	return (yy<<16)|ss;

}

/** @brief Temperature and humidity sensor thread
 *
 */
static void node_sensor_temp_hum_thread(void const *args)
{
    	int cnt=0;

    	while(1)
	{
	    cnt++;
	    Thread::wait(10);
	    if(cnt==100)
	    {
	       cnt=0;
	     	node_sensor_temp_hum=(unsigned int )hdc1510_sensor();

	     }
	}

}
/** @brief node tx procedure done
 *
 */
int node_tx_done_cb(void)
{
	node_state=NODE_STATE_LOWPOWER;
	return 0;
}



/** @brief node got rx data
 *
 */
int node_rx_done_cb(struct node_api_ev_rx_done *rx_done_data)
{
	memset(&node_rx_done_data,0,sizeof(struct node_api_ev_rx_done));
	memcpy(&node_rx_done_data,rx_done_data,sizeof(struct node_api_ev_rx_done));
	node_state=NODE_STATE_RX_DONE;
	return 0;
}



/** @brief An example to show version
 *
 */
void node_show_version()
{
	char buf_out[256];
	unsigned short ret=NODE_API_OK;

	memset(buf_out, 0, 256);
	ret=nodeApiGetVersion(buf_out, 256);
	if(ret==NODE_API_OK)
	{
		NODE_DEBUG("Version=%s\r\n", buf_out);
	}
}




/** @brief An example to set node config
 *
 */
void node_set_config()
{
	char deveui[32]={};
	char devaddr[16]={};


	if(nodeApiGetFuseDevEui(deveui,16)!=NODE_API_OK)
	{
		NODE_DEBUG("Get fuse DevEui failed\r\n");
		return;
	}

	nodeApiSetDevEui(deveui);
	nodeApiSetAppEui("00000000000000ab");
	nodeApiSetAppKey("00000000000000000000000000000011");
	strcpy(devaddr,&deveui[8]);
	nodeApiSetDevAddr(devaddr);
	nodeApiSetNwkSKey("00000000000000000000000000000011");
	nodeApiSetAppSKey("00000000000000000000000000000011");
	nodeApiSetDevActMode("2");
	nodeApiSetDevOpMode("1");
	nodeApiSetDevClass("3");
	nodeApiSetDevAdvwiseDataRate("4");
	nodeApiSetDevAdvwiseFreq("923300000");
	nodeApiSetDevAdvwiseTxPwr("20");
}

/** @brief An example to get node config
 *
 */
void node_get_config()
{
	char buf_out[256];
	unsigned short ret=NODE_API_OK;

	memset(buf_out, 0, 256);
	ret=nodeApiGetDevEui(buf_out, 256);
	if(ret==NODE_API_OK)
	{
		NODE_DEBUG("DevEui=%s\r\n", buf_out);
	}

	memset(buf_out, 0, 256);
	ret=nodeApiGetAppEui(buf_out, 256);
	if(ret==NODE_API_OK)
	{
		NODE_DEBUG("AppEui=%s\r\n", buf_out);
	}

	memset(buf_out, 0, 256);
	ret=nodeApiGetDevAddr(buf_out, 256);
	if(ret==NODE_API_OK)
	{
		NODE_DEBUG("DevAddr=%s\r\n", buf_out);
	}

	memset(buf_out, 0, 256);
	ret=nodeApiGetNwkSKey(buf_out, 256);
	if(ret==NODE_API_OK)
	{
		NODE_DEBUG("NwkSKey=%s\r\n", buf_out);
	}

	memset(buf_out, 0, 256);
	ret=nodeApiGetAppSKey(buf_out, 256);
	if(ret==NODE_API_OK)
	{
		NODE_DEBUG( "AppSKey=%s\r\n", buf_out);
	}

	memset(buf_out, 0, 256);
	ret=nodeApiGetDevActMode(buf_out, 256);
	if(ret==NODE_API_OK)
	{
		NODE_DEBUG("DevActMode=%s\r\n", buf_out);
	}

	memset(buf_out, 0, 256);
	ret=nodeApiGetDevOpMode(buf_out, 256);
	if(ret==NODE_API_OK)
	{
		NODE_DEBUG("DevOpMode=%s\r\n", buf_out);
	}

	memset(buf_out, 0, 256);
	ret=nodeApiGetDevClass(buf_out, 256);
	if(ret==NODE_API_OK)
	{
		NODE_DEBUG("DevClass=%s\r\n", buf_out);
		node_class=atoi(buf_out);
	}


	memset(buf_out, 0, 256);
	ret=nodeApiGetDevAdvwiseDataRate(buf_out, 256);
	if(ret==NODE_API_OK)
	{
		NODE_DEBUG("AdvwiseDataRate=%s\r\n", buf_out);
	}

	memset(buf_out, 0, 256);
	ret=nodeApiGetDevAdvwiseFreq(buf_out, 256);
	if(ret==NODE_API_OK)
	{
		NODE_DEBUG("DevAdvwiseFreq=%sHz\r\n", buf_out);
	}

	memset(buf_out, 0, 256);
	ret=nodeApiGetDevAdvwiseTxPwr(buf_out, 256);
	if(ret==NODE_API_OK)
	{
		NODE_DEBUG("DevAdvwiseTxPwr=%sdBm\r\n", buf_out);
	}

}

/** @brief Read sensor data
 *
 *  A simple sample to generate sensor data, user should implement read sensor data
 *  @param data sensor_data
 *  @returns data_length
 */
unsigned char node_get_sensor_data (char *data)
{
	unsigned char len=0;
	unsigned char sensor_data[32];

	memset(sensor_data,0x0,sizeof(sensor_data));

	sensor_data[len+2]=0x1;
	len++; // temperature
	sensor_data[len+2]=0x3;
	len++;  // len:3 bytes
	sensor_data[len+2]=0x0;
	len++; //0 is positive, 1 is negative
	sensor_data[len+2]=(node_sensor_temp_hum>>8)&0xff;
	len++;
	sensor_data[len+2]=node_sensor_temp_hum&0xff;
	len++;
	sensor_data[len+2]=0x2;
	len++;  // humidity
	sensor_data[len+2]=0x2;
	len++; // len:2 bytes
	sensor_data[len+2]=(node_sensor_temp_hum>>24)&0xff;
	len++;
	sensor_data[len+2]=(node_sensor_temp_hum>>16)&0xff;
	len++;

	//header
	sensor_data[0]=len;
	sensor_data[1]=0xc; //publish
	memcpy(data, sensor_data,len+2);

	return len+2;
}


/** @brief An loop to read and send sensor data via LoRa periodically
 *
 */
void node_state_loop()
{
	static unsigned char join_state=0;/*0:init state, 1:not joined, 2: joined*/

	nodeApiSetTxDoneCb(node_tx_done_cb);
	nodeApiSetRxDoneCb(node_rx_done_cb);

	node_state=NODE_STATE_LOWPOWER;

	while(1)
	{
		if(nodeApiJoinState()==0)
		{
			if(join_state==2)
				NODE_DEBUG("LoRa is not joined.\r\n");

			#if NODE_DEEP_SLEEP_MODE_SUPPORT
			nodeApiSetDevSleepRTCWakeup(1);
			#else
			Thread::wait(1000);
			#endif

			join_state=1;
			continue;
		}
		else
		{
			if(join_state<=1)
			{
				node_class=nodeApiDeviceClass();
				NODE_DEBUG("LoRa Joined.\r\n");
				node_state=NODE_STATE_LOWPOWER;
			}

			join_state=2;
		}

		switch(node_state)
		{
			case NODE_STATE_LOWPOWER:
			{
				if(node_class==3)
				{

					static int count=0;
					#if NODE_DEEP_SLEEP_MODE_SUPPORT
					nodeApiSetDevSleepRTCWakeup(1);
					#else
					Thread::wait(1000);
					#endif

					if(node_state!=NODE_STATE_RX_DONE)
					{
						if(count%NODE_ACTIVE_PERIOD_IN_SEC==0)
						{
							node_state=NODE_STATE_ACTIVE;
						}
					}
					count++;

				}
				else
				{
					#if NODE_DEEP_SLEEP_MODE_SUPPORT
					nodeApiSetDevSleepRTCWakeup(NODE_ACTIVE_PERIOD_IN_SEC);
					#else
					Thread::wait(NODE_ACTIVE_PERIOD_IN_SEC*1000);
					#endif

					if(node_state!=NODE_STATE_RX_DONE)
						node_state=NODE_STATE_ACTIVE;
				}
			}
				break;
			case NODE_STATE_ACTIVE:
			{
				int i=0;
				unsigned char frame_len=0;
				char frame[64]={};

				frame_len=node_get_sensor_data(frame);

				if(frame_len==0)
				{
					node_state=NODE_STATE_LOWPOWER;
					break;
				}

				NODE_DEBUG("TX: ");

				for(i=0;i<frame_len;i++)
				{
					NODE_DEBUG("%02x ",frame[i]);
				}

				NODE_DEBUG("\n\r");

				nodeApiSendData(NODE_ACTIVE_TX_PORT, frame, frame_len);

				node_state=NODE_STATE_TX;
			}
				break;
			case NODE_STATE_TX:
				break;
			case NODE_STATE_RX_DONE:
			{
				if(node_rx_done_data.data_len!=0)
				{
					int i=0;
					int j=0;
					char print_buf[512];
					memset(print_buf, 0, sizeof(print_buf));

					NODE_DEBUG("RX: ");
					for(i=0;i<node_rx_done_data.data_len;i++)
					{
						NODE_DEBUG("%x ", node_rx_done_data.data[i]);
					}

					NODE_DEBUG("(Length: %d, Port%d)", node_rx_done_data.data_len,node_rx_done_data.data_port);

					NODE_DEBUG("\n\r");

				}
				node_state=NODE_STATE_LOWPOWER;
				break;
			}
			default:
				break;

		}
	}
}




/** @brief Main function
 */
int main ()
{
	/*Create sensor thread*/
	Thread *p_node_sensor_temp_hum_thread;

	/* Init carrier board, must be first step */
	nodeApiInitCarrierBoard();

	debug_serial.baud(115200);
	m2_serial.baud(115200);

	nodeApiInit();

	p_node_sensor_temp_hum_thread=new Thread(node_sensor_temp_hum_thread);

	node_show_version();

	/*
	 * Init configuration at beginning
	 */
	node_set_config();

	/* Apply to module */
	nodeApiApplyCfg();

	/* Start Lora */
	nodeApiStartLora();

	node_get_config();

	Thread::wait(1000);

	/*
	 *  Node state loop
	 */
	node_state_loop();

	/*Never reach here*/
	return 0;
}
