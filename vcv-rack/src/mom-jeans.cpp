#include "plugin.hpp"
#include "pulsar.h"
#include "mom_jeans_gen.h"

static float fclampf(float x, float a, float b) {
    return fmaxf(fminf(x, b), a);
}

static float scaleLin(float x, float a, float b, float c, float d) {
	return (x - a) / (b - a) * (d - c) + c;
}

struct PulsarOutput {
	float pulse;
	float sync;
};

struct MomJeansBase : Module {

	ps_t pulsar;

	enum ParamId {
		PITCH_PARAM,
		DENSITY_PARAM,
		TORQUE_PARAM,
		CADENCE_PARAM,
		COUPLING_PARAM,
		SHAPE_PARAM,
		QUANTIZATION_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		DENSITY_INPUT,
		SHAPE_INPUT,
		TORQUE_INPUT,
		CADENCE_INPUT,
		FM_INDEX_INPUT,
		LINEAR_FM_INPUT,
		V_OCT_INPUT,
		SYNC_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		OUTPUT_OUTPUT,
		TRIGGER_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		PITCH_LED_LIGHT,
		CADENCE_LED_LIGHT,
		LIGHTS_LEN
	};

	MomJeansBase() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(PITCH_PARAM, 0.f, 1.f, 0.f, "");
		configParam(DENSITY_PARAM, 0.f, 1.f, 0.f, "");
		configParam(TORQUE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(CADENCE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(COUPLING_PARAM, 0.f, 1.f, 0.f, "");
		configParam(SHAPE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(QUANTIZATION_PARAM, 0.f, 1.f, 0.f, "");
		configInput(DENSITY_INPUT, "");
		configInput(SHAPE_INPUT, "");
		configInput(TORQUE_INPUT, "");
		configInput(CADENCE_INPUT, "");
		configInput(FM_INDEX_INPUT, "");
		configInput(LINEAR_FM_INPUT, "");
		configInput(V_OCT_INPUT, "");
		configInput(SYNC_INPUT, "");
		configOutput(OUTPUT_OUTPUT, "");
		configOutput(TRIGGER_OUTPUT, "");
	}

	void onReset(const ResetEvent& e) override {
		Module::onReset(e);
		pulsar_init(&pulsar, APP->engine->getSampleRate());
	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override {
		Module::onSampleRateChange(e);
		pulsar_init(&pulsar, APP->engine->getSampleRate());
	}

	virtual PulsarOutput nextSample(
		float pulse_frequency,
		float density_ratio,
		float mod_ratio,
		float mod_depth,
		float waveform,
		uint8_t ratio_lock,
		uint8_t frequency_couple,
		uint8_t sync
	) = 0;

	void process(const ProcessArgs& args) override {
		// Read inputs
		float shape = inputs[SHAPE_INPUT].getVoltage();
		float torque = inputs[TORQUE_INPUT].getVoltage();
		float cadence = inputs[CADENCE_INPUT].getVoltage();
		float fm_index = inputs[FM_INDEX_INPUT].getVoltage();
		float linear_fm = inputs[LINEAR_FM_INPUT].getVoltage();
		float v_oct = inputs[V_OCT_INPUT].getVoltage();
		float sync = inputs[SYNC_INPUT].getVoltage();
		float density = inputs[DENSITY_INPUT].getVoltage();

		// Read parameters
		float pitch = params[PITCH_PARAM].getValue();
		float density_param = params[DENSITY_PARAM].getValue();
		float torque_param = params[TORQUE_PARAM].getValue();
		float cadence_param = params[CADENCE_PARAM].getValue();
		float shape_param = params[SHAPE_PARAM].getValue();
		float coupling = params[COUPLING_PARAM].getValue();
		float quantization = params[QUANTIZATION_PARAM].getValue();

		// Map inputs/parameters to pulsar configuration
		// void pulsar_configure(
		// 	ps_t *self,
		// 	float pulse_frequency,
		// 	float density_ratio,
		// 	float mod_ratio,
		// 	float mod_depth,
		// 	float waveform,
		// 	uint8_t ratio_lock,
		// 	uint8_t frequency_couple
		// );

		// If the pitch knob is less than 0.25, then it's a continuous LFO control from
		// 0.25 Hz up to 27.5 Hz, with quadratic scaling. Above that, it covers seven octaves from
		// A0 to A7

		float pulse_frequency = 0.f;
		if (pitch < 0.25f) {
			pitch = scaleLin(pitch, 0.0, 0.25f, -2.f, 4.78135971352f);
			pitch += v_oct + linear_fm * fm_index / 5.f;
			pulse_frequency = std::pow(2.f, pitch);
		} else {
			pitch = scaleLin(pitch, 0.25f, 1.f, 0, 84);
			pitch = std::roundf(pitch);
			pitch += (v_oct + linear_fm * fm_index / 5.f) * 12.0f;
			pulse_frequency = 27.5 * std::pow(2.f, pitch / 12.f);
		}

		float density_input_voltage = density + (density_param - 0.5) * 10.0f;
		if (!inputs[DENSITY_INPUT].isConnected()) {
			if (density_input_voltage < 0.1f && density_input_voltage > -0.1f) {
				density_input_voltage = 0.0f;
			}
		}

		float density_ratio = fclampf(density_input_voltage / 5.0f, -1.0f, 1.0f);
		float mod_ratio = fclampf(cadence / 5.0 + cadence_param, 0.0, 1.0);
		float mod_depth = fclampf(torque / 5.0 + torque_param, 0.0, 1.0);
		float waveform = fclampf(shape / 5.0 + shape_param, 0.0, 1.0);
		
		PulsarOutput pulsar_output = nextSample(
			pulse_frequency,
			density_ratio,
			mod_ratio,
			mod_depth,
			waveform,
			quantization > 0.5f ? 1 : 0,
			coupling > 0.5f ? 1 : 0,
			sync > 2.5f ? 1 : 0
		);

		outputs[TRIGGER_OUTPUT].setVoltage(pulsar_output.sync);
		outputs[OUTPUT_OUTPUT].setVoltage(pulsar_output.pulse);
	}
};

struct MomJeans : MomJeansBase {
	virtual PulsarOutput nextSample (
		float pulse_frequency,
		float density_ratio,
		float mod_ratio,
		float mod_depth,
		float waveform,
		uint8_t ratio_lock,
		uint8_t frequency_couple,
		uint8_t sync
	) override {
		pulsar_configure(
			&pulsar,
			pulse_frequency,
			density_ratio,
			mod_ratio,
			mod_depth,
			waveform,
			ratio_lock,
			frequency_couple
		);

		// Process pulsar
		float pulse = pulsar_process(&pulsar, pulse_frequency, sync);

		return { pulse, pulsar_get_debug_value(&pulsar) };
	}
};

struct MomJeansGen : MomJeansBase {

	CommonState *gen = 0;
	t_sample **inputBuffers;
	t_sample **outputBuffers;
	
	MomJeansGen() {
		gen = (CommonState *) mom_jeans_gen::create(APP->engine->getSampleRate(), 1); // sample by sample

		inputBuffers = new t_sample*[mom_jeans_gen::num_inputs()];
		outputBuffers = new t_sample*[mom_jeans_gen::num_outputs()];

		for(int i = 0; i < mom_jeans_gen::num_inputs(); i++) {
			inputBuffers[i] = new t_sample[1];
		}
		for(int i = 0; i < mom_jeans_gen::num_outputs(); i++) {
			outputBuffers[i] = new t_sample[1];
		}
	}

	~MomJeansGen() {
		for(int i = 0; i < mom_jeans_gen::num_inputs(); i++) {
			delete[] inputBuffers[i];
		}
		for(int i = 0; i < mom_jeans_gen::num_outputs(); i++) {
			delete[] outputBuffers[i];
		}

		delete[] inputBuffers;
		delete[] outputBuffers;

		mom_jeans_gen::destroy(gen);
	}

	virtual PulsarOutput nextSample (
		float pulse_frequency,
		float density_ratio,
		float mod_ratio,
		float mod_depth,
		float waveform,
		uint8_t ratio_lock,
		uint8_t frequency_couple,
		uint8_t sync
	) override {

		// Set inputs
		inputBuffers[0][0] = pulse_frequency;
		inputBuffers[1][0] = density_ratio;
		inputBuffers[2][0] = mod_ratio;
		inputBuffers[3][0] = mod_depth;
		inputBuffers[4][0] = waveform;
		inputBuffers[5][0] = ratio_lock;
		inputBuffers[6][0] = frequency_couple;
		inputBuffers[7][0] = sync;

		mom_jeans_gen::perform(
			gen,
			inputBuffers, mom_jeans_gen::num_inputs(),
			outputBuffers, mom_jeans_gen::num_outputs(),
			1
		);

		return { (float) outputBuffers[0][0], (float) outputBuffers[1][0] };
	}
};

struct Mom_jeansWidget : ModuleWidget {
	Mom_jeansWidget(MomJeansBase* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/mom-jeans.svg")));

		// addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		// addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(9.574, 20.288)), module, MomJeansBase::PITCH_PARAM));
		addParam(createParam<RoundBigBlackKnob>(mm2px(Vec(21.462, 16.097)), module, MomJeansBase::DENSITY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(29.001, 44.915)), module, MomJeansBase::TORQUE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(14.175, 49.759)), module, MomJeansBase::CADENCE_PARAM));
		addParam(createParamCentered<NKK>(mm2px(Vec(7.996, 62.614)), module, MomJeansBase::COUPLING_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(23.201, 68.618)), module, MomJeansBase::SHAPE_PARAM));
		addParam(createParamCentered<NKK>(mm2px(Vec(7.996, 73.101)), module, MomJeansBase::QUANTIZATION_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(17.801, 34.734)), module, MomJeansBase::DENSITY_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(32.000, 81.248)), module, MomJeansBase::SHAPE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19.999, 83.249)), module, MomJeansBase::TORQUE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.996, 85.150)), module, MomJeansBase::CADENCE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(32.000, 94.248)), module, MomJeansBase::FM_INDEX_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19.999, 96.248)), module, MomJeansBase::LINEAR_FM_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.108, 98.149)), module, MomJeansBase::V_OCT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.996, 111.499)), module, MomJeansBase::SYNC_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(32.000, 107.251)), module, MomJeansBase::OUTPUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(19.999, 109.123)), module, MomJeansBase::TRIGGER_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(17.267, 12.607)), module, MomJeansBase::PITCH_LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(6.510, 42.485)), module, MomJeansBase::CADENCE_LED_LIGHT));
	}
};


Model* modelMom_jeans = createModel<MomJeans, Mom_jeansWidget>("mom-jeans");
Model* modelMom_jeans_gen = createModel<MomJeansGen, Mom_jeansWidget>("mom-jeans-gen");