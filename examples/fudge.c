#include <stdio.h>
#include <rim.h>

struct State {
	int window_open;
	int is_camera_connected;
	// TODO: Make this the main window title
	char connected_camera_name[32];

	struct PtpDeviceEntry *camlib_entries;

	char *ui_log_buffer;
	size_t ui_log_pos;
	size_t ui_log_length;

	int filmsim;
	int expobias;
	int color;
	int sharpness;
	int graineffect;
	int whitebalance;

	char backup_file_path[512];
	char raf_path[512];
	char output_jpg_path[512];
	char fp_xml_path[512];
};

static int im_entry2(const char *label, char *buffer, unsigned int size) {
	int rc = 0;
	im_set_next_gap(1);
	if (im_begin_hbox()) {
		im_label(label);
		im_set_next_expand();
		im_entry(label, buffer, size);
		im_end_hbox();
	}
	return 0;
}

static void im_entry3(const char *label, char *buffer, unsigned int size) {
	if (im_begin_form_entry(label)) {
		if (im_begin_hbox()) {
			im_set_next_expand();
			im_entry("", buffer, size);
			im_button("Browse");
			im_end_hbox();
		}
		im_end_form_entry();
	}
}

int mymain(struct RimContext *ctx, void *arg) {
	struct State backend_state = {0};
	struct State *state = &backend_state;
	int show_more = 0;
	int selected = 1;
	int n_options = 4;
	while (rim_poll(ctx)) {
		im_set_next_margin(1);
		if (im_begin_window("Fudge", 1000, 500)) {
			if (im_begin_tab_bar(&selected)) {
				im_set_next_margin(1);
				im_set_next_gap(1);
				if (im_begin_tab("Backup/Restore")) {
					im_set_next_gap(1);
					if (im_begin_form()) {
						im_entry3("Backup file path", state->backup_file_path, sizeof(state->backup_file_path));
						im_end_form();
					}
					if (im_button("Download backup to file")) {
					}
					if (im_button("Restore backup from file")) {
					}
					im_end_tab();
				}
				im_set_next_gap(1);
				im_set_next_margin(1);
				if (im_begin_tab("Raw Conversion")) {
					im_set_next_gap(1);
					if (im_begin_form()) {
						im_entry3("Input RAF path", state->raf_path, sizeof(state->raf_path));
						im_entry3("Output JPG path", state->output_jpg_path, sizeof(state->output_jpg_path));
						im_entry3("FP1/FP2/FP3 path", state->fp_xml_path, sizeof(state->fp_xml_path));
						im_end_form();
					}
					if (im_button("Connect to a camera and convert RAW")) {
					}
					im_end_tab();
				}
				if (im_begin_tab("About")) {
					im_label("Licenses:");
					im_label("libusb-1.0 (LGPL v2.1)");
					im_label("dear imgui (MIT)");
					im_label("hello_imgui (MIT)");
					im_label("Copyright (C) 2023 Fudge by Daniel C");
					im_label("Compile date: " __DATE__);
					im_end_tab();
				}
				im_end_tab_bar();
			}
			im_end_window();
		}
	}

	return 0;
}
int main(void) {
    return rim_start(mymain, NULL);
}
