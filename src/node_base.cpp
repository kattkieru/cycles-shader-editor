#include "node_base.h"

#include <cmath>
#include <iomanip>
#include <sstream>

#include <GLFW/glfw3.h>
#include <nanovg.h>

#include "config.h"
#include "curve.h"
#include "gui_sizes.h"
#include "sockets.h"

CyclesShaderEditor::NodeConnection::NodeConnection(NodeSocket* begin_socket, NodeSocket* end_socket)
{
	this->begin_socket = begin_socket;
	this->end_socket = end_socket;
}

bool CyclesShaderEditor::NodeConnection::includes_node(EditorNode* node)
{
	if (begin_socket->parent == node || end_socket->parent == node) {
		return true;
	}
	else {
		return false;
	}
}

CyclesShaderEditor::EditorNode::~EditorNode()
{
	for (NodeSocket* socket : sockets) {
		delete socket;
	}
}

std::string CyclesShaderEditor::EditorNode::get_title()
{
	return title;
}

void CyclesShaderEditor::EditorNode::draw_node(NVGcontext* draw_context)
{
	float draw_pos_x = 0.0f;
	float draw_pos_y = 0.0f;

	content_height = sockets.size() * UI_NODE_SOCKET_ROW_HEIGHT + UI_NODE_BOTTOM_PADDING;

	// Draw window
	nvgBeginPath(draw_context);
	nvgRoundedRect(draw_context, draw_pos_x, draw_pos_y, content_width, content_height + UI_NODE_HEADER_HEIGHT, UI_NODE_CORNER_RADIUS);
	nvgFillColor(draw_context, nvgRGBA(180, 180, 180, 255));
	nvgFill(draw_context);

	// Draw header
	{
		const float rect_pos_y = draw_pos_y + UI_NODE_HEADER_HEIGHT - UI_NODE_CORNER_RADIUS;
		nvgBeginPath(draw_context);
		nvgRoundedRect(draw_context, draw_pos_x, draw_pos_y, content_width, UI_NODE_HEADER_HEIGHT, UI_NODE_CORNER_RADIUS);
		nvgRect(draw_context, draw_pos_x, rect_pos_y, content_width, UI_NODE_CORNER_RADIUS);
		if (is_mouse_over_header()) {
			nvgFillColor(draw_context, nvgRGBA(225, 225, 225, 255));
		}
		else {
			nvgFillColor(draw_context, nvgRGBA(210, 210, 210, 255));
		}
		nvgFill(draw_context);
	}

	// Draw border
	nvgBeginPath(draw_context);
	nvgRoundedRect(draw_context, draw_pos_x, draw_pos_y, content_width, content_height + UI_NODE_HEADER_HEIGHT, UI_NODE_CORNER_RADIUS);
	if (selected) {
		nvgStrokeColor(draw_context, nvgRGBA(255, 255, 255, 225));
	}
	else {
		nvgStrokeColor(draw_context, nvgRGBA(0, 0, 0, 225));
	}
	nvgStrokeWidth(draw_context, 1.5f);
	nvgStroke(draw_context);

	// Title
	nvgFontSize(draw_context, UI_FONT_SIZE_NORMAL);
	nvgFontFace(draw_context, "sans");
	nvgTextAlign(draw_context, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
	nvgFontBlur(draw_context, 0.0f);
	nvgFillColor(draw_context, nvgRGBA(0, 0, 0, 255));
	nvgText(draw_context, draw_pos_x + content_width / 2, draw_pos_y + UI_NODE_HEADER_HEIGHT / 2, title.c_str(), NULL);

	float next_draw_y = draw_pos_y + UI_NODE_HEADER_HEIGHT + 2.0f;
	// Sockets
	label_targets.clear();
	socket_targets.clear();
	for (NodeSocket* this_socket: sockets) {
		// Generate the text that will be used on this socket's label
		std::string label_text;
		std::string text_before_crossout; // For measuring text size later
		if (this_socket->value != nullptr) {
			text_before_crossout = this_socket->display_name + ":";
			if (this_socket->socket_type == SocketType::Float) {
				FloatSocketValue* float_val = dynamic_cast<FloatSocketValue*>(this_socket->value);
				std::stringstream label_string_stream;
				label_string_stream << this_socket->display_name << ": "  << std::fixed << std::setprecision(3) << float_val->get_value();
				label_text = label_string_stream.str();
			}
			else if (this_socket->socket_type == SocketType::Vector) {
				if (this_socket->selectable == true) {
					label_text = this_socket->display_name + ": [Vector]";
				}
				else {
					label_text = this_socket->display_name;
				}
			}
			else if (this_socket->socket_type == SocketType::Color) {
				std::stringstream label_string_stream;
				label_string_stream << this_socket->display_name << ": ";
				label_text = label_string_stream.str();
			}
			else if (this_socket->socket_type == SocketType::StringEnum) {
				label_text = this_socket->display_name + ": [Enum]";
			}
			else if (this_socket->socket_type == SocketType::Int) {
				IntSocketValue* int_val = dynamic_cast<IntSocketValue*>(this_socket->value);
				std::stringstream label_string_stream;
				label_string_stream << this_socket->display_name << ": " << std::fixed << std::setprecision(3) << int_val->get_value();
				label_text = label_string_stream.str();
			}

			else if (this_socket->socket_type == SocketType::Boolean) {
				BoolSocketValue* bool_val = dynamic_cast<BoolSocketValue*>(this_socket->value);
				if (bool_val->value) {
					label_text = this_socket->display_name + ": True";
				}
				else {
					label_text = this_socket->display_name + ": False";
				}
			}
			else if (this_socket->socket_type == SocketType::Curve) {
				label_text = this_socket->display_name + ": [Curve]";
			}
			else {
				label_text = this_socket->display_name;
			}
		}
		else {
			label_text = this_socket->display_name;
		}

		// Draw highlight if this node is selected
		if (this_socket->selected) {
			nvgBeginPath(draw_context);
			nvgRoundedRect(draw_context, 4.0f, next_draw_y, content_width - 8.0f, UI_NODE_SOCKET_ROW_HEIGHT, 0.0f);
			nvgFillColor(draw_context, nvgRGBA(210, 210, 210, 255));
			nvgFill(draw_context);
		}

		// Draw label
		nvgFontSize(draw_context, UI_FONT_SIZE_NORMAL);
		nvgTextAlign(draw_context, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
		nvgFontBlur(draw_context, 0.0f);
		nvgFillColor(draw_context, nvgRGBA(0, 0, 0, 255));
		nvgFontFace(draw_context, "sans");

		if (this_socket->value != nullptr && this_socket->socket_type == SocketType::Color) {
			const float SWATCH_HEIGHT = 14.0f;
			const float SWATCH_WIDTH = 24.0f;
			const float SWATCH_CORNER_RADIUS = 8.0f;

			float bounds[4];
			nvgTextBounds(draw_context, 0.0f, 0.0f, label_text.c_str(), nullptr, bounds);
			float label_width = bounds[2] - bounds[0];

			const float text_pos_x = draw_pos_x + content_width / 2 - SWATCH_WIDTH / 2;
			const float text_pos_y = next_draw_y + UI_NODE_SOCKET_ROW_HEIGHT / 2;
			nvgText(draw_context, text_pos_x, text_pos_y, label_text.c_str(), nullptr);

			FloatRGBColor swatch_color = dynamic_cast<ColorSocketValue*>(this_socket->value)->get_value();

			const float swatch_pos_x = text_pos_x + label_width / 2 + 1.0f;
			const float swatch_pos_y = next_draw_y + (UI_NODE_SOCKET_ROW_HEIGHT - SWATCH_HEIGHT) / 2;

			nvgBeginPath(draw_context);
			nvgRoundedRect(draw_context, swatch_pos_x, swatch_pos_y, SWATCH_WIDTH, SWATCH_HEIGHT, SWATCH_CORNER_RADIUS);
			nvgFillColor(draw_context, nvgRGBAf(swatch_color.r, swatch_color.g, swatch_color.b, 1.0f));
			nvgFill(draw_context);
			nvgStrokeWidth(draw_context, 1.0f);
			nvgStrokeColor(draw_context, nvgRGBA(0, 0, 0, 255));
			nvgStroke(draw_context);

			if (this_socket->input_connected_this_frame && this_socket->value != nullptr) {
				const float x1 = swatch_pos_x - 3.0f;
				const float x2 = swatch_pos_x  + SWATCH_WIDTH + 3.0f;
				const float y = swatch_pos_y + SWATCH_HEIGHT / 2.0f;
				nvgBeginPath(draw_context);
				nvgMoveTo(draw_context, x1, y);
				nvgLineTo(draw_context, x2, y);
				nvgStrokeWidth(draw_context, 1.2f);
				nvgStroke(draw_context);
				this_socket->input_connected_this_frame = false;
			}
		}
		else {
			const float text_pos_x = draw_pos_x + content_width / 2;
			const float text_pos_y = next_draw_y + UI_NODE_SOCKET_ROW_HEIGHT / 2;
			nvgText(draw_context, text_pos_x, text_pos_y, label_text.c_str(), nullptr);
			if (this_socket->input_connected_this_frame && this_socket->value != nullptr) {
				// Output is [xmin, ymin, xmax, ymax]
				float full_size[4];
				float short_size[4];
				nvgTextBounds(draw_context, text_pos_x, text_pos_y, label_text.c_str(), nullptr, full_size);
				nvgTextBounds(draw_context, text_pos_x, text_pos_y, text_before_crossout.c_str(), nullptr, short_size);
				const float short_width = short_size[2] - short_size[0];
				const float x1 = full_size[0] + short_width + 1.0f;
				const float x2 = full_size[2] + 2.0f;
				nvgBeginPath(draw_context);
				nvgMoveTo(draw_context, x1, text_pos_y);
				nvgLineTo(draw_context, x2, text_pos_y);
				nvgStrokeWidth(draw_context, 1.2f);
				nvgStroke(draw_context);
				this_socket->input_connected_this_frame = false;
			}
		}

		if (this_socket->selectable) {
			// Add label click target
			CyclesShaderEditor::Point2 click_target_begin(0, next_draw_y - draw_pos_y);
			CyclesShaderEditor::Point2 click_target_end(content_width, next_draw_y - draw_pos_y + UI_NODE_SOCKET_ROW_HEIGHT);
			SocketClickTarget label_target(click_target_begin, click_target_end, this_socket);
			label_targets.push_back(label_target);
		}

		if (this_socket->draw_socket) {
			CyclesShaderEditor::Point2 socket_position;
			if (this_socket->socket_in_out == SocketInOut::Input) {
				socket_position = CyclesShaderEditor::Point2(draw_pos_x, next_draw_y + UI_NODE_SOCKET_ROW_HEIGHT / 2);
			}
			else {
				socket_position = CyclesShaderEditor::Point2(draw_pos_x + content_width, next_draw_y + UI_NODE_SOCKET_ROW_HEIGHT / 2);
			}
			nvgBeginPath(draw_context);
			nvgCircle(draw_context, socket_position.get_pos_x(), socket_position.get_pos_y(), UI_NODE_SOCKET_RADIUS);

			this_socket->world_draw_position = world_pos + socket_position;

			// Add click target for this socket
			CyclesShaderEditor::Point2 local_socket_position(socket_position.get_pos_x() - draw_pos_x, socket_position.get_pos_y() - draw_pos_y);
			CyclesShaderEditor::Point2 click_target_begin(local_socket_position.get_pos_x() - 7.0f, local_socket_position.get_pos_y() - 7.0f);
			CyclesShaderEditor::Point2 click_target_end(local_socket_position.get_pos_x() + 7.0f, local_socket_position.get_pos_y() + 7.0f);
			SocketClickTarget socket_target(click_target_begin, click_target_end, this_socket);
			socket_targets.push_back(socket_target);

			if (this_socket->socket_type == SocketType::Closure) {
				nvgFillColor(draw_context, nvgRGBA(100, 200, 100, 255));
			}
			else if (this_socket->socket_type == SocketType::Color) {
				nvgFillColor(draw_context, nvgRGBA(200, 200, 42, 255));
			}
			else if (this_socket->socket_type == SocketType::Float) {
				nvgFillColor(draw_context, nvgRGBA(240, 240, 240, 255));
			}
			else if (this_socket->socket_type == SocketType::Normal) {
				nvgFillColor(draw_context, nvgRGBA(100, 100, 200, 255));
			}
			else if (this_socket->socket_type == SocketType::Vector) {
				nvgFillColor(draw_context, nvgRGBA(100, 100, 200, 255));
			}
			else {
				nvgFillColor(draw_context, nvgRGBA(255, 0, 0, 255));
			}

			nvgFill(draw_context);

			nvgStrokeColor(draw_context, nvgRGBf(0.0f, 0.0f, 0.0f));
			nvgStrokeWidth(draw_context, 1.0f);
			nvgStroke(draw_context);
		}

		next_draw_y += UI_NODE_SOCKET_ROW_HEIGHT;
	}
}

void CyclesShaderEditor::EditorNode::set_mouse_position(CyclesShaderEditor::Point2 node_local_position)
{
	if (node_moving) {
		const CyclesShaderEditor::Point2 mouse_movement = (node_local_position - mouse_local_begin_move_pos);
		world_pos = world_pos + mouse_movement;
		mouse_local_pos = mouse_local_begin_move_pos;
		if (std::abs(mouse_movement.get_pos_x()) + std::abs(mouse_movement.get_pos_y()) > 0.1f) {
			has_moved = true;
		}
	}
	else {
		mouse_local_pos = node_local_position;
	}
}

bool CyclesShaderEditor::EditorNode::is_mouse_over_node()
{
	if (node_moving) {
		return true;
	}
	return (mouse_local_pos.get_pos_x() >= 0.0f &&
		mouse_local_pos.get_pos_x() <= content_width &&
		mouse_local_pos.get_pos_y() >= 0.0f &&
		mouse_local_pos.get_pos_y() <= UI_NODE_HEADER_HEIGHT + content_height);
}

bool CyclesShaderEditor::EditorNode::is_mouse_over_header()
{
	if (node_moving) {
		return true;
	}
	return (mouse_local_pos.get_pos_x() >= 0.0f &&
		mouse_local_pos.get_pos_x() <= content_width &&
		mouse_local_pos.get_pos_y() >= 0.0f &&
		mouse_local_pos.get_pos_y() <= UI_NODE_HEADER_HEIGHT);
}

void CyclesShaderEditor::EditorNode::handle_mouse_button(int /*button*/, int /*action*/, int /*mods*/)
{

}

void CyclesShaderEditor::EditorNode::move_begin()
{
	has_moved = false;
	node_moving = true;
	mouse_local_begin_move_pos = mouse_local_pos;
}

void CyclesShaderEditor::EditorNode::move_end()
{
	if (node_moving && has_moved) {
		changed = true;
	}
	node_moving = false;
}

CyclesShaderEditor::NodeSocket* CyclesShaderEditor::EditorNode::get_socket_under_mouse()
{
	std::vector<SocketClickTarget>::iterator target_iter;
	for (target_iter = socket_targets.begin(); target_iter != socket_targets.end(); ++target_iter) {
		if ((*target_iter).is_mouse_over_target(mouse_local_pos)) {
			return (*target_iter).socket;
		}
	}
	return nullptr;
}

CyclesShaderEditor::NodeSocket* CyclesShaderEditor::EditorNode::get_socket_label_under_mouse()
{
	std::vector<SocketClickTarget>::iterator target_iter;
	for (target_iter = label_targets.begin(); target_iter != label_targets.end(); ++target_iter) {
		if ((*target_iter).is_mouse_over_target(mouse_local_pos)) {
			return (*target_iter).socket;
		}
	}
	return nullptr;
}

CyclesShaderEditor::NodeSocket* CyclesShaderEditor::EditorNode::get_socket_by_display_name(SocketInOut in_out, std::string socket_name)
{
	for (NodeSocket* socket : sockets) {
		if (socket->display_name == socket_name && socket->socket_in_out == in_out) {
			return socket;
		}
	}

	return nullptr;
}

CyclesShaderEditor::NodeSocket* CyclesShaderEditor::EditorNode::get_socket_by_internal_name(SocketInOut in_out, std::string socket_name)
{
	for (NodeSocket* socket : sockets) {
		if (socket->internal_name == socket_name && socket->socket_in_out == in_out) {
			return socket;
		}
	}

	return nullptr;
}

CyclesShaderEditor::Point2 CyclesShaderEditor::EditorNode::get_dimensions()
{
	return CyclesShaderEditor::Point2(content_width, content_height + UI_NODE_HEADER_HEIGHT);
}

bool CyclesShaderEditor::EditorNode::can_be_deleted()
{
	return true;
}

void CyclesShaderEditor::EditorNode::update_output_node(OutputNode& output)
{
	output.type = type;

	output.world_x = world_pos.get_floor_pos_x();
	output.world_y = world_pos.get_floor_pos_y();

	if (type == CyclesNodeType::MaterialOutput) {
		output.name = std::string("output");
	}

	for (NodeSocket* this_socket : sockets) {
		if (this_socket->socket_in_out != SocketInOut::Input) {
			continue;
		}

		if (this_socket->socket_type == SocketType::Float && this_socket->value != nullptr) {
			FloatSocketValue* float_val = dynamic_cast<FloatSocketValue*>(this_socket->value);
			output.float_values[this_socket->internal_name] = float_val->get_value();
		}
		else if (this_socket->socket_type == SocketType::Color) {
			ColorSocketValue* color_val = dynamic_cast<ColorSocketValue*>(this_socket->value);
			const float x = color_val->red_socket_val.get_value();
			const float y = color_val->green_socket_val.get_value();
			const float z = color_val->blue_socket_val.get_value();
			Float3 float3_val(x, y, z);
			output.float3_values[this_socket->internal_name] = float3_val;
		}
		else if (this_socket->socket_type == SocketType::Vector && this_socket->value != nullptr) {
			Float3SocketValue* float3_socket_val = dynamic_cast<Float3SocketValue*>(this_socket->value);
			Float3Holder temp_value = float3_socket_val->get_value();
			Float3 float3_val(temp_value.x, temp_value.y, temp_value.z);
			output.float3_values[this_socket->internal_name] = float3_val;
		}
		else if (this_socket->socket_type == SocketType::StringEnum) {
			StringEnumSocketValue* string_val = dynamic_cast<StringEnumSocketValue*>(this_socket->value);
			output.string_values[this_socket->internal_name] = string_val->value.internal_value;
		}
		else if (this_socket->socket_type == SocketType::Int) {
			IntSocketValue* int_val = dynamic_cast<IntSocketValue*>(this_socket->value);
			if (int_val != nullptr) {
				output.int_values[this_socket->internal_name] = int_val->get_value();
			}
		}
		else if (this_socket->socket_type == SocketType::Boolean) {
			BoolSocketValue* bool_val = dynamic_cast<BoolSocketValue*>(this_socket->value);
			if (bool_val != nullptr) {
				output.bool_values[this_socket->internal_name] = bool_val->value;
			}
		}
		else if (this_socket->socket_type == SocketType::Curve) {
			CurveSocketValue* curve_val = dynamic_cast<CurveSocketValue*>(this_socket->value);
			if (curve_val != nullptr) {
				OutputCurve out_curve;
				for (size_t i = 0; i < curve_val->curve_points.size(); i++) {
					const Point2 this_point = curve_val->curve_points[i];
					out_curve.control_points.push_back(Float2(this_point.get_pos_x(), this_point.get_pos_y()));
				}
				out_curve.enum_curve_interp = static_cast<int>(curve_val->curve_interp);
				CurveEvaluator curve(curve_val);
				for (size_t i = 0; i < CURVE_TABLE_SIZE; i++) {
					const float x = static_cast<float>(i) / (CURVE_TABLE_SIZE - 1.0f);
					out_curve.samples.push_back(curve.eval(x));
				}
				output.curve_values[this_socket->internal_name] = out_curve;
			}
		}
	}
}
