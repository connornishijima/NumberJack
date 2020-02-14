#include <NumberJack.h>

NumberJack::NumberJack(){
}

void NumberJack::begin(uint32_t baud_rate, const uint8_t max_v){
	
	// Dynamic allocation is bad.
	// 
	// ...if you do it often.
	// These are allocated before anything else since
	// jack.begin() is likely your first line of code
	// in setup();
	//
	var_pointers = new const void*[max_v*3];
	last_values = new char[16*max_v];
	var_types = new uint8_t[max_v];
	var_trigger = new bool[max_v];
	var_category_ids = new uint8_t[max_v];
	last_reports = new uint32_t[max_v];
	
	max_vars = max_v;
	
	Serial.begin(baud_rate);
	Serial.println();
	Serial.println(F("$NBOOT"));
}

void NumberJack::update(){
	t_now = millis();
	
	if(!map_sent){
		map_sent = true;
		build_map();
	}
		
	// CHECK SERIAL
	while (Serial.available() > 0) {
		char c = Serial.read();
		if(c == '\n'){
			char* head = substr(serial_buffer, 0, 2);
			if(strcmp(head,"$N") == 0){
				parse_command(serial_buffer);
			}
			free(head);
			
			for(uint8_t s = 0; s < 64; s++){
				serial_buffer[s] = '\0';
			}
			serial_buffer_index = 0;
		}
		else{
			serial_buffer[serial_buffer_index] = c;
			serial_buffer_index++;
			if(serial_buffer_index >= 64){
				serial_buffer_index = 0;
			}
		}
	}
	
	for(uint8_t i = 0; i < var_pointer_index; i++){
		if(var_trigger[i]){
			char current[33];
			
			if(     var_types[i] == t_int8_t){   int8_t value   = *(int8_t   *)var_pointers[(i*3)+0]; sprintf(current, "%l", value); }
			else if(var_types[i] == t_int16_t){  int16_t value  = *(int16_t  *)var_pointers[(i*3)+0]; sprintf(current, "%l", value); }
			else if(var_types[i] == t_int32_t){  int32_t value  = *(int32_t  *)var_pointers[(i*3)+0]; sprintf(current, "%l", value); }
			
			else if(var_types[i] == t_uint8_t){  uint8_t value  = *(uint8_t  *)var_pointers[(i*3)+0]; sprintf(current, "%lu", value); }
			else if(var_types[i] == t_uint16_t){ uint16_t value = *(uint16_t *)var_pointers[(i*3)+0]; sprintf(current, "%lu", value); }
			else if(var_types[i] == t_uint32_t){ uint32_t value = *(uint32_t *)var_pointers[(i*3)+0]; sprintf(current, "%lu", value); }
			
			else if(var_types[i] == t_bool){     bool value     = *(bool     *)var_pointers[(i*3)+0]; sprintf(current, "%lu", value); }
			
			else if(var_types[i] == t_float){    float value    = *(float    *)var_pointers[(i*3)+0]; sprintf(current, "%f", value); }
			else if(var_types[i] == t_double){   double value   = *(double   *)var_pointers[(i*3)+0]; sprintf(current, "%d", value); }
			
			else if(var_types[i] == t_char){     char value     = *(char     *)var_pointers[(i*3)+0]; sprintf(current, "%c", value); }
			else if(var_types[i] == t_char_arr){ memcpy(current, *(char* *)var_pointers[(i*3)+0],16); }
			
			const uint8_t str_len = strlen(current);
			
			char new_val[str_len];
			for(uint8_t c = 0; c < 16; c++){				
				if(c < str_len){
					new_val[c] = current[c];
				}
				else{
					new_val[c] = '\0';
				}
			}
			
			char old_val[16];
			for(uint8_t c = 0; c < 16; c++){
				old_val[c] = last_values[(i*16)+c];
			}
						
			if(strcmp(new_val,old_val) != 0){
				send_var(i);
				for(uint8_t c = 0; c < 16; c++){
					if(c < str_len){
						last_values[(i*16)+c] = new_val[c];
					}
					else{
						last_values[(i*16)+c] = '\0';
					}
				}
			}
		}
	}
}

void NumberJack::parse_command(char* command) {
	get_sub_value(command, '|', 0);
	char* head = substr(command,sub_start,sub_end);
	Serial.println(head);
	if (strcmp_P(head, PSTR("$NP?")) == 0) { // NumberJack Present?
		Serial.println(F("$NP!"));
		map_sent = false;
	}
	else if (strcmp_P(head, PSTR("$NRESET")) == 0) { // Reset device!
		#ifdef ESP8266
			ESP.reset();
		#else
			wdt_enable(WDTO_15MS); // WATCHDOG TIMEOUT 15 MS
			while (1) {
				//WAIT WAY LONGER THAN 15 MS AND WE'll RESET AUTOMATICALLY
			}
		#endif
	}
	else if (strcmp_P(head, PSTR("$NRM")) == 0) { // REQUEST MAP
		build_map();
	}
	else if (strcmp_P(head, PSTR("$NRD")) == 0) { // REQUEST DUMP
		send_all();
	}
	else if (strcmp_P(head, PSTR("$NRT")) == 0) { // REQUEST TRIGGERS
		send_triggers();
	}
	else if (strcmp_P(head, PSTR("$NRV")) == 0) { // REQUEST VARIABLE
		get_sub_value(command, '|', 1);
		char* sub1 = substr(command,sub_start,sub_end);
		uint8_t index = strtoul(sub1, NULL, 0);
		free(sub1);
		send_var(index);
	}
	else if (strcmp_P(head, PSTR("$NSV")) == 0) { // SET VARIABLE
		get_sub_value(command, '|', 1);
		char* sub1 = substr(command,sub_start,sub_end);
		get_sub_value(command, '|', 2);
		char* sub2 = substr(command,sub_start,sub_end);
		
		uint8_t index = strtoul(sub1, NULL, 0);
		uint8_t data_type = var_types[index];
		if(data_type == t_int8_t){
			int8_t new_value = strtol(sub2, NULL, 0);
			set_var(index, new_value);
		}
		else if(data_type == t_int16_t){
			int16_t new_value = strtol(sub2, NULL, 0);
			set_var(index, new_value);
		}
		else if(data_type == t_int32_t){
			int32_t new_value = strtol(sub2, NULL, 0);
			set_var(index, new_value);
		}
		
		else if(data_type == t_uint8_t){
			uint8_t new_value = strtoul(sub2, NULL, 0);
			set_var(index, new_value);
		}
		else if(data_type == t_uint16_t){
			uint16_t new_value = strtoul(sub2, NULL, 0);
			set_var(index, new_value);
		}
		else if(data_type == t_uint32_t){
			uint32_t new_value = strtoul(sub2, NULL, 0);
			set_var(index, new_value);
		}
		
		else if(data_type == t_bool){
			bool new_value = strtoul(sub2, NULL, 0);
			set_var(index, new_value);
		}
		
		else if(data_type == t_float){
			float new_value = strtod(sub2, NULL);
			set_var(index, new_value);
		}
		else if(data_type == t_double){
			double new_value = strtod(sub2, NULL);
			set_var(index, new_value);
		}
		
		else if(data_type == t_char_arr){
			set_var(index, sub2);
		}
		free(sub1);
		free(sub2);
	}
	else if (strcmp_P(head, PSTR("$NST")) == 0) { // SET TRIGGER
		get_sub_value(command, '|', 1);
		char* sub1 = substr(command,sub_start,sub_end);
		get_sub_value(command, '|', 2);
		char* sub2 = substr(command,sub_start,sub_end);

		uint8_t index = strtoul(sub1, NULL, 0);
		uint8_t trig = strtoul(sub2, NULL, 0);
		free(sub1);
		free(sub2);
		var_trigger[index] = trig;
		send_triggers();
	}
	
	free(head);
}

void NumberJack::get_sub_value(char* data, char separator, int index) {
	uint8_t data_length = strlen(data);
	
	int found = 0;
	int strIndex[] = { 0, -1 };
	int maxIndex = data_length - 1;

	for (int i = 0; i <= maxIndex && found <= index; i++) {
		if (data[i] == separator || i == maxIndex) {
			found++;
			strIndex[0] = strIndex[1] + 1;
			strIndex[1] = (i == maxIndex) ? i + 1 : i;
		}
	}
	uint8_t delta = strIndex[1]-strIndex[0];
	char* empty = "";
	
	sub_start = strIndex[0];
	sub_end = strIndex[0]+delta;
}

void NumberJack::track(const void* var, uint8_t data_type, const void* var_name, const void* var_category, bool trigger){
	var_types[var_pointer_index] = data_type;
	var_trigger[var_pointer_index] = trigger;
	
	var_pointers[(var_pointer_index*3)+0] = var;
	var_pointers[(var_pointer_index*3)+1] = var_name;
	var_pointers[(var_pointer_index*3)+2] = var_category;
	
	var_pointer_index++;
}

void NumberJack::set_var(uint8_t index, int8_t value){   *(int8_t   *)var_pointers[(index*3)+0] = value;}
void NumberJack::set_var(uint8_t index, int16_t value){  *(int16_t  *)var_pointers[(index*3)+0] = value;}
void NumberJack::set_var(uint8_t index, int32_t value){  *(int32_t  *)var_pointers[(index*3)+0] = value;}

void NumberJack::set_var(uint8_t index, uint8_t value){  *(uint8_t  *)var_pointers[(index*3)+0] = value;}
void NumberJack::set_var(uint8_t index, uint16_t value){ *(uint16_t *)var_pointers[(index*3)+0] = value;}
void NumberJack::set_var(uint8_t index, uint32_t value){ *(uint32_t *)var_pointers[(index*3)+0] = value;}

void NumberJack::set_var(uint8_t index, bool value){     *(bool     *)var_pointers[(index*3)+0] = value;}

void NumberJack::set_var(uint8_t index, float value){    *(float    *)var_pointers[(index*3)+0] = value;}
void NumberJack::set_var(uint8_t index, double value){   *(double   *)var_pointers[(index*3)+0] = value;}

void NumberJack::set_var(uint8_t index, char value){     *(char     *)var_pointers[(index*3)+0] = value;}
void NumberJack::set_var(uint8_t index, char* value){    *(char*    *)var_pointers[(index*3)+0] = value;}


void NumberJack::send_var(uint8_t index){
	if(t_now - last_reports[index] >= min_report_ms){
		Serial.print(F("$NV|"));
		Serial.print(var_category_ids[index]);
		Serial.print('|');
		Serial.print(index);
		Serial.print('|');
		
		if(     var_types[index] == t_int8_t)  { Serial.println(  *(int8_t *)var_pointers[(index*3)+0]);}
		else if(var_types[index] == t_int16_t) { Serial.println( *(int16_t *)var_pointers[(index*3)+0]);}
		else if(var_types[index] == t_int32_t) { Serial.println( *(int32_t *)var_pointers[(index*3)+0]);}
		
		else if(var_types[index] == t_uint8_t) { Serial.println( *(uint8_t *)var_pointers[(index*3)+0]);}
		else if(var_types[index] == t_uint16_t){ Serial.println(*(uint16_t *)var_pointers[(index*3)+0]);}
		else if(var_types[index] == t_uint32_t){ Serial.println(*(uint32_t *)var_pointers[(index*3)+0]);}
		
		else if(var_types[index] == t_bool){     Serial.println(    *(bool *)var_pointers[(index*3)+0]);}
		
		else if(var_types[index] == t_float){    Serial.println(   *(float *)var_pointers[(index*3)+0],4);}
		else if(var_types[index] == t_double){   Serial.println(  *(double *)var_pointers[(index*3)+0],4);}
		
		else if(var_types[index] == t_char){     Serial.println(    *(char *)var_pointers[(index*3)+0]);}
		else if(var_types[index] == t_char_arr){ Serial.println(   *(char* *)var_pointers[(index*3)+0]);}
		last_reports[index] = t_now;
	}
	
	/*
	for(uint16_t i = 0; i < 16*var_pointer_index; i++){
		char c = (char)last_values[i];
		if(c >= 32 && c < 127){
			Serial.print(c);
		}
		else{
			Serial.print(' ');
		}
	}
	Serial.println();
	*/
}

void NumberJack::send_var(char* name){
	for(uint8_t i = 0; i < var_pointer_index; i++){
		const char* pointer_name = (char*)var_pointers[(i*3)+1];
		if(strcmp(pointer_name, name) == 0){
			send_var(i);
		}
	}
}

void NumberJack::send_all(){
	for(uint8_t i = 0; i < var_pointer_index; i++){
		send_var(i);
	}
}

void NumberJack::send_triggers(){
	Serial.print(F("$NT|"));
	for(uint8_t i = 0; i < var_pointer_index; i++){
		Serial.print(var_trigger[i]);
	}
	Serial.println();
}

void NumberJack::build_map(){
	uint8_t cat_count = 0;
	uint8_t cat_traveled = 0;
	
	while(cat_traveled < var_pointer_index){
		for(uint8_t i = 0; i < (cat_traveled+1); i++){
			char* current_cat = (char*)var_pointers[(i*3)+2];
			
			bool cat_found = false;
			uint8_t old_cat_id = 0;
			for(uint8_t c = 0; c < (cat_traveled); c++){
				char* this_cat = (char*)var_pointers[(c*3)+2];
				if(strcmp(current_cat,this_cat) == 0){
					cat_found = true;
					old_cat_id = var_category_ids[c];
				}
			}
			
			if(cat_found){
				var_category_ids[i] = old_cat_id;
			}
			else{
				var_category_ids[i] = cat_count;
				cat_count++;
			}
			
		}
		cat_traveled += 1;
	}
	send_map();
	send_version();
}

void NumberJack::send_map(){
	Serial.print(F("$NM|"));
	
	for(uint8_t i = 0; i < var_pointer_index; i++){
		char* current_cat = (char*)var_pointers[(i*3)+2];
		
		bool string_found = false;
		for(uint8_t c = 0; c < i; c++){
			char* this_cat = (char*)var_pointers[(c*3)+2];
			if(strcmp(current_cat,this_cat) == 0){
				string_found = true;
			}
		}
		if(!string_found){
			if(i != 0){
				Serial.print(',');
			}
			Serial.print(current_cat);
			Serial.print('=');
			Serial.print(var_category_ids[i]);
		}
	}
	Serial.print("|");
	for(uint8_t i = 0; i < var_pointer_index; i++){
		Serial.print((char*)var_pointers[(i*3)+1]);
		Serial.print('=');
		Serial.print(i);
		if(i != var_pointer_index-1){
			Serial.print(',');
		}
	}
	Serial.print("|");
	for(uint8_t i = 0; i < var_pointer_index; i++){
		Serial.print(var_trigger[i]);
	}
	Serial.println();
}

void NumberJack::send_version(){
	Serial.print(F("$NLV|")); // Library Version
	Serial.println(NJ_VERSION);
}

void NumberJack::info(char* output){
	Serial.print(F("$NHI|"));
	Serial.print(output);
	Serial.println();
}

void NumberJack::warn(char* output){
	Serial.print(F("$NHW|"));
	Serial.print(output);
	Serial.println();
}

void NumberJack::error(char* output){
	Serial.print(F("$NHE|"));
	Serial.print(output);
	Serial.println();
}

char* NumberJack::substr(const char *src, int m, int n){
	// get length of the destination string
	int len = n - m;

	// allocate (len + 1) chars for destination (+1 for extra null character)
	char *dest = (char*)malloc(sizeof(char) * (len + 1));

	// extracts characters between m'th and n'th index from source string
	// and copy them into the destination string
	for (int i = m; i < n && (*src != '\0'); i++)
	{
		*dest = *(src + i);
		dest++;
	}

	// null-terminate the destination string
	*dest = '\0';

	// return the destination string
	return dest - len;
}

uint8_t NumberJack::manual_strcmp(char* str_a, char* str_b, uint8_t len){
	uint8_t mismatched = 0;
	for(uint8_t i = 0; i < len; i++){
		if(str_a[i] != str_b[i]){
			mismatched++;
		}
	}
	return mismatched;
}


int16_t NumberJack::strcmp_limited(char* str_a, char* str_b){
	uint8_t str_a_len = strlen(str_a);
	uint8_t str_b_len = strlen(str_b);
	
	bool a_greater = true;
	if(str_b_len > str_a_len){
		a_greater = false;
	}
	
	if(a_greater){
		return strncmp(str_a,str_b, str_a_len);
	}
	else{
		return strncmp(str_a,str_b, str_b_len);
	}
}