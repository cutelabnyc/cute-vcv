#include "plugin.hpp"
// TODO: Add these header files when they exist
// #include "cacio_e_pepe.h"
#include "cacio_e_pepe_gen.h"

struct CacioEPepeOutput {
	float gate_out;
	float division_out;
	float blink_out;
};

enum CacioEPepeMode {
	MODE_QUADRUPLET,
	MODE_QUINTUPLET,
	MODE_TRIPLET
};

struct CacioEPepeBase : Module {
	enum ParamId {
		MODE_PARAM,  // Now a tri-state toggle
		MODE_SWITCH_PARAM,  // This is now a momentary button
		QUARTER_PARAM,
		EIGHTH_PARAM,
		SIXTEENTH_PARAM,
		THIRTYSECOND_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		CLOCK_INPUT,
		GATE_INPUT,
		ALIGN_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		GATE_OUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		MODE_LED_LIGHT,
		QUARTER_LED_LIGHT,
		EIGHTH_LED_LIGHT,
		SIXTEENTH_LED_LIGHT,
		THIRTYSECOND_LED_LIGHT,
		LIGHTS_LEN
	};

	// Store previous state for edge detection
	bool prevModeSwitch = false;

	CacioEPepeBase() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		// Configure tri-state toggle (0 = triplet, 1 = quintuplet, 2 = quadruplet)
		configParam(MODE_PARAM, 0.f, 2.f, 0.f, "Mode", "");
		// Configure as trigger button
		configParam(MODE_SWITCH_PARAM, 0.f, 1.f, 0.f, "Mode Switch", "");
		configParam(QUARTER_PARAM, 0.f, 1.f, 0.f, "");
		configParam(EIGHTH_PARAM, 0.f, 1.f, 0.f, "");
		configParam(SIXTEENTH_PARAM, 0.f, 1.f, 0.f, "");
		configParam(THIRTYSECOND_PARAM, 0.f, 1.f, 0.f, "");
		configInput(CLOCK_INPUT, "");
		configInput(GATE_INPUT, "");
		configInput(ALIGN_INPUT, "");
		configOutput(GATE_OUT_OUTPUT, "");
	}

	virtual void onReset(const ResetEvent& e) override {
		Module::onReset(e);
		prevModeSwitch = false;
		// Add any reset logic here
	}

	virtual void onSampleRateChange(const SampleRateChangeEvent& e) override {
		Module::onSampleRateChange(e);
		// Add any sample rate change logic here
	}

	// This is the interface that both implementations will need to implement
	virtual CacioEPepeOutput nextSample(
		bool modeSwitchPressed,
		CacioEPepeMode mode,
		float quarter_prob,
		float eighth_prob,
		float sixteenth_prob,
		float thirtysecond_prob,
		float clock_voltage,
		float gate_voltage,
		float align_voltage,
		bool clock_connected,
		bool gate_connected
	) = 0;

	void process(const ProcessArgs& args) override {
		// Handle mode switch with edge detection
		bool currentModeSwitch = params[MODE_SWITCH_PARAM].getValue() > 0.f;
		bool modeSwitchPressed = currentModeSwitch && !prevModeSwitch;
		prevModeSwitch = currentModeSwitch;

		// Get the current mode
		int modeInt = (int)std::round(params[MODE_PARAM].getValue());
		CacioEPepeMode mode = static_cast<CacioEPepeMode>(modeInt);

		// Get the next sample from the implementation
		CacioEPepeOutput output = nextSample(
			modeSwitchPressed,
			mode,
			params[QUARTER_PARAM].getValue(),
			params[EIGHTH_PARAM].getValue(),
			params[SIXTEENTH_PARAM].getValue(),
			params[THIRTYSECOND_PARAM].getValue(),
			inputs[CLOCK_INPUT].getVoltage(),
			inputs[GATE_INPUT].getVoltage(),
			inputs[ALIGN_INPUT].getVoltage(),
			inputs[CLOCK_INPUT].isConnected(),
			inputs[GATE_INPUT].isConnected()
		);
		
		// Process the output
		outputs[GATE_OUT_OUTPUT].setVoltage(output.gate_out);

		lights[QUARTER_LED_LIGHT].setBrightness(output.division_out == 1.0f ? output.gate_out : 0.0f);
		lights[EIGHTH_LED_LIGHT].setBrightness(output.division_out == 2.0f ? output.gate_out : 0.0f);
		lights[SIXTEENTH_LED_LIGHT].setBrightness(output.division_out == 3.0f ? output.gate_out : 0.0f);
		lights[THIRTYSECOND_LED_LIGHT].setBrightness(output.division_out == 4.0f ? output.gate_out	 : 0.0f);
		lights[MODE_LED_LIGHT].setBrightness(output.blink_out);

		if (!inputs[CLOCK_INPUT].isConnected() && !inputs[GATE_INPUT].isConnected()) {
			lights[MODE_LED_LIGHT].setBrightness(0.0f);
		}
	}
};

// The "plain C" implementation
struct CacioEPepe : CacioEPepeBase {
	virtual CacioEPepeOutput nextSample(
		bool modeSwitchPressed,
		CacioEPepeMode mode,
		float quarter_prob,
		float eighth_prob,
		float sixteenth_prob,
		float thirtysecond_prob,
		float clock_voltage,
		float gate_voltage,
		float align_voltage,
		bool clock_connected,
		bool gate_connected
	) override {
		// TODO: Implement the plain C version
		return { 0.0f, 0.0f, 0.0f };
	}
};

// The Gen implementation
struct CacioEPepeGen : CacioEPepeBase {
	// Gen-specific state
	CommonState *gen = 0;
	t_sample **inputBuffers;
	t_sample **outputBuffers;

	CacioEPepeGen() {
		gen = (CommonState *) cacio_e_pepe_gen::create(APP->engine->getSampleRate(), 1);
				gen = (CommonState *) cacio_e_pepe_gen::create(APP->engine->getSampleRate(), 1); // sample by sample

		inputBuffers = new t_sample*[cacio_e_pepe_gen::num_inputs()];
		outputBuffers = new t_sample*[cacio_e_pepe_gen::num_outputs()];

		for(int i = 0; i < cacio_e_pepe_gen::num_inputs(); i++) {
			inputBuffers[i] = new t_sample[1];
		}
		for(int i = 0; i < cacio_e_pepe_gen::num_outputs(); i++) {
			outputBuffers[i] = new t_sample[1];
		}
	}

	~CacioEPepeGen() {
		for(int i = 0; i < cacio_e_pepe_gen::num_inputs(); i++) {
			delete[] inputBuffers[i];
		}
		for(int i = 0; i < cacio_e_pepe_gen::num_outputs(); i++) {
			delete[] outputBuffers[i];
		}

		delete[] inputBuffers;
		delete[] outputBuffers;

		cacio_e_pepe_gen::destroy(gen);
	}

	virtual CacioEPepeOutput nextSample(
		bool modeSwitchPressed,
		CacioEPepeMode mode,
		float quarter_prob,
		float eighth_prob,
		float sixteenth_prob,
		float thirtysecond_prob,
		float clock_voltage,
		float gate_voltage,
		float align_voltage,
		bool clock_connected,
		bool gate_connected
	) override {
		inputBuffers[0][0] = clock_voltage;
		inputBuffers[1][0] = gate_voltage;
		inputBuffers[2][0] = align_voltage;
		inputBuffers[3][0] = quarter_prob;
		inputBuffers[4][0] = eighth_prob;
		inputBuffers[5][0] = sixteenth_prob;
		inputBuffers[6][0] = thirtysecond_prob;
		inputBuffers[7][0] = mode;

		// If the clock is not connected but the gate is, then tempo information comes from the gate input
		cacio_e_pepe_gen::setparameter(gen, 0, (!clock_connected && gate_connected) ? 1 : 0, NULL);

		// If the gate is not connected, then triggers information comes from the gate input
		cacio_e_pepe_gen::setparameter(gen, 1, (gate_connected) ? 1 : 0, NULL);

		cacio_e_pepe_gen::perform(
			gen,
			inputBuffers, cacio_e_pepe_gen::num_inputs(),
			outputBuffers, cacio_e_pepe_gen::num_outputs(),
			1
		);

		return { (float) outputBuffers[0][0], (float) outputBuffers[1][0], (float) outputBuffers[2][0] };
	}
};

struct Cacio_e_pepeWidget : ModuleWidget {
	Cacio_e_pepeWidget(CacioEPepeBase* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/cacio-e-pepe.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		// addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		// addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// Changed to CKSSThree tri-state toggle
		addParam(createParamCentered<CKSSThree>(mm2px(Vec(16.375, 17.838)), module, CacioEPepeBase::MODE_PARAM));
		// Changed to TL1105 small tactile button
		addParam(createParamCentered<TL1105>(mm2px(Vec(3.976, 18.044)), module, CacioEPepeBase::MODE_SWITCH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(12.162, 34.05)), module, CacioEPepeBase::QUARTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(12.162, 51.051)), module, CacioEPepeBase::EIGHTH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(12.162, 67.55)), module, CacioEPepeBase::SIXTEENTH_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(12.162, 84.552)), module, CacioEPepeBase::THIRTYSECOND_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.402, 99.25)), module, CacioEPepeBase::CLOCK_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(15.206, 99.209)), module, CacioEPepeBase::GATE_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.402, 112.261)), module, CacioEPepeBase::ALIGN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(15.203, 112.251)), module, CacioEPepeBase::GATE_OUT_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.976, 12.544)), module, CacioEPepeBase::MODE_LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.603, 26.592)), module, CacioEPepeBase::QUARTER_LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.603, 43.553)), module, CacioEPepeBase::EIGHTH_LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.603, 60.096)), module, CacioEPepeBase::SIXTEENTH_LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(3.603, 77.064)), module, CacioEPepeBase::THIRTYSECOND_LED_LIGHT));
	}
};

Model* modelCacio_e_pepe = createModel<CacioEPepe, Cacio_e_pepeWidget>("cacio-e-pepe");
Model* modelCacio_e_pepe_gen = createModel<CacioEPepeGen, Cacio_e_pepeWidget>("cacio-e-pepe-gen");