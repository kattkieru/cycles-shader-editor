#include "panel_edit_curve.h"

#include <cassert>

#include <GLFW/glfw3.h>
#include <nanovg.h>

#include "curve.h"
#include "gui_sizes.h"
#include "sockets.h"

static CyclesShaderEditor::Point2 get_panel_space_point(const CyclesShaderEditor::Point2 normalized_point, float hpad, float vpad, float width, float height)
{
	float out_x = hpad + normalized_point.get_pos_x() * width;
	float out_y = vpad + (1.0f - normalized_point.get_pos_y()) * height;

	return CyclesShaderEditor::Point2(out_x, out_y);
}

CyclesShaderEditor::EditCurvePanel::EditCurvePanel(float width) :
	panel_width(width),
	target_view(Point2(), Point2()),
	target_edit_mode_move(Point2(), Point2(), EditCurveMode::MOVE, &edit_mode),
	target_edit_mode_create(Point2(), Point2(), EditCurveMode::CREATE, &edit_mode),
	target_edit_mode_delete(Point2(), Point2(), EditCurveMode::DELETE, &edit_mode),
	target_interp_linear(Point2(), Point2(), CurveInterpolation::LINEAR, nullptr),
	target_interp_hermite(Point2(), Point2(), CurveInterpolation::CUBIC_HERMITE, nullptr)
{

}

bool CyclesShaderEditor::EditCurvePanel::is_active()
{
	return (attached_curve != nullptr);
}

void CyclesShaderEditor::EditCurvePanel::set_attached_curve_value(CyclesShaderEditor::CurveSocketValue* curve_value)
{
	attached_curve = curve_value;
}

void CyclesShaderEditor::EditCurvePanel::reset_panel_state()
{
	edit_mode = EditCurveMode::MOVE;
	selected_point_valid = false;
}

void CyclesShaderEditor::EditCurvePanel::pre_draw()
{
	assert(attached_curve != nullptr);

	if (move_selected_point && mouse_local_pos != move_selected_point_begin_mouse_pos) {
		mouse_has_moved = true;
	}

	if (selected_point_valid && move_selected_point && mouse_has_moved) {
		Point2 normalized_pos = target_view.get_normalized_mouse_pos(mouse_local_pos);
		normalized_pos.clamp_to(Point2(0.0f, 0.0f), Point2(1.0f, 1.0f));
		Point2 xy_pos = Point2(normalized_pos.get_pos_x(), 1.0f - normalized_pos.get_pos_y());
		selected_point_index = attached_curve->move_point(selected_point_index, xy_pos);
	}
}

float CyclesShaderEditor::EditCurvePanel::draw(NVGcontext* draw_context)
{
	float height_drawn = 0.0f;

	if (is_active() == false) {
		return 0.0f;
	}

	if (edit_mode != EditCurveMode::MOVE) {
		selected_point_valid = false;
	}

	// Draw curve view
	{
		const float rect_draw_x = UI_SUBWIN_PARAM_EDIT_RECT_HPAD;
		const float rect_draw_y = height_drawn + UI_SUBWIN_PARAM_EDIT_RECT_VPAD;
		const float rect_width = panel_width - 2 * UI_SUBWIN_PARAM_EDIT_RECT_HPAD;
		const float rect_height = UI_SUBWIN_PARAM_EDIT_RECT_HEIGHT;

		NVGcolor bg_color = nvgRGBAf(0.15f, 0.15f, 0.15f, 1.0f);
		NVGcolor bg_line_color = nvgRGBAf(0.45f, 0.45f, 0.45f, 1.0f);
		NVGcolor line_color = nvgRGBAf(0.8f, 0.8f, 0.8f, 1.0f);
		NVGcolor point_color = nvgRGBAf(0.9f, 0.9f, 0.9f, 1.0f);
		NVGcolor selected_point_color = nvgRGBAf(1.0f, 1.0f, 1.0f, 1.0f);

		// Draw background
		nvgBeginPath(draw_context);
		nvgRect(draw_context, rect_draw_x, rect_draw_y, rect_width, rect_height);
		nvgFillColor(draw_context, bg_color);
		nvgFill(draw_context);

		// Set click target for the view
		{
			const Point2 view_target_begin(rect_draw_x, rect_draw_y);
			const Point2 view_target_end(rect_draw_x + rect_width, rect_draw_y + rect_height);
			target_view = GenericClickTarget(view_target_begin, view_target_end);
		}

		// Draw background lines
		nvgBeginPath(draw_context);
		{
			// Horizontal
			for (int i = 1; i < 10; i++) {
				const float normalized_y = 0.1f * i;
				const Point2 left_point(0.0f, normalized_y);
				const Point2 right_point(1.0f, normalized_y);
				const Point2 left_point_panel_space = get_panel_space_point(left_point, UI_SUBWIN_PARAM_EDIT_RECT_HPAD, UI_SUBWIN_PARAM_EDIT_RECT_VPAD, rect_width, rect_height);
				const Point2 right_point_panel_space = get_panel_space_point(right_point, UI_SUBWIN_PARAM_EDIT_RECT_HPAD, UI_SUBWIN_PARAM_EDIT_RECT_VPAD, rect_width, rect_height);

				nvgMoveTo(draw_context, left_point_panel_space.get_floor_pos_x(), left_point_panel_space.get_floor_pos_y());
				nvgLineTo(draw_context, right_point_panel_space.get_floor_pos_x(), right_point_panel_space.get_floor_pos_y());
			}

			// Vertical
			for (int i = 1; i < 4; i++) {
				const float normalized_x = 0.25f * i;
				const Point2 top_point(normalized_x, 1.0f);
				const Point2 bot_point(normalized_x, 0.0f);
				const Point2 top_point_panel_space = get_panel_space_point(top_point, UI_SUBWIN_PARAM_EDIT_RECT_HPAD, UI_SUBWIN_PARAM_EDIT_RECT_VPAD, rect_width, rect_height);
				const Point2 bot_point_panel_space = get_panel_space_point(bot_point, UI_SUBWIN_PARAM_EDIT_RECT_HPAD, UI_SUBWIN_PARAM_EDIT_RECT_VPAD, rect_width, rect_height);

				nvgMoveTo(draw_context, top_point_panel_space.get_floor_pos_x(), top_point_panel_space.get_floor_pos_y());
				nvgLineTo(draw_context, bot_point_panel_space.get_floor_pos_x(), bot_point_panel_space.get_floor_pos_y());
			}
		}
		nvgStrokeWidth(draw_context, 1.0f);
		nvgStrokeColor(draw_context, bg_line_color);
		nvgStroke(draw_context);

		// Draw evaluated curve
		nvgBeginPath(draw_context);
		{
			constexpr int SEGMENTS = 128;
			constexpr float UNITS_PER_SEGMENT = 1.0f / SEGMENTS;
			const CurveEvaluator curve(attached_curve);
			for (int i = 0; i <= SEGMENTS; i++) {
				const float current_x = i * UNITS_PER_SEGMENT;
				const float current_y = curve.eval(current_x);
				const Point2 normalized_point(current_x, current_y);
				const Point2 draw_point = get_panel_space_point(normalized_point, UI_SUBWIN_PARAM_EDIT_RECT_HPAD, UI_SUBWIN_PARAM_EDIT_RECT_VPAD, rect_width, rect_height);
				if (i == 0) {
					nvgMoveTo(draw_context, draw_point.get_pos_x(), draw_point.get_pos_y());
				}
				else {
					nvgLineTo(draw_context, draw_point.get_pos_x(), draw_point.get_pos_y());
				}
			}
		}
		nvgStrokeWidth(draw_context, 1.0f);
		nvgStrokeColor(draw_context, line_color);
		nvgStroke(draw_context);

		// Draw points
		nvgBeginPath(draw_context);
		for (const Point2 this_point : attached_curve->curve_points) {
			const Point2 panel_space_point = get_panel_space_point(this_point, UI_SUBWIN_PARAM_EDIT_RECT_HPAD, UI_SUBWIN_PARAM_EDIT_RECT_VPAD, rect_width, rect_height);
			nvgCircle(draw_context, panel_space_point.get_pos_x(), panel_space_point.get_pos_y(), UI_SUBWIN_PARAM_EDIT_CURVE_POINT_RADIUS);
		}
		nvgFillColor(draw_context, point_color);
		nvgFill(draw_context);

		// Draw selected point
		if (selected_point_valid) {
			const Point2 selected_point = attached_curve->curve_points[selected_point_index];
			const Point2 selected_point_panel_space = get_panel_space_point(selected_point, UI_SUBWIN_PARAM_EDIT_RECT_HPAD, UI_SUBWIN_PARAM_EDIT_RECT_VPAD, rect_width, rect_height);
			nvgBeginPath(draw_context);
			nvgCircle(draw_context, selected_point_panel_space.get_pos_x(), selected_point_panel_space.get_pos_y(), UI_SUBWIN_PARAM_EDIT_CURVE_POINT_RADIUS * 1.5f);
			nvgStrokeWidth(draw_context, 1.0f);
			nvgStrokeColor(draw_context, selected_point_color);
			nvgStroke(draw_context);
		}

		height_drawn += 2 * UI_SUBWIN_PARAM_EDIT_RECT_VPAD + UI_SUBWIN_PARAM_EDIT_RECT_HEIGHT;
	}

	// Draw edit mode controls
	{
		// Shared font rendering state
		nvgFontSize(draw_context, UI_FONT_SIZE_NORMAL);
		nvgFontFace(draw_context, "sans");
		nvgFontBlur(draw_context, 0.0f);
		nvgFillColor(draw_context, nvgRGBA(0, 0, 0, 255));

		// Edit mode label
		nvgTextAlign(draw_context, NVG_ALIGN_RIGHT | NVG_ALIGN_BASELINE);
		const float mode_label_pos_x = 100.0f;
		const float mode_label_pos_y = height_drawn + UI_SUBWIN_PARAM_EDIT_LAYOUT_ROW_HEIGHT * 0.75f;
		nvgText(draw_context, mode_label_pos_x, mode_label_pos_y, "Edit Mode:", nullptr);

		// Edit mode option labels
		nvgTextAlign(draw_context, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
		const float label_choice_pos_x = mode_label_pos_x + UI_CHECKBOX_SPACING * 2 + UI_CHECKBOX_RADIUS * 2;
		const float label_move_pos_y = height_drawn + UI_SUBWIN_PARAM_EDIT_LAYOUT_ROW_HEIGHT * 0.75f;
		const float label_create_pos_y = height_drawn + UI_SUBWIN_PARAM_EDIT_LAYOUT_ROW_HEIGHT * 1.75f;
		const float label_delete_pos_y = height_drawn + UI_SUBWIN_PARAM_EDIT_LAYOUT_ROW_HEIGHT * 2.75f;
		nvgText(draw_context, label_choice_pos_x, label_move_pos_y, "Move", nullptr);
		nvgText(draw_context, label_choice_pos_x, label_create_pos_y, "Create", nullptr);
		nvgText(draw_context, label_choice_pos_x, label_delete_pos_y, "Delete", nullptr);

		// Edit mode option radio buttons
		const float radio_choice_pos_x = mode_label_pos_x + UI_CHECKBOX_SPACING + UI_CHECKBOX_RADIUS;
		const float radio_move_pos_y = height_drawn + UI_SUBWIN_PARAM_EDIT_LAYOUT_ROW_HEIGHT * 0.5f;
		const float radio_create_pos_y = height_drawn + UI_SUBWIN_PARAM_EDIT_LAYOUT_ROW_HEIGHT * 1.5f;
		const float radio_delete_pos_y = height_drawn + UI_SUBWIN_PARAM_EDIT_LAYOUT_ROW_HEIGHT * 2.5f;
		nvgBeginPath(draw_context);
		nvgCircle(draw_context, radio_choice_pos_x, radio_move_pos_y, UI_CHECKBOX_RADIUS);
		nvgCircle(draw_context, radio_choice_pos_x, radio_create_pos_y, UI_CHECKBOX_RADIUS);
		nvgCircle(draw_context, radio_choice_pos_x, radio_delete_pos_y, UI_CHECKBOX_RADIUS);
		nvgFillColor(draw_context, nvgRGBAf(0.1f, 0.1f, 0.1f, 1.0f));
		nvgFill(draw_context);

		// Mark for selected radio button
		float selected_radio_pos_y = 0.0f;
		if (edit_mode == EditCurveMode::MOVE) {
			selected_radio_pos_y = radio_move_pos_y;
		}
		else if (edit_mode == EditCurveMode::CREATE) {
			selected_radio_pos_y = radio_create_pos_y;
		}
		else if (edit_mode == EditCurveMode::DELETE) {
			selected_radio_pos_y = radio_delete_pos_y;
		}
		nvgBeginPath(draw_context);
		nvgCircle(draw_context, radio_choice_pos_x, selected_radio_pos_y, UI_CHECKBOX_RADIUS * 0.666f);
		nvgFillColor(draw_context, nvgRGBAf(0.9f, 0.9f, 0.9f, 1.0f));
		nvgFill(draw_context);

		// Make click targets
		const float click_target_begin_x = mode_label_pos_x + UI_CHECKBOX_SPACING;
		const float click_target_end_x = click_target_begin_x + 100.0f;
		const float horizontal_separator_0_y = height_drawn;
		const float horizontal_separator_1_y = height_drawn + UI_SUBWIN_PARAM_EDIT_LAYOUT_ROW_HEIGHT;
		const float horizontal_separator_2_y = height_drawn + UI_SUBWIN_PARAM_EDIT_LAYOUT_ROW_HEIGHT * 2;
		const float horizontal_separator_3_y = height_drawn + UI_SUBWIN_PARAM_EDIT_LAYOUT_ROW_HEIGHT * 3;
		target_edit_mode_move = CurveEditModeClickTarget(
			Point2(click_target_begin_x, horizontal_separator_0_y),
			Point2(click_target_end_x, horizontal_separator_1_y),
			EditCurveMode::MOVE,
			&edit_mode
		);
		target_edit_mode_create = CurveEditModeClickTarget(
			Point2(click_target_begin_x, horizontal_separator_1_y),
			Point2(click_target_end_x, horizontal_separator_2_y),
			EditCurveMode::CREATE,
			&edit_mode
		);
		target_edit_mode_delete = CurveEditModeClickTarget(
			Point2(click_target_begin_x, horizontal_separator_2_y),
			Point2(click_target_end_x, horizontal_separator_3_y),
			EditCurveMode::DELETE,
			&edit_mode
		);

		height_drawn += (UI_SUBWIN_PARAM_EDIT_LAYOUT_ROW_HEIGHT * 3);
	}

	// Draw separator
	{
		height_drawn += UI_SUBWIN_PARAM_EDIT_SEPARATOR_VPAD;
		nvgBeginPath(draw_context);
		nvgMoveTo(draw_context, UI_SUBWIN_PARAM_EDIT_SEPARATOR_HPAD, height_drawn);
		nvgLineTo(draw_context, panel_width - UI_SUBWIN_PARAM_EDIT_SEPARATOR_HPAD, height_drawn);
		nvgStrokeColor(draw_context, nvgRGBAf(0.0f, 0.0f, 0.0f, 1.0f));
		nvgStrokeWidth(draw_context, 0.8f);
		nvgStroke(draw_context);
		height_drawn += UI_SUBWIN_PARAM_EDIT_SEPARATOR_VPAD;
	}

	// Draw interpolation mode controls
	{
		// Shared font rendering state
		nvgFontSize(draw_context, UI_FONT_SIZE_NORMAL);
		nvgFontFace(draw_context, "sans");
		nvgFontBlur(draw_context, 0.0f);
		nvgFillColor(draw_context, nvgRGBA(0, 0, 0, 255));

		// Interpolation mode label
		nvgTextAlign(draw_context, NVG_ALIGN_RIGHT | NVG_ALIGN_BASELINE);
		const float mode_label_pos_x = 100.0f;
		const float mode_label_pos_y = height_drawn + UI_SUBWIN_PARAM_EDIT_LAYOUT_ROW_HEIGHT * 0.75f;
		nvgText(draw_context, mode_label_pos_x, mode_label_pos_y, "Interpolation:", nullptr);

		// Interpolation mode option labels
		nvgTextAlign(draw_context, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
		const float label_choice_pos_x = mode_label_pos_x + UI_CHECKBOX_SPACING * 2 + UI_CHECKBOX_RADIUS * 2;
		const float label_linear_pos_y = height_drawn + UI_SUBWIN_PARAM_EDIT_LAYOUT_ROW_HEIGHT * 0.75f;
		const float label_hermite_pos_y = height_drawn + UI_SUBWIN_PARAM_EDIT_LAYOUT_ROW_HEIGHT * 1.75f;
		nvgText(draw_context, label_choice_pos_x, label_linear_pos_y, "Linear", nullptr);
		nvgText(draw_context, label_choice_pos_x, label_hermite_pos_y, "Cubic Hermite", nullptr);

		// Interpolation mode option radio buttons
		const float radio_choice_pos_x = mode_label_pos_x + UI_CHECKBOX_SPACING + UI_CHECKBOX_RADIUS;
		const float radio_linear_pos_y = height_drawn + UI_SUBWIN_PARAM_EDIT_LAYOUT_ROW_HEIGHT * 0.5f;
		const float radio_hermite_pos_y = height_drawn + UI_SUBWIN_PARAM_EDIT_LAYOUT_ROW_HEIGHT * 1.5f;
		nvgBeginPath(draw_context);
		nvgCircle(draw_context, radio_choice_pos_x, radio_linear_pos_y, UI_CHECKBOX_RADIUS);
		nvgCircle(draw_context, radio_choice_pos_x, radio_hermite_pos_y, UI_CHECKBOX_RADIUS);
		nvgFillColor(draw_context, nvgRGBAf(0.1f, 0.1f, 0.1f, 1.0f));
		nvgFill(draw_context);

		// Mark for selected radio button
		float selected_radio_pos_y = 0.0f;
		if (attached_curve->curve_interp == CurveInterpolation::LINEAR) {
			selected_radio_pos_y = radio_linear_pos_y;
		}
		else if (attached_curve->curve_interp == CurveInterpolation::CUBIC_HERMITE) {
			selected_radio_pos_y = radio_hermite_pos_y;
		}
		nvgBeginPath(draw_context);
		nvgCircle(draw_context, radio_choice_pos_x, selected_radio_pos_y, UI_CHECKBOX_RADIUS * 0.666f);
		nvgFillColor(draw_context, nvgRGBAf(0.9f, 0.9f, 0.9f, 1.0f));
		nvgFill(draw_context);

		// Make click targets
		const float click_target_begin_x = mode_label_pos_x + UI_CHECKBOX_SPACING;
		const float click_target_end_x = click_target_begin_x + 120.0f;
		const float horizontal_separator_0_y = height_drawn;
		const float horizontal_separator_1_y = height_drawn + UI_SUBWIN_PARAM_EDIT_LAYOUT_ROW_HEIGHT;
		const float horizontal_separator_2_y = height_drawn + UI_SUBWIN_PARAM_EDIT_LAYOUT_ROW_HEIGHT * 2;
		target_interp_linear = CurveInterpClickTarget(
			Point2(click_target_begin_x, horizontal_separator_0_y),
			Point2(click_target_end_x, horizontal_separator_1_y),
			CurveInterpolation::LINEAR,
			&(attached_curve->curve_interp)
		);
		target_interp_hermite = CurveInterpClickTarget(
			Point2(click_target_begin_x, horizontal_separator_1_y),
			Point2(click_target_end_x, horizontal_separator_2_y),
			CurveInterpolation::CUBIC_HERMITE,
			&(attached_curve->curve_interp)
		);

		height_drawn += (UI_SUBWIN_PARAM_EDIT_LAYOUT_ROW_HEIGHT * 2);
	}

	panel_height = height_drawn;
	return height_drawn;
}

void CyclesShaderEditor::EditCurvePanel::set_mouse_local_position(Point2 local_pos)
{
	mouse_local_pos = local_pos;
}

bool CyclesShaderEditor::EditCurvePanel::is_mouse_over()
{
	if (is_active() == false) {
		return false;
	}

	const float min_x = 0.0f;
	const float max_x = panel_width;
	const float min_y = 0.0f;
	const float max_y = panel_height;

	if (mouse_local_pos.get_pos_x() > min_x &&
		mouse_local_pos.get_pos_x() < max_x &&
		mouse_local_pos.get_pos_y() > min_y &&
		mouse_local_pos.get_pos_y() < max_y)
	{
		return true;
	}

	return false;
}

void CyclesShaderEditor::EditCurvePanel::handle_mouse_button(int button, int action, int /*mods*/)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		if (target_view.is_mouse_over_target(mouse_local_pos)) {
			Point2 normalized_pos = target_view.get_normalized_mouse_pos(mouse_local_pos);
			Point2 xy_pos = Point2(normalized_pos.get_pos_x(), 1.0f - normalized_pos.get_pos_y());
			if (edit_mode == EditCurveMode::MOVE) {
				size_t target_index;
				if (attached_curve->get_target_index(xy_pos, target_index)) {
					// Select new point
					selected_point_index = target_index;
					selected_point_valid = true;
				}
				// Move selected point
				if (selected_point_valid) {
					move_selected_point = true;
					mouse_has_moved = false;
					move_selected_point_begin_mouse_pos = mouse_local_pos;
				}
			}
			else if (edit_mode == EditCurveMode::CREATE) {
				attached_curve->create_point(xy_pos.get_pos_x());
				request_undo_push = true;
			}
			else if (edit_mode == EditCurveMode::DELETE) {
				attached_curve->delete_point(xy_pos);
				request_undo_push = true;
			}
		}
		if (target_edit_mode_move.is_mouse_over_target(mouse_local_pos)) {
			target_edit_mode_move.click();
			request_undo_push = true;
		}
		else if (target_edit_mode_create.is_mouse_over_target(mouse_local_pos)) {
			target_edit_mode_create.click();
			request_undo_push = true;
		}
		else if (target_edit_mode_delete.is_mouse_over_target(mouse_local_pos)) {
			target_edit_mode_delete.click();
			request_undo_push = true;
		}
		else if (target_interp_linear.is_mouse_over_target(mouse_local_pos)) {
			target_interp_linear.click();
			request_undo_push = true;
		}
		else if (target_interp_hermite.is_mouse_over_target(mouse_local_pos)) {
			target_interp_hermite.click();
			request_undo_push = true;
		}
	}
}

void CyclesShaderEditor::EditCurvePanel::mouse_button_release()
{
	if (move_selected_point && mouse_has_moved) {
		request_undo_push = true;
		mouse_has_moved = false;
	}
	move_selected_point = false;
}

bool CyclesShaderEditor::EditCurvePanel::should_push_undo_state()
{
	bool result = request_undo_push;
	request_undo_push = false;
	return result;
}
