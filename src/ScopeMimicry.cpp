#include "ScopeMimicry.h"
#include <stdio.h>
#include <string.h>

ScopeMimicry::ScopeMimicry(uint16_t length, uint16_t nb_channel):
    _length(length),
    _nb_channel(nb_channel),
    _acq_counter(0),
    _effective_chan_number(0),
    _old_trigg_value(false),
    _trigged(false),
    _delay(0),
    _delay_complement(length),
    _trigged_counter(length),
    _final_idx(length-1)
{
    _memory = new float[_length*_nb_channel]();
    _names = new const char*[nb_channel];
    _channels = new float32_t *[nb_channel];
};

ScopeMimicry::~ScopeMimicry() {
    delete(_memory);
    delete(_names);
}

/**
 * @brief it get the reference of a static variable (channel) to be able to record its values when
 * it is necessary.
 *
 * @param channel : reference to a static variable
 * @param name: name of the variable you want to have after retrieving.
 */
void ScopeMimicry::connectChannel(float &channel, const char name[]) {
    if (_effective_chan_number < _nb_channel) {
        // capture the reference of the variable.
        _channels[_effective_chan_number] = &channel;
        _names[_effective_chan_number] = name;
        _effective_chan_number++;
    }
    // TODO: return errno code ?
}

/**
 * @brief add one value of each channel into its internal memory if the trigger has been
 * activated
 */
uint16_t ScopeMimicry::acquire() {
    bool trigg_value = (*_triggFunc)();
    // compute trigger
    if (!_old_trigg_value && trigg_value && !_trigged) {
        _trigged = true;
        _trigged_counter = (_acq_counter + _delay_complement - 1) % _length;
    }
    _old_trigg_value = trigg_value;

    if (!_trigged) {
        _trigged_counter = -1;
    }
    if (_acq_counter != _trigged_counter) {
        _acq_counter = (_acq_counter + 1)% _length;
        for (int k_ch=0; k_ch < _effective_chan_number; k_ch++) {
            _memory[(_acq_counter * _nb_channel) + k_ch] = *(_channels[k_ch]);
        }
        if (_trigged)
            return 1;
        else
            return 0;
    } else {
        _final_idx = _acq_counter;
        return 2;
    }
}

/**
 * @brief define a delay to record data before the trigg. delay is in [0, 1]
 *
 * @param d delay
 */
void ScopeMimicry::set_delay(float32_t d) {
    if (d < 0.0) _delay = 0.0;
    else if (d >= 1.0) _delay = _length;
    else _delay = d *_length;
    _delay_complement = _length - _delay;
}

/**
 * @brief reset the trigger and enable to record new data_dumped
 */
void ScopeMimicry::start() {
    _trigged = false;
    _old_trigg_value = false;
}

uint16_t ScopeMimicry::get_final_idx() {
    return _final_idx;
}

/**
 * @brief get a function without arguments which should return a boolean to enable
 * recording
 *
 * @param func: handle to a function which shall return a boolean.
 */
void ScopeMimicry::set_trigger(bool (*func)()) {
    _triggFunc = func;
}

uint8_t * ScopeMimicry::get_buffer() {
    return (uint8_t * )_memory;
}

uint16_t ScopeMimicry::get_buffer_size() {
    return (_length * _nb_channel * sizeof(float32_t));
}

uint16_t ScopeMimicry::get_length() {
    return _length;
}

uint16_t ScopeMimicry::get_nb_channel() {
    return _nb_channel;
}

bool ScopeMimicry::has_trigged() {
    return _trigged;
}

uint16_t ScopeMimicry::status() {
    return (uint16_t)((_trigged_counter*100) / _delay_complement);

}

const char *ScopeMimicry::get_channel_name(uint16_t idx) {
    if (idx < _nb_channel) {
        return this->_names[idx];
    }
    else {
        return "BadIdx";
    }
}

/**
 * @brief get one value of one channel
 *
 * @param index : value in the table between [0, length-1]
 * @param channel_idx :channel number.
 * @return float value.
 */
float32_t ScopeMimicry::get_channel_value(uint32_t index, uint32_t channel_idx) {
    /* to be sure we are not outside the buffer */
    index = (index) % _length;
    return _memory[(index * _nb_channel) + channel_idx];
}

void ScopeMimicry::reset_dump() {
	dump_state = initialized;
}

enum e_dump_state ScopeMimicry::get_dump_state() {
	return dump_state;
}

char* ScopeMimicry::dump_datas()
{
	uint16_t n_datas = _length * _nb_channel;
	/* reset char_name */
	char_name[0] = '\0';
	switch (dump_state) {
		case initialized:
			data_dumped = hash;
			_idx_name = 0;
			_idx_datas = 0;
			dump_state = names;
		break;
		case names:
			if (_idx_name < this->get_nb_channel())
			{
				strcat(char_name,get_channel_name(_idx_name));
				strcat(char_name, ",");
				data_dumped = char_name;
				_idx_name +=1;
			}
			else
			{
				data_dumped = (char *) "\n";
				dump_state = final_idx;
			}
		break;
		case final_idx:
			sprintf(char_name, "## %d\n", get_final_idx());
			data_dumped = char_name;
			dump_state = datas;
		break;
		case datas:
			if (_idx_datas < n_datas-1)
			{
				sprintf(char_data, "%08x\n", *((uint32_t *) _memory + _idx_datas));
				data_dumped = char_data;
				_idx_datas += 1;
			}
			else
			{
				dump_state = finished;
			}
		break;
		case finished:
			data_dumped = nullchar;
		break;
		}
	return data_dumped;
}
