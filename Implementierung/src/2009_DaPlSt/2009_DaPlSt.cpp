#include "2009_DaPlSt/2009_DaPlSt.h"


_2009_DaPlSt::_2009_DaPlSt(float sample_rate) : onset_detection(0)
{
	this->sample_rate = sample_rate;
	this->odf_sample = 0;

	size_t hop_size = roundf(ODF_SAMPLE_INTERVAL * this->sample_rate);
	this->stft = new STFT(
		2 * hop_size,
		hop_size,
		0,
		HANN,
		COMPLEX
	);

	this->onset_detection = OnsetDetection(this->stft->numBins());
	this->tempo_induction = TempoInduction();
	this->beat_prediction = BeatPrediction();
}

_2009_DaPlSt::_2009_DaPlSt(const _2009_DaPlSt &that)
	: onset_detection(that.onset_detection)
{
	this->sample_rate = that.sample_rate;
	this->odf_sample = 0;

	unsigned hop_size = roundf(ODF_SAMPLE_INTERVAL * this->sample_rate);
	this->stft = new STFT(
		2 * hop_size,
		hop_size,
		0,
		HANN,
		COMPLEX
	);

	this->tempo_induction = that.tempo_induction;
	this->beat_prediction = that.beat_prediction;

}

_2009_DaPlSt &_2009_DaPlSt::operator=(const _2009_DaPlSt &that)
{
	if (this != &that)
	{
		this->sample_rate = that.sample_rate;
		this->odf_sample = that.odf_sample;

		delete this->stft;
		unsigned hop_size = roundf(ODF_SAMPLE_INTERVAL * this->sample_rate);
		this->stft = new STFT(
			2 * hop_size,
			hop_size,
			0,
			HANN,
			COMPLEX
		);

		this->onset_detection = that.onset_detection;
		this->tempo_induction = that.tempo_induction;
		this->beat_prediction = that.beat_prediction;
	}

	return *this;
}

_2009_DaPlSt::~_2009_DaPlSt()
{
	delete this->stft;
}

const BeatPrediction *_2009_DaPlSt::get_beat_prediction() const
{
	return &this->beat_prediction;
}

size_t _2009_DaPlSt::get_current_beat_time() const
{
	return this->beat_prediction.get_current_beat_time();
}

const STFT *_2009_DaPlSt::get_stft() const
{
	return this->stft;
}

const TempoInduction *_2009_DaPlSt::get_tempo_induction() const
{
	return &this->tempo_induction;
}

float _2009_DaPlSt::get_odf_sample() const
{
	return this->odf_sample;
}

size_t _2009_DaPlSt::get_time() const
{
	return this->beat_prediction.get_time();
}

float _2009_DaPlSt::get_tempo() const
{
	return this->tempo_induction.get_tempo();
}

int _2009_DaPlSt::operator()(float sample)
{
	int result = 0;

	if ((*this->stft)(sample))
	{
		Complex<float> stft_frame[this->stft->numBins()];

		for (size_t k = 0; k < this->stft->numBins(); ++k)
		{
			stft_frame[k] = this->stft->bin(k);
		}

		this->odf_sample = this->onset_detection(stft_frame);
		result |= 1;

		if (this->tempo_induction(this->odf_sample))
		{
			// new tempo estimate is available
			float tempo = this->tempo_induction.get_tempo();
			this->beat_prediction.set_tempo(tempo);

			result |= 4;
		}

		if (this->beat_prediction(this->odf_sample))
		{
			// new beat prediction is available
			result |= 2;
		}
	}

	return result;
}
