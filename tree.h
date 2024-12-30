#include <stdint.h>

struct __attribute__((packed)) String {
	// length of this data chunk
	uint32_t length;
	// null terminated string
	char s[];
};

struct __attribute__((packed)) WidgetPropertyHeader {
	uint32_t length;
	uint32_t type;
	union un {
		struct __attribute__((packed)) Slider {
			int32_t value;
		}slider;
		struct __attribute__((packed)) Checkbox {
			uint32_t checked;
		}checkbox;
		struct __attribute__((packed)) Button {
			struct String str;
		}button;
		struct __attribute__((packed)) Label {
			struct String str;
		}label;
		struct __attribute__((packed)) Layout {
			uint32_t direction;
		}layout;
	}u;
};

struct __attribute__((packed)) WidgetHeader {
	uint32_t type;
	uint32_t unique_id;
	uintptr_t os_handle;
	uint32_t n_children;
	uint8_t data[];
	// properties start here
	// children start here
};
