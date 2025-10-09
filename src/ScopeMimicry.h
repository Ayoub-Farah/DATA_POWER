/**
 * @file
 * @brief ScopeMimicry: to mimicry a digital scope inside the cpu with a ram array. 
 */

#ifndef _SCOPEMIMICRY_H_
#define _SCOPEMIMICRY_H_

#include <cstdint>
typedef float float32_t;

enum e_dump_state {
	initialized,
	names,
	final_idx,
	datas,
	finished
};

class ScopeMimicry {
public:
	/**
	 * @brief create a scope 
	 *
	 * @param length : the size of each variable to record 
	 * @param nb_channel : number of variables to record
	 */
	ScopeMimicry(uint16_t length, uint16_t nb_channel);
	~ScopeMimicry();
	/**
	 * @brief define a variable to record
	 * Care!! the data must be a static float variable.
	 *
	 * @param channel : static float variable
	 * @param name : to define a name for the variable ex: "myvar"
	 */
	void connectChannel(float &channel, const char name[]);
	/**
   * @brief add one data in memories for all channels if trigger has been true
   * one time.
   *
   * @return an integer percentage of acquisition. then if you have 100 the
   * acquisition is finished.
   */
	uint16_t acquire(); // or trace ?
	// void dump();
	/**
   * @brief reset acquisition trigger
   */
	void start();

	/**
   * @brief define the trigger
   *
   * @param func must a function without arguments but which return a boolean
   * value when its true the data are recorded in memory by the way of the
   * `acquire()` method.
   */
	void set_trigger(bool (*func)());
	/**
   * @brief define a delay to have some datas before the trigger instant between
   * the delay is a relative value between [0, 1]
   * delay of 0 : mean no delay after trigger
   *
   * @param delay (float value)
   */
	void set_delay(float32_t delay);
	/**
   * @brief as there's a circular buffer, the last point recorded can be
   * anywhere in this buffer. this function give to us the index of this last
   * value. the index is between 0 and lenght-1
   *
   * @return
   */
	uint16_t get_final_idx();

	/**
   * @brief get pointer to the memory of data recorded
   *
   * @return
   */
	uint8_t *get_buffer();

	/**
   * @brief return number of byte used by the memory
   *
   * @return
   */
	uint16_t get_buffer_size();
	/**
   * @brief return the length of one channel recording
   *
   * @return
   */
	uint16_t get_length();
	/**
   * @brief number of channel recorded
   *
   * @return
   */
	uint16_t get_nb_channel();
	/**
   * @brief return the status of the trigger
   *
   * @return
   */
	bool has_trigged();

	/**
   * @brief return percentage of filling memory
   *
   * @return percentage of filling memory in % (integer)
   */
	uint16_t status();
	/**
	 * @brief return name of the channel "idx"
	 *
	 * @param idx : number of the channel (begin with 0)
	 * @return a c-string (const char*)
	 */
	const char *get_channel_name(uint16_t idx);
	/**
	 * @brief to retrieve one date saved
	 *
	 * @param index : number ot the data to get [0, length-1]
	 * @param channel_idx : number of the channel [0, nb_channel-1]
	 * @return a float number
	 */
	float32_t get_channel_value(uint32_t index, uint32_t channel_idx);
	/**
	 * @brief help to get all the data recorded in a csv format.
	 * you must call scope.reset_dump() before, then
	 * it must be called in a while loop until the scope.dump_state == finished
	 *
	 * @return string.
	 */
	char *dump_datas();
	/**
	 * @brief to call one time before using dump_datas()
	 */
	void reset_dump();
	/**
	 * @brief dump_state can be: initialized, names, final_idx, datas, and finished.
	 *
	 * @return current dump_state value
	 */
	e_dump_state get_dump_state();

private:
	uint16_t _length;
	uint16_t _nb_channel;
	uint16_t _acq_counter;
	uint16_t _effective_chan_number;
	bool _old_trigg_value;
	bool _trigged;
	uint16_t _delay;
	uint16_t _idx_name;
	uint16_t _idx_datas;


	float32_t **_channels;
	float32_t *_channel_numbers;
	const char **_names;
	float32_t *_memory;

	uint16_t _decimation;
	uint16_t _delay_complement;
	uint16_t _trigged_counter;
	uint16_t _final_idx;
	bool (*_triggFunc)();
	/* variable for dump_datas method */
	e_dump_state dump_state;
	char hash[2] = "#";
	char nullchar[2] = " ";
	char char_name[256];
	/* each data is a float written in hexa so eight chars with \n and \0 chars at end */
	char char_data[10];
	char *data_dumped;


};

#endif
