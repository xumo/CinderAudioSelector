/****************************************
*
*	Simple tool to test audio input and outputs in Cinder
*	
*
*	Rodrigo Torres 
*	@xumerio
*	2018	
*
*	Feel free to use it as you like. 
*	This code is not ready for production in any way, so I'm not resposible for any further use of it.
*	
*	It uses this librearie, so check the licence details of them i you need to 
*	Cinder https://libcinder.org
*	Cinder-ImGui https://github.com/simongeilfus/Cinder-ImGui
*	dear imgui https://github.com/ocornut/imgui/
*
*	Sounds effect used is from SoundBible.com
*	
*	Used AudioDrawUtils from the audio cinder examples
*
*/

#if defined(CINDER_MSW)
#include <windows.h>
#endif
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/TextureFont.h"
#include "cinder/Utilities.h"
#include "cinder/audio/audio.h"


#include "utils/AudioDrawUtils.h"
#include "AudioOutput.h"
#include "CinderImGui.h"

using namespace ci;
using namespace ci::app;
using namespace std;



class AudioInputSelectApp : public App {
  public:
	void setup() override;
	
	void loadPing();
	void connectOutput();
	void ping();
	void selectDevice(string deviceStr);
	void changeOutput(string output);
	void updateUi();


	void update() override;
	void draw() override;

	audio::InputDeviceNodeRef		mInputDeviceNode;
	audio::MonitorSpectralNodeRef	mMonitorSpectralNode, mSpectral;
	vector<float>					mMagSpectrum, mSpectrum;

	SpectrumPlot					mSpectrumPlot;
	gl::TextureFontRef				mTextureFont;
	float							gain, volume;

	vector<string>					mDevices;
	std::vector<std::string>		mAudioOutputDevices;
	vector<AudioOutputRef>			mOutputs;

	ci::audio::GainNodeRef				mGain;
	ci::audio::BufferPlayerNodeRef		mSoundBufferPing;
	std::string						mSelectedOutput;
};

void AudioInputSelectApp::setup()
{
	ui::Options	options;
	options.darkTheme();
	ui::initialize(options);

	ci::audio::DeviceRef device;
	for (auto audioDevice : ci::audio::Device::getInputDevices()) {
		mDevices.push_back(audioDevice->getName());
	};

	for (auto audioDevice : ci::audio::Device::getOutputDevices()) {
		mAudioOutputDevices.push_back(audioDevice->getName());
	};

	mSelectedOutput = "Not selected";
	loadPing();

	//Bring focus to the opengl window
#if defined(CINDER_MSW)
	// at top of file, add #include <windows.h> (also within a CINDER_MSW block)
	auto nativeWindow = static_cast<HWND>(getWindow()->getNative());
	::SetForegroundWindow(nativeWindow);
	::SetFocus(nativeWindow);
#endif // CINDER_MSW
}


void AudioInputSelectApp::loadPing()
{
	
	auto ctx = ci::audio::Context::master();
	try {

		auto audioTrack = ci::audio::load(ci::loadFile(ci::app::getAssetPath("train.mp3")), ctx->getSampleRate());

		ci::audio::BufferRef buffer = audioTrack->loadBuffer();
		mSoundBufferPing = ctx->makeNode(new ci::audio::BufferPlayerNode(buffer));
		
	}
	catch (ci::Exception &exc) {
		ci::app::console() << "[xumo::AudioModule::loadPing] failed to load sound." << exc.what() << std::endl;
	}
	ci::app::console() << "[xumo::AudioModule::loadPing] laoaded" << std::endl;
}

void AudioInputSelectApp::connectOutput()
{
	auto ctx = ci::audio::Context::master();

	if (mGain) {
		if (mGain->isConnectedToOutput(ctx->getOutput())) {
			mGain->disconnect(ctx->getOutput());
		}
	}

	mGain = ctx->makeNode(new ci::audio::GainNode(1.0f));
	mSoundBufferPing >> mGain;
	mGain >> ctx->getOutput();
	ctx->enable();
	ci::app::console() << "[ AudioInputSelectApp::connectOutput]" << std::endl;
}

void AudioInputSelectApp::ping()
{
	if (mSoundBufferPing)
		mSoundBufferPing->start();
}

void AudioInputSelectApp::selectDevice(string deviceStr)
{
	console() << "[AudioInputSelectApp::selectDevice] Find audio device by name: " << deviceStr << endl;
	ci::audio::DeviceRef device;
	for (auto audioDevice : ci::audio::Device::getDevices()) {
		std::size_t found = audioDevice->getName().find(deviceStr);
		if (found != std::string::npos) {
			device = audioDevice;
		}
	};

	bool repeated = false;
	for (AudioOutputRef output: mOutputs) {
		if (output->getName() == device->getName()) {
			repeated = true;
		}
	}

	if (device && !repeated) {

		console() << "[AudioInputSelectApp::selectDevice] Audio ouput add-->>>> " << device->getName() << endl;
		mOutputs.push_back(AudioOutputRef(new AudioOutput(device)));
	}
	
}

void AudioInputSelectApp::changeOutput(string output)
{
	auto ctx = ci::audio::master();
	ctx->disable();
	auto device = ci::audio::Device::findDeviceByName(output);
	if (device)
		ci::app::console() << "[AudioInputSelectApp::changeOutput] device " << device->getName() << "\n";
	ci::audio::OutputDeviceNodeRef outputDevice = ctx->createOutputDeviceNode(device);
	
	ctx->setOutput(outputDevice);
	connectOutput();
	
}

void AudioInputSelectApp::updateUi()
{
	
	{
		ui::ScopedWindow window("Audio Input", vec2(500, 263), 0.85);
		static int item2 = -1;
		ui::Text("Select an audio input");
		ui::Combo("Audio Input Devices", &item2, mDevices, mDevices.size());

		bool select = ImGui::Button("Select");
		if (select) {
			if (item2 >= 0 && item2 < mDevices.size()) {
				selectDevice(mDevices.at(item2));
			}
		}

		ImGui::Dummy(ci::vec2(20, 20));
		static int itemOut = -1;
		ui::Text("Select an audio output");
		ui::Combo("Audio Output Devices", &itemOut, mAudioOutputDevices, mAudioOutputDevices.size());

		bool selectOut = ImGui::Button("Select Audio Output");
		if (selectOut) {
			if (itemOut >= 0 && itemOut < mAudioOutputDevices.size()) {

				mSelectedOutput = mAudioOutputDevices[itemOut];
				changeOutput(mSelectedOutput);
			}
		}
		ui::Text("Selected output");
		ui::Text(mSelectedOutput.c_str());

		if (ui::Button("Ping")) {
			ping();
		}
	}


	for (AudioOutputRef &output : mOutputs) {

		output->update();
		ui::ScopedWindow window(output->getName(), vec2(440, 120));
		ImGui::Columns(1, "statscols", false);
		int i = 0;
		ImGui::PushItemWidth(380);
		for (auto &spectrum : output->spectrums()) {
			i++;
			ui::PlotHistogram(i == 1 ? "left" : "right", spectrum.data(), spectrum.size(), 0, NULL, 0.0f, 0.001f, ImVec2(0, 40));
		}
		ImGui::PopItemWidth();

	}
}

void AudioInputSelectApp::update()
{
	updateUi();
}



void AudioInputSelectApp::draw()
{
	gl::clear(Color(0, 0, 0));
}

CINDER_APP( AudioInputSelectApp, RendererGl, [](App::Settings * settings) {
	settings->setBorderless(false);
	//settings->setConsoleWindowEnabled(true);
	settings->setWindowSize(600, 768);
	settings->setTitle("Audio selector");
})
