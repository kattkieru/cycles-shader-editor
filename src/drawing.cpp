#include "drawing.h"

#include <nanovg.h>

#include "gui_sizes.h"

void CyclesShaderEditor::Drawing::draw_button(NVGcontext* draw_context, float x_pos, float y_pos, float width, float height, std::string label, bool enabled, bool pressed)
{
	const int gradient_alpha = enabled ? 31 : 15;
	const int button_alpha = enabled ? 127 : 63;
	const int label_alpha = enabled ? 255 : 127;

	const unsigned char start_color = pressed ? 0 : 255;
	const unsigned char end_color = pressed ? 127 : 15;

	NVGpaint button_bg = nvgLinearGradient(
		draw_context,
		x_pos,
		y_pos,
		x_pos,
		y_pos + height,
		nvgRGBA(start_color, start_color, start_color, gradient_alpha),
		nvgRGBA(end_color, end_color, end_color, gradient_alpha)
	);

	// Draw button
	nvgBeginPath(draw_context);
	nvgRoundedRect(draw_context,
		x_pos,
		y_pos,
		width,
		height,
		UI_BUTTON_CORNER_RADIUS);
	nvgFillPaint(draw_context, button_bg);
	nvgFill(draw_context);
	nvgStrokeColor(draw_context, nvgRGBA(0, 0, 0, button_alpha));
	nvgStrokeWidth(draw_context, 1.0f);
	nvgStroke(draw_context);

	// Button label
	nvgFontSize(draw_context, UI_FONT_SIZE_NORMAL);
	nvgFontFace(draw_context, "sans");
	nvgTextAlign(draw_context, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
	nvgFontBlur(draw_context, 0.0f);
	nvgFillColor(draw_context, nvgRGBA(0, 0, 0, label_alpha));
	nvgText(draw_context, x_pos + width / 2, y_pos + height / 2, label.c_str(), NULL);
}

void CyclesShaderEditor::Drawing::draw_color_pick_cursor(NVGcontext* draw_context, float x_pos, float y_pos)
{
	const float CURSOR_LENGTH_SHORT = 3.0f;
	const float CURSOR_LENGTH_LONG = 8.0f;

	nvgBeginPath(draw_context);

	const float left_x = x_pos - CURSOR_LENGTH_SHORT - CURSOR_LENGTH_LONG;
	const float left_w = CURSOR_LENGTH_LONG;
	const float left_y = y_pos - CURSOR_LENGTH_SHORT / 2;
	const float left_h = CURSOR_LENGTH_SHORT;
	nvgRect(draw_context, left_x, left_y, left_w, left_h);

	const float right_x = x_pos + CURSOR_LENGTH_SHORT;
	const float right_w = CURSOR_LENGTH_LONG;
	const float right_y = y_pos - CURSOR_LENGTH_SHORT / 2;
	const float right_h = CURSOR_LENGTH_SHORT;
	nvgRect(draw_context, right_x, right_y, right_w, right_h);

	const float top_x = x_pos - CURSOR_LENGTH_SHORT / 2;
	const float top_w = CURSOR_LENGTH_SHORT;
	const float top_y = y_pos - CURSOR_LENGTH_SHORT - CURSOR_LENGTH_LONG;
	const float top_h = CURSOR_LENGTH_LONG;
	nvgRect(draw_context, top_x, top_y, top_w, top_h);

	const float bot_x = x_pos - CURSOR_LENGTH_SHORT / 2;
	const float bot_w = CURSOR_LENGTH_SHORT;
	const float bot_y = y_pos + CURSOR_LENGTH_SHORT;
	const float bot_h = CURSOR_LENGTH_LONG;
	nvgRect(draw_context, bot_x, bot_y, bot_w, bot_h);

	nvgFillColor(draw_context, nvgRGBAf(0.0f, 0.0f, 0.0f, 1.0f));
	nvgFill(draw_context);
	nvgStrokeColor(draw_context, nvgRGBAf(1.0f, 1.0f, 1.0f, 1.0f));
	nvgStrokeWidth(draw_context, 1.0f);
	nvgStroke(draw_context);
}
