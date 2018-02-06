#include "curve.h"

#include "sockets.h"
#include "util_hermite_spline.h"

CyclesShaderEditor::CurveEvaluator::CurveEvaluator(CurveSocketValue* curve_socket_val, int segments)
{
	const std::vector<Point2>& curve_control_points = curve_socket_val->curve_points;

	// No points, in this case treat as output = input
	if (curve_control_points.size() == 0) {
		const Point2 point_0(0.0f, 0.0f);
		const Point2 point_1(1.0f, 1.0f);
		sampled_points.push_back(point_0);
		sampled_points.push_back(point_1);
		return;
	}
	// One point, this represents constant output
	else if (curve_control_points.size() == 1) {
		const float const_output = curve_control_points[0].get_pos_y();
		const Point2 point_0(0.0f, const_output);
		const Point2 point_1(1.0f, const_output);
		sampled_points.push_back(point_0);
		sampled_points.push_back(point_1);
		return;
	}

	if (curve_socket_val->curve_interp == CurveInterpolation::CUBIC_HERMITE) {
		// Sample as a series of cubic hermite splines
		constexpr float FIRST_SAMPLE_X = 0.0f;
		constexpr float LAST_SAMPLE_X = 1.0f;

		const float x_increment = (LAST_SAMPLE_X - FIRST_SAMPLE_X) / segments;
		float next_sample_x = FIRST_SAMPLE_X;
		const size_t final_segment_begin_index = curve_control_points.size() - 2;
		for (size_t segment_begin_index = 0; segment_begin_index <= final_segment_begin_index; segment_begin_index++) {
			const Point2 point_b = curve_control_points[segment_begin_index];
			const Point2 point_c = curve_control_points[segment_begin_index + 1];
			Point2 point_a, point_d;
			if (segment_begin_index == 0) {
				point_a = point_b + point_b - point_c;
			}
			else {
				point_a = curve_control_points[segment_begin_index - 1];
			}
			if (segment_begin_index == final_segment_begin_index) {
				point_d = point_c + point_c - point_b;
			}
			else {
				point_d = curve_control_points[segment_begin_index + 2];
			}
			CurveEvaluator hermite_spline(point_a, point_b, point_c, point_d);

			// If the sample is before the first control point(b), use the value of the first control point
			while (next_sample_x <= LAST_SAMPLE_X && hermite_spline.compare_to_range(next_sample_x) == -1) {
				const Point2 new_point(next_sample_x, point_b.get_pos_y());
				sampled_points.push_back(new_point);
				next_sample_x += x_increment;
			}

			// If the sample is in this range, evaluate normally
			while (next_sample_x <= LAST_SAMPLE_X && hermite_spline.compare_to_range(next_sample_x) == 0) {
				const float sampled_y = hermite_spline.eval(next_sample_x);
				const Point2 new_point(next_sample_x, sampled_y);
				sampled_points.push_back(new_point);
				next_sample_x += x_increment;
			}

			// If the sample is past this range and we are in the last segment, use the value of the last control point
			while (next_sample_x <= LAST_SAMPLE_X && hermite_spline.compare_to_range(next_sample_x) == 1 && segment_begin_index == final_segment_begin_index) {
				const Point2 new_point(next_sample_x, point_c.get_pos_y());
				sampled_points.push_back(new_point);
				next_sample_x += x_increment;
			}
		}
	}
	else {
		// Default is linear
		// In this case, copy points directly rather than sampling
		for (const Point2 this_point : curve_control_points) {
			sampled_points.push_back(this_point);
		}
	}
}

CyclesShaderEditor::CurveEvaluator::CurveEvaluator(Point2 a, Point2 b, Point2 c, Point2 d, int segments)
{
	const CubicHermiteSplineInterpolator x_solver(a.get_pos_x(), b.get_pos_x(), c.get_pos_x(), d.get_pos_x());
	const CubicHermiteSplineInterpolator y_solver(a.get_pos_y(), b.get_pos_y(), c.get_pos_y(), d.get_pos_y());

	const float segment_size = 1.0f / segments;
	for (int i = 0; i <= segments; i++) {
		const float this_t = segment_size * i;
		const float this_x = x_solver.eval(this_t);
		const float this_y = y_solver.eval(this_t);
		const Point2 this_point(this_x, this_y);
		sampled_points.push_back(this_point);
	}
}

int CyclesShaderEditor::CurveEvaluator::compare_to_range(float in_value) const
{
	if (sampled_points.size() < 1) {
		return 0;
	}

	const float first = sampled_points[0].get_pos_x();
	if (in_value < first) {
		return -1;
	}

	const float last = sampled_points[sampled_points.size() - 1].get_pos_x();
	if (in_value > last) {
		return 1;
	}

	return 0;
}

float CyclesShaderEditor::CurveEvaluator::eval(float in_value) const
{
	if (sampled_points.size() < 2) {
		return in_value;
	}

	typedef std::vector<Point2>::size_type vec_size_t;

	// sampled_points is ordered by x value
	// do a binary search to find which two values surround the input value
	vec_size_t min_index = 0;
	vec_size_t max_index = sampled_points.size() - 1;
	while ((max_index - min_index) > 1) {
		const vec_size_t delta = max_index - min_index;
		const vec_size_t index_to_check = min_index + delta / 2;
		const float value_to_check = sampled_points[index_to_check].get_pos_x();
		if (in_value >= value_to_check) {
			min_index = index_to_check;
		}
		else {
			max_index = index_to_check;
		}
	}

	const Point2 point_a = sampled_points[min_index];
	const Point2 point_b = sampled_points[max_index];
	const float x_min = point_a.get_pos_x();
	const float x_max = point_b.get_pos_x();
	const float x_delta = x_max - x_min;

	if (in_value < x_min) {
		return point_a.get_pos_y();
	}
	if (in_value > x_max) {
		return point_b.get_pos_y();
	}

	const float weight_factor = (in_value - x_min) / (x_delta);
	const float result = (1.0f - weight_factor) * point_a.get_pos_y() + weight_factor * point_b.get_pos_y();

	return result;
}
