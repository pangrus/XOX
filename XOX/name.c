#include "usb_names.h"

#define MIDI_NAME     { 'P','a','n','g','r','u','s',' ','X','O','X'}
#define MIDI_NAME_LEN 11

struct usb_string_descriptor_struct usb_string_product_name = {
	2 + MIDI_NAME_LEN * 2,
	3,
	MIDI_NAME
};
