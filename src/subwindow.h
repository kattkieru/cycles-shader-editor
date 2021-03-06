#pragma once

#include <string>

#include "point2.h"

struct NVGcontext;

namespace CyclesShaderEditor {

	class NodeEditorSubwindow {
	public:
		NodeEditorSubwindow(Point2 screen_position, std::string title);
		virtual ~NodeEditorSubwindow() {}

		virtual Point2 get_screen_pos() const;
		virtual float get_width() const;
		virtual float get_height() const;

		virtual void pre_draw();
		// Draws the window at (0, 0). Can freely modify nanovg state without nvgSave/nvgRestore.
		virtual void draw(NVGcontext* draw_context);
		virtual bool is_mouse_over() const;
		virtual bool is_mouse_over_header() const;
		virtual void set_mouse_position(Point2 screen_position, float max_pos_y);

		virtual void handle_mouse_button(int button, int action, int mods);
		virtual void handle_key(int key, int scancode, int action, int mods);
		virtual void handle_character(unsigned int codepoint);

		virtual void move_window_begin();
		virtual void move_window_end();

		virtual bool is_active() const { return true; }

	protected:
		virtual void draw_content(NVGcontext* draw_context) = 0;

		std::string title = "Subwindow";
		Point2 subwindow_screen_pos;

		Point2 mouse_local_pos;
		Point2 mouse_local_begin_move_pos;
		Point2 mouse_panel_pos;

		float subwindow_width = 100.0f;
		float content_height = 1.0f;

		bool subwindow_moving = false;
	};

}
