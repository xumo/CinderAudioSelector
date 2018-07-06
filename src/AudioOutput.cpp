#include "AudioOutput.h"


AudioOutput::AudioOutput(audio::DeviceRef device)
{
	if (device) {
		mDevice = device;
		auto format_mono = audio::Node::Format().channels(1);
		auto ctx = audio::Context::master();
		if (device) {
			ci::app::console() << "selected " << device->getName() << std::endl;
			mInputDeviceNode = ctx->createInputDeviceNode(device);
		}
		
		if (mInputDeviceNode) {
			mInputDeviceNode->getChannelMode();
			mNumChannels = (int)mInputDeviceNode->getNumChannels();
			console() << "Channels " << mNumChannels << endl;
			auto monitorFormat = audio::MonitorSpectralNode::Format().fftSize(1024).windowSize(512);

			for (int i = 0; i < mNumChannels; i++) {
				auto spectral = ctx->makeNode( new audio::MonitorSpectralNode( monitorFormat ) );
				mSpectrals.push_back(spectral);
				mInputDeviceNode >> spectral;
			}

			mInputDeviceNode->enable();
			ctx->enable();
			mSpectrums.resize(mSpectrals.size());
		}
		
	}
}

AudioOutput::~AudioOutput()
{
}

void AudioOutput::update()
{
	//if(mInputDeviceNode)
		//spectrum	= mSpectral->getMagSpectrum();
	for (int i = 0; i < mSpectrals.size(); i++) {
		mSpectrums[i] = mSpectrals[i]->getMagSpectrum();
	}

	
}
