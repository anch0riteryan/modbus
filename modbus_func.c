#include "modbus.h"

extern uint8_t modbus_buffer_pending_size;
extern uint8_t modbus_buffer[BUFFER_SIZE];

void modbus_func03_read_register () {
	uint8_t id = *(modbus_buffer + 0);
	uint16_t reg = *(modbus_buffer + 2) << 8 | *(modbus_buffer + 3);
	uint16_t size = *(modbus_buffer + 4) << 8 | *(modbus_buffer + 5);

	uint8_t response[BUFFER_SIZE];

	uint16_t value;
	uint16_t crc;
	uint8_t index;

	if (id == 0) { //func 03h do not response boardcast
		return;
	}

	if (size > 120) {
		return;
	}

	*(response + 0) = id;
	*(response + 1) = 0x03;
	*(response + 2) = size << 1; //size x 2 (1 word = 2 bytes)

	for (index = 0; index < size; index++) {
		value = modbus_port_get_register_value (reg + index);
		*(response + 3 + (index << 1)) = (value >> 8) & 0xFF;
		*(response + 4 + (index << 1)) = value & 0xFF;
	}

	crc = modbus_calc_crc16 (response, 3 + (size << 1));
	*(response + 3 + (size << 1)) = crc & 0xFF;
	*(response + 4 + (size << 1)) = (crc >> 8) & 0xFF;

	modbus.transmit_bytes (response, 5 + (size << 1));
}

void modbus_func06_write_single_register () {
	uint8_t id = *(modbus_buffer + 0); //0=>no reply
	uint16_t reg = *(modbus_buffer + 2) << 8 | *(modbus_buffer + 3);
	uint16_t value = *(modbus_buffer + 4) << 8 | *(modbus_buffer + 5);

	uint8_t response_size = 8; //fixed
	uint8_t response[8];
	uint16_t crc = 0;

	//uint16_t result = 0;

	if (modbus_port_is_register_writable (reg, value)) {
		modbus_port_set_register_value (reg, value);
	} else {
		modbus_port_get_register_value (reg);
	}

	if (id != 0) {
		*(response + 0) = id;
		*(response + 1) = 0x06; //func
		*(response + 2) = (reg >> 8) & 0xFF;
		*(response + 3) = reg & 0xFF;

		//value or result?
		*(response + 4) = (value >> 8) & 0xFF;
		*(response + 5) = value & 0xFF;
		//

		crc = modbus_calc_crc16 (response, 6);
		*(response + 6) = crc & 0xFF;
		*(response + 7) = (crc >> 8) & 0xFF;

		modbus.transmit_bytes (response, response_size); //
	}

	modbus_port_on_process_frame_done ();
}

void modbus_func10_write_multipul_register () {
	uint8_t id = *(modbus_buffer + 0);
	uint16_t reg = *(modbus_buffer + 2) << 8 | *(modbus_buffer + 3);
	uint16_t cnt = *(modbus_buffer + 4) << 8 | *(modbus_buffer + 5);
	uint16_t data_size = *(modbus_buffer + 6); //size in byte
	uint16_t value;
	//uint16_t result;
	uint16_t success = 0;
	uint16_t crc;
	uint8_t response[BUFFER_SIZE];

	//check buffer size
	if ((cnt << 1) != data_size) {
		return;
	}

	//check protocal limit
	if (reg + cnt > 250) {
		return;
	}

	//check register size 64 reg = 128 byte
	if (cnt > 64) {
		return;
	}

	for (uint8_t index = 0; index < cnt; index++) {
		value = *(modbus_buffer + 7 + (index << 1)) | *(modbus_buffer + 8 + (index << 1));
		if (modbus_port_is_register_writable (reg, value)) {
			modbus_port_set_register_value (reg, value);
			success ++;
		} else {
			modbus_port_get_register_value (reg);
		}
	}

	if (id != 0) {
		*(response + 0) = id;
		*(response + 1) = 0x10; //func
		*(response + 2) = (reg >> 8) & 0xFF;
		*(response + 3) = reg & 0xFF;

		//value or result?
		*(response + 4) = (success >> 8) & 0xFF;
		*(response + 5) = success & 0xFF;
		//

		crc = modbus_calc_crc16 (response, 6);
		*(response + 6) = crc & 0xFF;
		*(response + 7) = (crc >> 8) & 0xFF;

		modbus.transmit_bytes (response, 7); //
	}

	modbus_port_on_process_frame_done ();
}

void modbus_func_not_support () {
	modbus_port_func_not_support ();
}