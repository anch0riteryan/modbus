#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MODBUS_STATUS_IDLE 0
#define MODBUS_STATUS_RECEIVING 1
#define MODBUS_STATUS_BUSY 2

#define MODBUS_TIMEOUT 3000 //3ms
#define MODBUS_NO_SUPPORT_REGISTER_VALUE 0xFFFF
#define BUFFER_SIZE 255

#define MODBUS_FUNC_READ_COIL_STATUS 0x01 //DO
#define MODBUS_FUNC_READ_INPUT_STATUS 0x02 //DI
#define MODBUS_FUNC_READ_HOLDING_REGISTER 0x03 //讀REG
#define MODBUS_FUNC_READ_INPUT_REGISTER 0x04
#define MODBUS_FUNC_FORCE_SINGLE_COIL 0x05
#define MODBUS_FUNC_PRESET_SINGLE_REGISTER 0x06 //寫單個REG
#define MODBUS_FUNC_FORCE_MULTIPLE_COILS 0x0F
#define MODBUS_FUNC_PRESET_MULTIPLE_REGISTERS 0x10 //寫多個REG連續

typedef struct _modbus {
	uint8_t is_pending;
	uint8_t is_timeout;

	uint8_t address;

	uint8_t status;
	uint8_t t35;

	void (*delay) (uint32_t );
	void (*tick) (void ); //1KHz
	void (*routine) (void ); // <-- main loop
	void (*receivce_byte) (uint8_t ); // <-- usart isr

	//PORT FUNC. CODE(由外部porting專案交付func.ptr)
	void (*transmit_bytes) (uint8_t *, uint8_t);

	//implement function codes
	void (*func_03h) (void ); //read holding register
	void (*func_06h) (void ); //write single register
	void (*func_10h) (void ); //write multipul register
	void (*func_not_support) (void );
} Modbus;
extern Modbus modbus;

Modbus *init_modbus (void );
uint16_t modbus_calc_crc16 (uint8_t *data, uint8_t size);

void modbus_func03_read_register (void );
void modbus_func06_write_single_register (void );
void modbus_func10_write_multipul_register (void );
void modbus_func_not_support (void );

//ModbusPort.c (USER APPLICATION IMPLEMENTS)
uint16_t modbus_port_get_register_value (uint16_t reg);
uint16_t modbus_port_set_register_value (uint16_t reg, uint16_t value);
uint8_t modbus_port_is_register_writable (uint16_t reg, uint16_t value); //檢查當下狀態與數值是否可寫入
void modbus_port_on_process_frame_done (void ); //Callback
void modbus_port_func_not_support (void );
