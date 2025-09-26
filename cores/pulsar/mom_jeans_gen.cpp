#include "mom_jeans_gen.h"

namespace mom_jeans_gen {

/****************************************************************************************
Copyright (c) 2023 Cycling '74

The code that Max generates automatically and that end users are capable of
exporting and using, and any associated documentation files (the “Software”)
is a work of authorship for which Cycling '74 is the author and owner for
copyright purposes.

This Software is dual-licensed either under the terms of the Cycling '74
License for Max-Generated Code for Export, or alternatively under the terms
of the General Public License (GPL) Version 3. You may use the Software
according to either of these licenses as it is most appropriate for your
project on a case-by-case basis (proprietary or not).

A) Cycling '74 License for Max-Generated Code for Export

A license is hereby granted, free of charge, to any person obtaining a copy
of the Software (“Licensee”) to use, copy, modify, merge, publish, and
distribute copies of the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The Software is licensed to Licensee for all uses that do not include the sale,
sublicensing, or commercial distribution of software that incorporates this
source code. This means that the Licensee is free to use this software for
educational, research, and prototyping purposes, to create musical or other
creative works with software that incorporates this source code, or any other
use that does not constitute selling software that makes use of this source
code. Commercial distribution also includes the packaging of free software with
other paid software, hardware, or software-provided commercial services.

For entities with UNDER 200k USD in annual revenue or funding, a license is hereby
granted, free of charge, for the sale, sublicensing, or commercial distribution
of software that incorporates this source code, for as long as the entity's
annual revenue remains below 200k USD annual revenue or funding.

For entities with OVER 200k USD in annual revenue or funding interested in the
sale, sublicensing, or commercial distribution of software that incorporates
this source code, please send inquiries to licensing (at) cycling74.com.

The above copyright notice and this license shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Please see
https://support.cycling74.com/hc/en-us/articles/360050779193-Gen-Code-Export-Licensing-FAQ
for additional information

B) General Public License Version 3 (GPLv3)
Details of the GPLv3 license can be found at: https://www.gnu.org/licenses/gpl-3.0.html
****************************************************************************************/

// global noise generator
Noise noise;
static const int GENLIB_LOOPCOUNT_BAIL = 2000000;


// The State struct contains all the state and procedures for the gendsp kernel
typedef struct State {
	CommonState __commonstate;
	Change __m_change_19;
	Change __m_change_17;
	DCBlock __m_dcblock_9;
	Data m_ola_1;
	Phasor __m_phasor_18;
	SineCycle __m_cycle_26;
	SineData __sinedata;
	int __exception;
	int __loopcount;
	int vectorsize;
	t_sample __m_carry_4;
	t_sample __m_slide_14;
	t_sample samplerate;
	t_sample samples_to_seconds;
	t_sample __m_count_2;
	t_sample __m_slide_11;
	// re-initialize all member variables;
	inline void reset(t_param __sr, int __vs) {
		__exception = 0;
		vectorsize = __vs;
		samplerate = __sr;
		m_ola_1.reset("ola", samplerate, ((int)1));
		__m_count_2 = 0;
		__m_carry_4 = 0;
		__m_dcblock_9.reset();
		__m_slide_11 = 0;
		__m_slide_14 = 0;
		__m_change_17.reset(0);
		samples_to_seconds = (1 / samplerate);
		__m_phasor_18.reset(0);
		__m_change_19.reset(0);
		__m_cycle_26.reset(samplerate, 0);
		genlib_reset_complete(this);
		
	};
	// the signal processing routine;
	inline int perform(t_sample ** __ins, t_sample ** __outs, int __n) {
		vectorsize = __n;
		const t_sample * __in1 = __ins[0];
		const t_sample * __in2 = __ins[1];
		const t_sample * __in3 = __ins[2];
		const t_sample * __in4 = __ins[3];
		const t_sample * __in5 = __ins[4];
		const t_sample * __in6 = __ins[5];
		const t_sample * __in7 = __ins[6];
		const t_sample * __in8 = __ins[7];
		t_sample * __out1 = __outs[0];
		t_sample * __out2 = __outs[1];
		if (__exception) {
			return __exception;
			
		} else if (( (__in1 == 0) || (__in2 == 0) || (__in3 == 0) || (__in4 == 0) || (__in5 == 0) || (__in6 == 0) || (__in7 == 0) || (__in8 == 0) || (__out1 == 0) || (__out2 == 0) )) {
			__exception = GENLIB_ERR_NULL_BUFFER;
			return __exception;
			
		};
		int ola_dim = m_ola_1.dim;
		int ola_channels = m_ola_1.channels;
		int ola_dim_801 = ola_dim;
		int min_10 = (-1);
		t_sample iup_12 = (1 / maximum(1, abs(((int)500))));
		t_sample idown_13 = (1 / maximum(1, abs(((int)500))));
		t_sample iup_15 = (1 / maximum(1, abs(((int)1000))));
		t_sample idown_16 = (1 / maximum(1, abs(((int)1000))));
		samples_to_seconds = (1 / samplerate);
		int choice_24 = ((int)1);
		__loopcount = (__n * GENLIB_LOOPCOUNT_BAIL);
		// the main sample loop;
		while ((__n--)) {
			const t_sample in1 = (*(__in1++));
			const t_sample in2 = (*(__in2++));
			const t_sample in3 = (*(__in3++));
			const t_sample in4 = (*(__in4++));
			const t_sample in5 = (*(__in5++));
			const t_sample in6 = (*(__in6++));
			const t_sample in7 = (*(__in7++));
			const t_sample in8 = (*(__in8++));
			__m_count_2 = (((int)0) ? 0 : (fixdenorm(__m_count_2 + ((int)1))));
			int carry_3 = 0;
			if ((((int)0) != 0)) {
				__m_count_2 = 0;
				__m_carry_4 = 0;
				
			} else if (((ola_dim_801 > 0) && (__m_count_2 >= ola_dim_801))) {
				int wraps_5 = (__m_count_2 / ola_dim_801);
				__m_carry_4 = (__m_carry_4 + wraps_5);
				__m_count_2 = (__m_count_2 - (wraps_5 * ola_dim_801));
				carry_3 = 1;
				
			};
			int counter_796 = __m_count_2;
			int counter_797 = carry_3;
			int counter_798 = __m_carry_4;
			bool index_ignore_6 = ((counter_796 >= ola_dim) || (counter_796 < 0));
			// samples ola channel 1;
			t_sample sample_ola_799 = (index_ignore_6 ? 0 : m_ola_1.read(counter_796, 0));
			t_sample index_ola_800 = counter_796;
			int index_trunc_7 = fixnan(floor(index_ola_800));
			bool index_ignore_8 = ((index_trunc_7 >= ola_dim) || (index_trunc_7 < 0));
			if ((!index_ignore_8)) {
				m_ola_1.write(((int)0), index_trunc_7, 0);
				
			};
			t_sample clamp_765 = ((in5 <= ((int)0)) ? ((int)0) : ((in5 >= ((int)1)) ? ((int)1) : in5));
			t_sample mul_764 = (clamp_765 * ((int)2));
			t_sample dcblock_789 = __m_dcblock_9(sample_ola_799);
			t_sample tanh_780 = tanh(dcblock_789);
			t_sample out1 = tanh_780;
			t_sample clamp_763 = ((in2 <= min_10) ? min_10 : ((in2 >= ((int)1)) ? ((int)1) : in2));
			t_sample clamp_1457 = ((in1 <= ((int)1)) ? ((int)1) : ((in1 >= ((int)8000)) ? ((int)8000) : in1));
			__m_slide_11 = fixdenorm((__m_slide_11 + (((clamp_1457 > __m_slide_11) ? iup_12 : idown_13) * (clamp_1457 - __m_slide_11))));
			t_sample slide_781 = __m_slide_11;
			t_sample sub_1566 = (slide_781 - ((int)65));
			t_sample scale_1563 = ((safepow((sub_1566 * ((t_sample)0.00032562683165093)), ((int)1)) * ((t_sample)3.3)) + ((t_sample)0.2));
			__m_slide_14 = fixdenorm((__m_slide_14 + (((clamp_763 > __m_slide_14) ? iup_15 : idown_16) * (clamp_763 - __m_slide_14))));
			t_sample slide_745 = __m_slide_14;
			t_sample sub_1570 = (slide_745 - (-1));
			t_sample scale_1567 = ((safepow((sub_1570 * ((t_sample)0.5)), ((int)2)) * ((t_sample)-0.95)) + ((int)1));
			t_sample sub_1574 = (in4 - ((int)0));
			t_sample scale_1571 = ((safepow((sub_1574 * ((t_sample)1)), ((int)2)) * ((int)36)) + ((int)4));
			t_sample orange_1577 = (scale_1571 - ((t_sample)1.2));
			t_sample sub_1578 = (slide_781 - ((int)65));
			t_sample scale_1575 = ((safepow((sub_1578 * ((t_sample)0.00032562683165093)), ((int)1)) * orange_1577) + ((t_sample)1.2));
			t_sample orange_1581 = (scale_1563 - scale_1575);
			t_sample sub_1582 = (slide_745 - ((int)-1));
			t_sample scale_1579 = ((safepow((sub_1582 * ((t_sample)0.5)), ((t_sample)1.5)) * orange_1581) + scale_1575);
			t_sample sub_1586 = (scale_1567 - ((int)0));
			t_sample scale_1583 = ((safepow((sub_1586 * ((t_sample)1)), ((int)1)) * ((int)17)) + ((int)3));
			t_sample mul_758 = (slide_745 * ((int)10));
			t_sample pow_761 = safepow(((t_sample)1.15), mul_758);
			t_sample mul_759 = (slide_781 * pow_761);
			t_sample rdiv_788 = safediv(samplerate, mul_759);
			int gt_756 = (in8 > ((int)0));
			int change_757 = __m_change_17(gt_756);
			int eq_755 = (change_757 == ((int)1));
			if ((eq_755 != 0)) {
				__m_phasor_18.phase = 0;
				
			};
			t_sample phasor_793 = __m_phasor_18(slide_781, samples_to_seconds);
			int lt_762 = (phasor_793 < ((t_sample)0.5));
			t_sample out2 = lt_762;
			int change_792 = __m_change_19(lt_762);
			int eq_791 = (change_792 == ((int)1));
			t_sample add_976 = (in7 + ((int)1));
			t_sample sub_1590 = (in4 - ((int)0));
			t_sample scale_1587 = ((safepow((sub_1590 * ((t_sample)1.3333333333333)), ((int)1)) * ((int)-1)) + ((int)0));
			t_sample sub_1594 = (in4 - ((t_sample)0.75));
			t_sample scale_1591 = ((safepow((sub_1594 * ((t_sample)4)), ((int)1)) * ((int)-9)) + (-1));
			t_sample sub_1598 = (in4 - ((int)0));
			t_sample scale_1595 = ((safepow((sub_1598 * ((t_sample)1.3333333333333)), ((int)1)) * ((int)1)) + ((int)0));
			t_sample sub_1602 = (in4 - ((t_sample)0.75));
			t_sample scale_1599 = ((safepow((sub_1602 * ((t_sample)4)), ((int)1)) * ((int)10)) + ((int)0));
			t_sample sub_1606 = (in4 - ((int)0));
			t_sample scale_1603 = ((safepow((sub_1606 * ((t_sample)1)), ((int)3)) * ((int)10)) + ((int)0));
			int gt_971 = (in4 > ((t_sample)0.75));
			int add_970 = (gt_971 + ((int)1));
			int choice_20 = add_970;
			t_sample selector_972 = ((choice_20 >= 2) ? scale_1591 : ((choice_20 >= 1) ? scale_1587 : 0));
			int gt_966 = (in4 > ((t_sample)0.75));
			int add_965 = (gt_966 + ((int)1));
			int choice_21 = add_965;
			t_sample selector_967 = ((choice_21 >= 2) ? scale_1599 : ((choice_21 >= 1) ? scale_1595 : 0));
			t_sample div_980 = (slide_781 * ((t_sample)0.001));
			t_sample mul_979 = (div_980 * scale_1603);
			t_sample choice_22 = int(add_976);
			t_sample selector_974 = ((choice_22 >= 2) ? mul_979 : ((choice_22 >= 1) ? selector_967 : 0));
			t_sample mul_973 = (mul_979 * (-1));
			t_sample choice_23 = int(add_976);
			t_sample selector_978 = ((choice_23 >= 2) ? mul_973 : ((choice_23 >= 1) ? selector_972 : 0));
			t_sample clamp_779 = ((in6 <= ((int)0)) ? ((int)0) : ((in6 >= ((int)1)) ? ((int)1) : in6));
			t_sample clamp_785 = ((in3 <= ((int)0)) ? ((int)0) : ((in3 >= ((int)1)) ? ((int)1) : in3));
			t_sample expr_897 = ((int)0);
			t_sample tf = ((int)1);
			if ((clamp_785 < ((t_sample)0.25))) {
				expr_897 = ((clamp_785 * ((t_sample)4)) * ((int)10));
				
			} else {
				if ((clamp_785 < ((t_sample)0.5))) {
					tf = ((clamp_785 - ((t_sample)0.25)) * ((t_sample)4));
					expr_897 = (((((int)1) - tf) * ((int)10)) + ((tf * slide_781) * ((t_sample)0.1)));
					
				} else {
					t_sample fac1 = ((((t_sample)0.9) * (clamp_785 - ((t_sample)0.5))) * ((t_sample)2));
					expr_897 = ((fac1 + ((t_sample)0.1)) * slide_781);
					
				};
				
			};
			t_sample sub_1610 = (clamp_785 - ((int)0));
			t_sample scale_1607 = ((safepow((sub_1610 * ((t_sample)1)), ((t_sample)2.2)) * ((int)1000)) + ((int)0));
			t_sample selector_894 = ((choice_24 >= 2) ? expr_897 : ((choice_24 >= 1) ? scale_1607 : 0));
			t_sample add_772 = (clamp_779 + ((int)1));
			t_sample div_775 = safediv(slide_781, selector_894);
			t_sample ceil_774 = ceil(div_775);
			t_sample rdiv_773 = safediv(slide_781, ceil_774);
			t_sample choice_25 = int(add_772);
			t_sample selector_771 = ((choice_25 >= 2) ? rdiv_773 : ((choice_25 >= 1) ? selector_894 : 0));
			__m_cycle_26.freq(selector_771);
			t_sample cycle_783 = __m_cycle_26(__sinedata);
			t_sample cycleindex_784 = __m_cycle_26.phase();
			t_sample orange_1613 = (selector_974 - selector_978);
			t_sample sub_1614 = (cycle_783 - (-1));
			t_sample scale_1611 = ((safepow((sub_1614 * ((t_sample)0.5)), ((int)1)) * orange_1613) + selector_978);
			t_sample mul_786 = (scale_1579 * scale_1611);
			t_sample mul_787 = (rdiv_788 * (scale_1579 + mul_786));
			if (eq_791) {
				t_sample len = mul_787;
				t_sample rate = safediv(((int)2), (len - ((int)1)));
				// for loop initializer;
				int i = ((int)0);
				// for loop condition;
				while ((i < len)) {
					// abort processing if an infinite loop is suspected;
					if (((__loopcount--) <= 0)) {
						__exception = GENLIB_ERR_LOOP_OVERFLOW;
						break ;
						
					};
					t_sample bipolarphase = ((i * rate) - ((int)1));
					t_sample samp = ((int)0);
					if ((mul_764 < ((int)1))) {
						samp = (samp + (make_sinc_d_d(bipolarphase, scale_1583) * (((int)1) - mul_764)));
						
					};
					t_sample samp_1458 = (samp + (make_rect_d_d(bipolarphase, scale_1583) * (((int)1) - fabs((((int)1) - mul_764)))));
					if ((mul_764 > ((int)1))) {
						samp_1458 = (samp_1458 + (make_saw_d_d(bipolarphase, scale_1583) * (mul_764 - ((int)1))));
						
					};
					int index_wrap_31 = (((counter_796 + i) < 0) ? ((ola_dim - 1) + (((counter_796 + i) + 1) % ola_dim)) : ((counter_796 + i) % ola_dim));
					m_ola_1.write((samp_1458 + (m_ola_1.read(index_wrap_31, 0) * ((int)1))), index_wrap_31, 0);
					// for loop increment;
					i = (i + ((int)1));
					
				};
				
			};
			// abort processing if an infinite loop is suspected;
			if (((__loopcount--) <= 0)) {
				__exception = GENLIB_ERR_LOOP_OVERFLOW;
				break ;
				
			};
			// assign results to output buffer;
			(*(__out1++)) = out1;
			(*(__out2++)) = out2;
			
		};
		return __exception;
		
	};
	inline void set_ola(void * _value) {
		m_ola_1.setbuffer(_value);
	};
	inline t_sample make_sinc_d_d(t_sample bipolarphase, t_sample bandwidth) {
		t_sample x = ((bandwidth * ((t_sample)3.1415926535898)) * bipolarphase);
		int cond_27 = (x == ((int)0));
		t_sample iffalse_28 = safediv(sin(x), x);
		return (cond_27 ? ((int)1) : iffalse_28);
		
	};
	inline t_sample make_rect_d_d(t_sample bipolarphase, t_sample bandwidth) {
		t_sample x = (bipolarphase * bandwidth);
		int cond_29 = (safemod(x, ((int)1)) < ((int)0));
		int iftrue_30 = (-((int)1));
		int rect = (cond_29 ? iftrue_30 : ((int)1));
		return (rect * ((t_sample)0.5));
		
	};
	inline t_sample make_saw_d_d(t_sample bipolarphase, t_sample bandwidth) {
		t_sample x = (bipolarphase * bandwidth);
		t_sample saw = safemod(x, ((int)1));
		return (saw * ((t_sample)0.5));
		
	};
	
} State;


///
///	Configuration for the genlib API
///

/// Number of signal inputs and outputs

int gen_kernel_numins = 8;
int gen_kernel_numouts = 2;

int num_inputs() { return gen_kernel_numins; }
int num_outputs() { return gen_kernel_numouts; }
int num_params() { return 1; }

/// Assistive lables for the signal inputs and outputs

const char *gen_kernel_innames[] = { "freq", "density_ratio", "mod_ratio", "grain_mod_depth", "waveform", "ratio_lock", "mod_frequency_couple", "sync" };
const char *gen_kernel_outnames[] = { "out1", "sync" };

/// Invoke the signal process of a State object

int perform(CommonState *cself, t_sample **ins, long numins, t_sample **outs, long numouts, long n) {
	State* self = (State *)cself;
	return self->perform(ins, outs, n);
}

/// Reset all parameters and stateful operators of a State object

void reset(CommonState *cself) {
	State* self = (State *)cself;
	self->reset(cself->sr, cself->vs);
}

/// Set a parameter of a State object

void setparameter(CommonState *cself, long index, t_param value, void *ref) {
	State *self = (State *)cself;
	switch (index) {
		case 0: self->set_ola(ref); break;
		
		default: break;
	}
}

/// Get the value of a parameter of a State object

void getparameter(CommonState *cself, long index, t_param *value) {
	State *self = (State *)cself;
	switch (index) {
		
		
		default: break;
	}
}

/// Get the name of a parameter of a State object

const char *getparametername(CommonState *cself, long index) {
	if (index >= 0 && index < cself->numparams) {
		return cself->params[index].name;
	}
	return 0;
}

/// Get the minimum value of a parameter of a State object

t_param getparametermin(CommonState *cself, long index) {
	if (index >= 0 && index < cself->numparams) {
		return cself->params[index].outputmin;
	}
	return 0;
}

/// Get the maximum value of a parameter of a State object

t_param getparametermax(CommonState *cself, long index) {
	if (index >= 0 && index < cself->numparams) {
		return cself->params[index].outputmax;
	}
	return 0;
}

/// Get parameter of a State object has a minimum and maximum value

char getparameterhasminmax(CommonState *cself, long index) {
	if (index >= 0 && index < cself->numparams) {
		return cself->params[index].hasminmax;
	}
	return 0;
}

/// Get the units of a parameter of a State object

const char *getparameterunits(CommonState *cself, long index) {
	if (index >= 0 && index < cself->numparams) {
		return cself->params[index].units;
	}
	return 0;
}

/// Get the size of the state of all parameters of a State object

size_t getstatesize(CommonState *cself) {
	return genlib_getstatesize(cself, &getparameter);
}

/// Get the state of all parameters of a State object

short getstate(CommonState *cself, char *state) {
	return genlib_getstate(cself, state, &getparameter);
}

/// set the state of all parameters of a State object

short setstate(CommonState *cself, const char *state) {
	return genlib_setstate(cself, state, &setparameter);
}

/// Allocate and configure a new State object and it's internal CommonState:

void *create(t_param sr, long vs) {
	State *self = new State;
	self->reset(sr, vs);
	ParamInfo *pi;
	self->__commonstate.inputnames = gen_kernel_innames;
	self->__commonstate.outputnames = gen_kernel_outnames;
	self->__commonstate.numins = gen_kernel_numins;
	self->__commonstate.numouts = gen_kernel_numouts;
	self->__commonstate.sr = sr;
	self->__commonstate.vs = vs;
	self->__commonstate.params = (ParamInfo *)genlib_sysmem_newptr(1 * sizeof(ParamInfo));
	self->__commonstate.numparams = 1;
	// initialize parameter 0 ("m_ola_1")
	pi = self->__commonstate.params + 0;
	pi->name = "ola";
	pi->paramtype = GENLIB_PARAMTYPE_SYM;
	pi->defaultvalue = 0.;
	pi->defaultref = 0;
	pi->hasinputminmax = false;
	pi->inputmin = 0;
	pi->inputmax = 1;
	pi->hasminmax = false;
	pi->outputmin = 0;
	pi->outputmax = 1;
	pi->exp = 0;
	pi->units = "";		// no units defined
	
	return self;
}

/// Release all resources and memory used by a State object:

void destroy(CommonState *cself) {
	State *self = (State *)cself;
	genlib_sysmem_freeptr(cself->params);
		
	delete self;
}


} // mom_jeans_gen::
