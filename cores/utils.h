#include <math.h>
#include <stdint.h>
#include <stdio.h>

float uint16_to_float(uint16_t value) {
  return (value / 65535.0f) * 2.0f - 1.0f;
}

float clamp(float value, float min, float max) {
  if (value < min) {
    return min;
  } else if (value > max) {
    return max;
  } else {
    return value;
  }
}

float weighted_value(float input, float transition_width) {
  // Define the seven values in an array
  float values[7] = {0.0, 0.090909, 0.142857, 0.2, 0.3333, 0.5, 1.0};

  // Ensure input is between 0 and 1, inclusive
  if (input < 0.0 || input > 1.0) {
    printf("Input must be between 0 and 1.\n");
    return -1; // Error code for invalid input
  }

  // Calculate the size of each main range (1 divided by 7)
  float range_size = 1.0 / 7.0;

  // Define the transition width as a small portion of the range (e.g., 10%)
  transition_width *= range_size;

  // Determine the base index for the range
  int index = (int)(input / range_size);

  // Handle edge case where input is exactly 1.0
  if (index >= 6) {
    return values[6];
  }

  // Calculate the start of the transition zone for this range
  float range_start = index * range_size;
  float transition_start = range_start + (range_size - transition_width);

  // If in transition zone, interpolate to the next value
  if (input >= transition_start) {
    float t = (input - transition_start) / transition_width;
    return (1 - t) * values[index] + t * values[index + 1];
  }

  // If outside transition zone, return the base value
  return values[index];
}

float scale(float input, float min_in, float max_in, float min_out,
            float max_out, float exponent) {
  if (max_in == min_in) {
    fprintf(stderr, "Error: Input range cannot have zero width.\n");
    return min_out;
  }

  float normalized = (input - min_in) / (max_in - min_in);
  float out_range = max_out - min_out;

  // Handle positive and negative input ranges
  if (normalized == 0.0) {
    return min_out;
  } else if (normalized > 0.0) {
    return min_out + out_range * pow(normalized, exponent);
  } else {
    return min_out + out_range * -pow(-normalized, exponent);
  }
}

float dc_blocker(float input, float *last_input, float *last_output,
                 float coeff) {
  float output = input - *last_input + coeff * (*last_output);
  *last_input = input;
  *last_output = output;
  return output;
}
