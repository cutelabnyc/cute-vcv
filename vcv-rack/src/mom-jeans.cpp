#include "plugin.hpp"
#include "pulsar.h"

static float fclampf(float x, float a, float b) {
    return fmaxf(fminf(x, b), a);
}

static float scaleLin(float x, float a, float b, float c, float d) {
	return (x - a) / (b - a) * (d - c) + c;
}

struct PulsarOutput {
	float pulse;
	float sync;
	float internal_lfo;
};

static float denormalizedPitch(float pitch, float minScaleValue, float maxScaleValue) {
	pitch = scaleLin(pitch, 0.f, 1.f, minScaleValue, maxScaleValue);
	return 27.5 * powf(2.f, pitch / 12.f);
}

struct FrequencyParamQuantity : ParamQuantity {
	float _minScaleValue = 0.f;
	float _maxScaleValue = 84.f;

	void setScaleRange(float minValue, float maxValue) {
		_minScaleValue = minValue;
		_maxScaleValue = maxValue;
	}

    std::string getDisplayValueString() override {
        float value = getValue();
		float displayValue = denormalizedPitch(value, _minScaleValue, _maxScaleValue);
        return string::f("%.1f", displayValue);
    }
    
    std::string getLabel() override {
        return "Pitch";
    }
    
    std::string getUnit() override {
        return "Hz"; // or whatever unit you want
    }
};

struct MomJeansBase : Module {

	ps_t pulsar;
	FrequencyParamQuantity *frequencyParamQuantity;

	enum ParamId {
		PITCH_PARAM,
		DENSITY_PARAM,
		TORQUE_PARAM,
		CADENCE_PARAM,
		COUPLING_PARAM,
		SHAPE_PARAM,
		QUANTIZATION_PARAM,
		PITCH_MODE_PARAM,
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
		COUPLING_LED_LIGHT,
		QUANTIZATION_LED_LIGHT,
		PITCH_MODE_LED_LIGHT,
		LIGHTS_LEN
	};

	MomJeansBase() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(PITCH_PARAM, 0.f, 1.f, 0.25f, "Pitch");
		frequencyParamQuantity = new FrequencyParamQuantity();
		paramQuantities[PITCH_PARAM] = frequencyParamQuantity;
		paramQuantities[PITCH_PARAM]->name = "Pitch";
		paramQuantities[PITCH_PARAM]->module = this;
		paramQuantities[PITCH_PARAM]->paramId = PITCH_PARAM;

		configParam(DENSITY_PARAM, 0.f, 100.f, 0.f, "Density");
		paramQuantities[DENSITY_PARAM]->name = "Density";
		paramQuantities[DENSITY_PARAM]->unit = "%";

		configParam(TORQUE_PARAM, 0.f, 100.f, 0.f, "Torque");
		paramQuantities[TORQUE_PARAM]->name = "Torque";
		paramQuantities[TORQUE_PARAM]->unit = "%";

		configParam(CADENCE_PARAM, 0.f, 100.f, 0.f, "Cadence");
		paramQuantities[CADENCE_PARAM]->name = "Cadence";
		paramQuantities[CADENCE_PARAM]->unit = "%";

		configParam(COUPLING_PARAM, 0.f, 1.f, 0.f, "Coupling");
		paramQuantities[COUPLING_PARAM]->name = "Coupling";
		paramQuantities[COUPLING_PARAM]->unit = "";

		configParam(SHAPE_PARAM, 0.f, 5.f, 0.f, "Shape");
		paramQuantities[SHAPE_PARAM]->name = "Shape";
		paramQuantities[SHAPE_PARAM]->unit = "";

		configParam(QUANTIZATION_PARAM, 0.f, 1.f, 0.f, "Quantization");
		paramQuantities[QUANTIZATION_PARAM]->name = "Quantization";
		paramQuantities[QUANTIZATION_PARAM]->unit = "";

		configParam(PITCH_MODE_PARAM, 0.f, 1.f, 0.f, "Extended Pitch Range");
		paramQuantities[PITCH_MODE_PARAM]->name = "Extended Pitch Range";
		paramQuantities[PITCH_MODE_PARAM]->unit = "";

		configInput(DENSITY_INPUT, "Density");
		configInput(SHAPE_INPUT, "Shape");
		configInput(TORQUE_INPUT, "Torque");
		configInput(CADENCE_INPUT, "Cadence");
		configInput(FM_INDEX_INPUT, "FM Index");
		configInput(LINEAR_FM_INPUT, "Linear FM");
		configInput(V_OCT_INPUT, "V/Oct");
		configInput(SYNC_INPUT, "Sync");
		configOutput(OUTPUT_OUTPUT, "Output");
		configOutput(TRIGGER_OUTPUT, "Trigger");
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

		if (!inputs[FM_INDEX_INPUT].isConnected()) {
			fm_index = 1.0f;
		}

		// Read parameters
		float pitch = params[PITCH_PARAM].getValue();
		float density_param = params[DENSITY_PARAM].getValue() / 100.f;
		float torque_param = params[TORQUE_PARAM].getValue() / 100.f;
		float cadence_param = params[CADENCE_PARAM].getValue() / 100.f;
		float shape_param = params[SHAPE_PARAM].getValue() / 5.f;

		float coupling = params[COUPLING_PARAM].getValue();
		float quantization = params[QUANTIZATION_PARAM].getValue();
		float pitch_mode = params[PITCH_MODE_PARAM].getValue();

		lights[COUPLING_LED_LIGHT].setBrightness((coupling > 0.5f) ? 1.0f : 0.0f);
		lights[QUANTIZATION_LED_LIGHT].setBrightness((quantization > 0.5f) ? 1.0f : 0.0f);
		lights[PITCH_MODE_LED_LIGHT].setBrightness((pitch_mode > 0.5f) ? 1.0f : 0.0f);

		// Simply scale the pitch, since the hardware DAC can't do LFO values
		float lfo_cutoff = 0.0f;
		float pulse_frequency = 0.f;
		float pitch_min = 36.f;
		float pitch_max = 60.f;
		if (pitch_mode > 0.5f) {
			pitch_min = 0.f;
			pitch_max = 84.f;
		}

		frequencyParamQuantity->setScaleRange(pitch_min, pitch_max);
		pitch = scaleLin(pitch, lfo_cutoff, 1.f, pitch_min, pitch_max);
		pitch += v_oct * 12.0f;
		pitch = 27.5 * powf(2.f, pitch / 12.f);

		// apply linear fm
		pitch += linear_fm * fm_index * pitch * 0.2f;
		pulse_frequency = fclampf(pitch, 1.0f, 20000.0f);

		float density_input_voltage = density + (density_param - 0.5) * 10.0f;
		if (!inputs[DENSITY_INPUT].isConnected()) {
			if (density_input_voltage < 0.1f && density_input_voltage > -0.1f) {
				density_input_voltage = 0.0f;
			}
		}

		float density_ratio = fclampf(density_input_voltage / 5.0f, -1.0f, 1.0f);
		float mod_ratio = fclampf(cadence / 5.0 + cadence_param, 0.0, 1.0);
		float mod_depth = fclampf(torque / 5.0 + torque_param, 0.0, 1.0);
		float waveform = fclampf(shape / 5.0 + shape_param, 0.0, 1.0) * 4.9999f;;
		
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

		outputs[TRIGGER_OUTPUT].setVoltage(pulsar_output.sync * 10.0f);
		outputs[OUTPUT_OUTPUT].setVoltage(pulsar_output.pulse * 10.0f);

		// lights[PITCH_LED_LIGHT].setBrightness(fclampf(pulsar_output.pulse, 0.0f, 1.0f));
		lights[CADENCE_LED_LIGHT].setBrightness(fclampf(pulsar_output.internal_lfo, 0.0f, 1.0f));
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
		float debug_value = 0.0f;
		float pulse = pulsar_process(&pulsar, pulse_frequency, sync, &debug_value);
		float sync_output = pulsar_get_sync_output(&pulsar);
		float internal_lfo = sinf(pulsar_get_internal_lfo_phase(&pulsar) * 2.0f * M_PI);

		return { pulse, sync_output, internal_lfo };
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

		return { (float) outputBuffers[0][0], (float) outputBuffers[1][0], 0.0f };
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

		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(19.477, 12.053)), module, MomJeansBase::PITCH_MODE_PARAM, MomJeansBase::PITCH_MODE_LED_LIGHT));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.329, 19.253)), module, MomJeansBase::PITCH_PARAM));
		addParam(createParamCentered<RoundBigBlackKnob>(mm2px(Vec(31.296, 22.843)), module, MomJeansBase::DENSITY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(31.374, 44.988)), module, MomJeansBase::TORQUE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(13.963, 49.862)), module, MomJeansBase::CADENCE_PARAM));
		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(8.795, 62.626)), module, MomJeansBase::COUPLING_PARAM, MomJeansBase::COUPLING_LED_LIGHT));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(23.967, 68.655)), module, MomJeansBase::SHAPE_PARAM));
		addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(8.795, 73.254)), module, MomJeansBase::QUANTIZATION_PARAM, MomJeansBase::QUANTIZATION_LED_LIGHT));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19.785, 33.45)), module, MomJeansBase::DENSITY_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(32.793, 82.255)), module, MomJeansBase::SHAPE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20.821, 84.253)), module, MomJeansBase::TORQUE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.795, 86.255)), module, MomJeansBase::CADENCE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(32.793, 95.253)), module, MomJeansBase::FM_INDEX_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20.821, 97.255)), module, MomJeansBase::LINEAR_FM_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.795, 99.253)), module, MomJeansBase::V_OCT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.795, 112.254)), module, MomJeansBase::SYNC_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(32.793, 108.255)), module, MomJeansBase::OUTPUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.821, 110.253)), module, MomJeansBase::TRIGGER_OUTPUT));

		// addChild(createLightCentered<SmallLight<YellowLight>>(mm2px(Vec(17.267, 12.607)), module, MomJeansBase::PITCH_LED_LIGHT));
		addChild(createLightCentered<SmallLight<YellowLight>>(mm2px(Vec(6.25, 42.25)), module, MomJeansBase::CADENCE_LED_LIGHT));
	}
};


Model* modelMom_jeans = createModel<MomJeans, Mom_jeansWidget>("mom-jeans");
Model* modelMom_jeans_gen = createModel<MomJeansGen, Mom_jeansWidget>("mom-jeans-gen");