#include "modbus.h"

void modbus_routine (void );
void modbus_tick (void );
void modbus_receive_byte (uint8_t );
uint8_t is_modbus_frame_validity (void );
uint16_t modbus_calc_crc16 (uint8_t *, uint8_t );

uint32_t modbus_timeout_counter = 0;
uint8_t modbus_receive_counter = 0;

uint8_t modbus_buffer_pending_size = 0;
uint8_t modbus_buffer_index = 0;
uint8_t modbus_buffer[BUFFER_SIZE];

void modbus_routine () {
	uint8_t is_valid = 0;
	uint8_t id;
	uint8_t func;

	if (modbus.is_pending) {
		modbus_buffer_pending_size = modbus_buffer_index;

		is_valid = is_modbus_frame_validity ();
		if (is_valid) {
			id = *(modbus_buffer + 0);
			if (modbus.address != id) {
				modbus.is_pending = 0;
				modbus.status = MODBUS_STATUS_IDLE;
				modbus_buffer_index = 0;
				return;
			}

			func = *(modbus_buffer + 1);

			switch (func) {
			case 0x03:
				modbus.func_03h();
				break;

			case 0x06:
				modbus.func_06h ();
				break;

			case 0x10:
				modbus.func_10h ();
				break;

			default:
				modbus.func_not_support ();
			}

			modbus_timeout_counter = 0;
		}

		memset (modbus_buffer, 0, sizeof (BUFFER_SIZE)); //clear buffer
		modbus_buffer_index = 0;

		modbus.is_pending = 0;
		modbus.status = MODBUS_STATUS_IDLE;
	}

	//Timeout
	modbus.is_timeout = (modbus_timeout_counter < MODBUS_TIMEOUT) ? 1 : 0;
}

uint8_t is_modbus_frame_validity () {
	uint16_t crca = 0;
	uint16_t crcb = 0;

	if (modbus_buffer_pending_size < 8) {
		return 0;
	}

	//crcb = *(modbus_buffer + 7) << 8 | *(modbus_buffer + 6);
	crcb = *(modbus_buffer + (modbus_buffer_pending_size - 1)) << 8 | *(modbus_buffer + (modbus_buffer_pending_size - 2));
	crca = modbus_calc_crc16 (modbus_buffer, modbus_buffer_pending_size - 2);
	if (crca != crcb) {
		return 0;
	}

	return 1;
}

uint16_t modbus_calc_crc16 (uint8_t *data, uint8_t size) {
	uint16_t result = 0xFFFF;
	uint8_t dataOffset = 0;
	uint8_t bitOffset = 0;

	for (dataOffset = 0; dataOffset < size; dataOffset ++) {
		result ^= *(data + dataOffset);

		for (bitOffset = 0; bitOffset < 8; bitOffset ++) {
			if (result & 0x0001) {
				result >>= 1;
				result ^= 0xA001;
			} else {
				result >>= 1;
			}
		}
	}

	return result;
}

void modbus_tick () {
	if (modbus.status == MODBUS_STATUS_RECEIVING) {
		if (modbus_receive_counter) {
			modbus_receive_counter --;
		} else {
			modbus.is_pending = 1;
			modbus.status = MODBUS_STATUS_BUSY;
		}
	}

	if (modbus_timeout_counter < MODBUS_TIMEOUT) {
		modbus_timeout_counter ++;
	}
}

void modbus_receive_byte (uint8_t data) {
	switch (modbus.status) {
	case MODBUS_STATUS_IDLE:
		if (modbus_buffer_index >= BUFFER_SIZE) {
			return;
		}

		if (data == modbus.address || data == 0) { //address(0) = broadcast
		//if (data == modbus->address) {
			*(modbus_buffer + modbus_buffer_index) = data;
			modbus_buffer_index ++;
			modbus_receive_counter = modbus.t35;
			modbus.status = MODBUS_STATUS_RECEIVING;
		}
		break;

	case MODBUS_STATUS_RECEIVING:
		*(modbus_buffer + modbus_buffer_index) = data;
		modbus_buffer_index ++;
		modbus_receive_counter = modbus.t35; //update count down

		if (modbus_buffer_index >= BUFFER_SIZE) {
			//overflow protect
			return;
		}
		break;

	case MODBUS_STATUS_BUSY:
		break;

	default:
		break;
	}
}

Modbus *init_modbus () {
	Modbus *p;

	p = &modbus;
	p->is_pending = 0;
	p->is_timeout = 0;
	p->status = MODBUS_STATUS_IDLE;
	p->t35 = 3;

	p->tick = modbus_tick;
	p->routine = modbus_routine;
	p->receivce_byte = modbus_receive_byte;

	p->func_03h = modbus_func03_read_register;
	p->func_06h = modbus_func06_write_single_register;
	p->func_10h = modbus_func10_write_multipul_register;
	p->func_not_support = modbus_func10_write_multipul_register;

	memset (modbus_buffer, 0, sizeof (BUFFER_SIZE));

	return p;

}