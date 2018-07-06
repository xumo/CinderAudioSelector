#pragma once

#include "cinder/app/App.h"
#include "cinder/gl/gl.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/TextureFont.h"
#include "cinder/Utilities.h"
#include "cinder/audio/audio.h"

#include "utils/AudioDrawUtils.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class AudioOutput;

typedef std::shared_ptr<AudioOutput> AudioOutputRef;


class AudioOutput 
{
public:
	AudioOutput(audio::DeviceRef device);
	~AudioOutput();
	void update();

	string getName() { return mDevice->getName(); };
	//vector<float>					spectrum;
	int	numChannles() { return mNumChannels; };

	vector<vector<float>> spectrums() { return mSpectrums; };

private:
	ci::audio::DeviceRef			mDevice;
	audio::InputDeviceNodeRef		mInputDeviceNode;
	vector<audio::MonitorSpectralNodeRef>	 mSpectrals;
	vector<vector<float>>			mSpectrums;


	float							mNumChannels;

};



