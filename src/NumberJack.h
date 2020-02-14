#ifndef _numberjack_h
#define _numberjack_h

#include <Arduino.h>

#define NJ_VERSION 10100 // Version 1.1.0

#ifndef ESP8266 || ESP32
	#include <avr/wdt.h>
	#include <string.h>
#endif

// signed integers
#define t_int8_t         1
#define t_int16_t        2
#define t_int32_t        3

// unsigned integers
#define t_uint8_t        4
#define t_uint16_t       5
#define t_uint32_t       6

// bool
#define t_bool           7

// floats
#define t_float          8
#define t_double         9

// chars
#define t_char           10
#define t_char_arr       11

#define TRIGGERED true

class NumberJack{
	public:
		NumberJack();
		void begin(uint32_t baud_rate, const uint8_t max_v);
		void update();
		
		void track(const void* var, uint8_t data_type, const void* var_name, const void* var_category, bool trigger = false);
		void send_var(uint8_t index);
		void send_var(char* name);
		void send_all();
		void send_triggers();
		void set_var(uint8_t index, int8_t value);
		void set_var(uint8_t index, int16_t value);
		void set_var(uint8_t index, int32_t value);
		void set_var(uint8_t index, uint8_t value);
		void set_var(uint8_t index, uint16_t value);
		void set_var(uint8_t index, uint32_t value);
		void set_var(uint8_t index, bool value);
		void set_var(uint8_t index, float value);
		void set_var(uint8_t index, double value);
		void set_var(uint8_t index, char value);
		void set_var(uint8_t index, char* value);
		void build_map();
		void send_map();
		void send_version();
		void parse_command(char* command);
		void get_sub_value(char* data, char separator, int index);
		
		void info(char* output);
		void warn(char* output);
		void error(char* output);
		
		char* substr(const char *src, int m, int n);
		uint8_t manual_strcmp(char* str_a, char* str_b, uint8_t len);
			
		int16_t strcmp_limited(char* str_a, char* str_b);
		
	private:
		static const uint8_t max_report_hz = 20;
		
		const void* *var_pointers;
		char *last_values;
		uint8_t *var_types;
		bool *var_trigger;
		uint8_t *var_category_ids;
		uint32_t *last_reports;
		
		uint8_t var_pointer_index = 0;
		uint8_t max_vars = 0;
		
		char serial_buffer[64];
		uint8_t serial_buffer_index = 0;
		uint8_t sub_start = 0;
		uint8_t sub_end = 0;
		
		static const uint32_t min_report_ms = (1000/max_report_hz);
		uint32_t t_now = 0;
		bool map_sent = false;
};

#endif