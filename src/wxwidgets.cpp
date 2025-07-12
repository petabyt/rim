#include <stdio.h>
#include <stdlib.h>
#include <rim.h>
#include <rim_internal.h>
#include <string.h>
#include <fcntl.h>

#include <wx/wx.h>
#include <wx/app.h>
#include <wx/frame.h>

struct Priv {
	struct RimContext *ctx;
};

int is_base_control_class(uint32_t type) {
	uint32_t base[] = {
		RIM_LABEL,
		RIM_BUTTON,
		RIM_PROGRESS_BAR,
		RIM_ENTRY,
		RIM_MULTILINE_ENTRY,
		RIM_SPINBOX,
		RIM_SLIDER,
		RIM_SPINBOX,
		RIM_COMBOBOX,
		RIM_RADIO,
		RIM_DATE_PICKER,
		RIM_TAB_BAR,
		RIM_HORIZONTAL_BOX,
		RIM_VERTICAL_BOX,
		RIM_TABLE,
	};
	for (int i = 0; i < sizeof(base) / sizeof(base[0]); i++) {
		if (base[i] == type) return 1;
	}
	return 0;
}

static int get_prop_rules(void *priv, const struct WidgetHeader *w, const struct PropHeader *p) {
	int flag = 0;
	return flag;
}

static int get_widget_rules(void *priv, const struct WidgetHeader *w, const struct WidgetHeader *parent) {
	int flag = 0;
	return flag;
}

static int rim_backend_remove(void *priv, struct WidgetHeader *w, struct WidgetHeader *parent) {
	rim_abort("TODO\n");
	return 1;
}

static int rim_backend_destroy(void *priv, struct WidgetHeader *w) {
	rim_abort("TODO\n");
	return 1;
}

static int rim_backend_create(void *priv, struct WidgetHeader *w) {
	rim_abort("TODO\n");
	return 1;
}

static int rim_backend_update_id(void *priv, struct WidgetHeader *w) {
	rim_abort("TODO\n");
	return 1;
}

static int rim_backend_append(void *priv, struct WidgetHeader *w, struct WidgetHeader *parent) {
	rim_abort("TODO\n");
	return 1;
}

static int rim_backend_tweak(void *priv, struct WidgetHeader *w, struct PropHeader *prop, enum RimPropTrigger type) {
	rim_abort("TODO\n");
	return 1;
}

int rim_backend_run(struct RimContext *ctx, rim_on_run_callback *callback, void *arg) {
	rim_abort("TODO\n");
	return 0;
}

static void rim_backend_close(void *priv) {
	
}

wxDECLARE_EVENT(LIBUI_QUEUE_FUNC, wxCommandEvent);

class MyCustomEvent : public wxCommandEvent {
	wxDECLARE_DYNAMIC_CLASS(MyCustomEvent);
public:
	MyCustomEvent(wxEventType type = LIBUI_QUEUE_FUNC, int id = 0)
		: wxCommandEvent(type, id), m_data(0) {}

	MyCustomEvent(const MyCustomEvent& e)
		: wxCommandEvent(e), m_data(e.m_data) {}

	wxEvent* Clone() const override { return new MyCustomEvent(*this); }

	void SetData(int d) { m_data = d; }
	int GetData() const { return m_data; }

private:
	int m_data;
};

wxDEFINE_EVENT(LIBUI_QUEUE_FUNC, wxCommandEvent);
wxIMPLEMENT_DYNAMIC_CLASS(MyCustomEvent, wxCommandEvent);

class MyApp : public wxApp {
public:
	bool OnInit() override {
		return true;
	}
};

wxAppConsole *wxCreateApp() {
	return new MyApp();
}

wxAppInitializer wxTheAppInitializer((wxAppInitializerFunction)wxCreateApp);

void rim_backend_start(struct RimContext *ctx, sem_t *done) {
	struct Priv *p = (struct Priv *)malloc(sizeof(struct Priv));
	ctx->backend.priv = (void *)p;
	ctx->backend.ext_id = 2;
	ctx->backend.create = rim_backend_create;
	ctx->backend.tweak = rim_backend_tweak;
	ctx->backend.append = rim_backend_append;
	ctx->backend.remove = rim_backend_remove;
	ctx->backend.destroy = rim_backend_destroy;
	ctx->backend.close = rim_backend_close;
	ctx->backend.update_onclick = rim_backend_update_id;
	ctx->backend.get_prop_rules = get_prop_rules;
	ctx->backend.get_widget_rules = get_widget_rules;
	// ...

	int argc = 0;
	wxEntry(argc, (char **){});
}
