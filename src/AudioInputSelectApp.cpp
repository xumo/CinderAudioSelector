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

#include <windows.h>
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
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
	void loadPing();
	void connectOutput();
	void ping();
	void selectDevice(string deviceStr);
	void changeOutput(string output);

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

static auto vector_getter = [](void* vec, int idx, const char** out_text)
{
	auto& vector = *static_cast<std::vector<std::string>*>(vec);
	if (idx < 0 || idx >= static_cast<int>(vector.size())) { return false; }
	*out_text = vector.at(idx).c_str();
	return true;
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

#if defined(CINDER_MSW)
	// at top of file, add #include <windows.h> (also within a CINDER_MSW block)
	auto nativeWindow = static_cast<HWND>(getWindow()->getNative());
	::SetForegroundWindow(nativeWindow);
	::SetFocus(nativeWindow);
#endif // CINDER_MSW
}

void AudioInputSelectApp::mouseDown( MouseEvent event )
{
}

void AudioInputSelectApp::update()
{
	
	ImGuiWindowFlags_ winFlags;

	{
		ui::ScopedWindow window("Audio Input", vec2(500, 263), 0.85);
		static int item2 = -1;
		ui::Text("Select an audio input");
		ImGui::ListBox("Devices", &item2, vector_getter, static_cast<void*>(&mDevices), mDevices.size());
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
	
	{
		//ui::ShowTestWindow();
	}

	
	for (AudioOutputRef &output : mOutputs) {
		
		output->update();
		ui::ScopedWindow window(output->getName(), vec2(440, 120));
		//ImGui::Columns(0);
		ImGui::Columns(1, "statscols", false);
		ImVec2 size = ImGui::GetItemRectSize();
		int i = 0;
		ImGui::PushItemWidth(380);
		for (auto &spectrum : output->spectrums()) {
			i++;
			ui::PlotHistogram( i == 1 ? "left":"right", spectrum.data(), spectrum.size(), 0, NULL, 0.0f, 0.001f, ImVec2(0, 40));
		}
		ImGui::PopItemWidth();
		
		//
		//ImGui::PlotLines("Lines", output->spectrum.data(), output->spectrum.size(), 0, "avg 0.0", 0.0f, 0.001f, ImVec2(0, 80));
	}
}



void AudioInputSelectApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 

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
	//connectOutput();
}

void AudioInputSelectApp::connectOutput()
{
	if (mSoundBufferPing->isConnectedToOutput(mGain)) {
		//mSoundBufferPing->disconnect(mGain);
	}
	//	mSoundBufferPing->disconnectAllOutputs();

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
	console() << "Find audio device by name: " << deviceStr << endl;
	ci::audio::DeviceRef device;
	for (auto audioDevice : ci::audio::Device::getDevices()) {
		//ci::app::console() << "device " << audioDevice->getName() << std::endl;
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

		console() << "Audio ouput add-->>>> " << device->getName() << endl;
		mOutputs.push_back(AudioOutputRef(new AudioOutput(device)));
	}
	
}

void AudioInputSelectApp::changeOutput(string output)
{
	auto ctx = ci::audio::master();
	ctx->disable();
	auto device = ci::audio::Device::findDeviceByName(output);
	if (device)
		ci::app::console() << "[xumo::AudioModule::selectOutput] device " << device->getName() << "\n";
	ci::audio::OutputDeviceNodeRef outputDevice = ctx->createOutputDeviceNode(device);
	
	ctx->setOutput(outputDevice);
	connectOutput();
	if (mGain) {
		//mGain->disconnectAllOutputs();
		//mGain >> output;
		
		//mGain >> ctx->getOutput();
		//ctx->enable();
	}
}

CINDER_APP( AudioInputSelectApp, RendererGl(RendererGl::Options().msaa(8)), [](App::Settings * settings) {
	settings->setBorderless(false);
	settings->setConsoleWindowEnabled(true);
	settings->setWindowSize(1024, 768);
	settings->setFrameRate(50);
	settings->setTitle("Audio input selector");
})
