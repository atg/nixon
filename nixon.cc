#include <boost/circular_buffer.hpp>
#include <SFML/System.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <sstream>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "configuration_options.h"

// So if we're at 44100 sample/s, then let's have two frames per second
static const size_t NixonFrameSize = 22050;

typedef int16_t sample_t;
static const sample_t SilenceThresholdValue = 1024;
static const int SilenceThresholdCount = NixonFrameSize / 4;

static const int NumSilentFramesBeforeStopping = 20; // 10 frames or 10 seconds

static const int NixonChannelCount = 1;
static const int NixonSampleRate = 44100;

static void copyFrame(sample_t* src, sample_t* dest) {
    memcpy(dest, src, NixonFrameSize);
}

template<typename T>
static std::string toString(T x) {
    std::stringstream ss;
    ss << x;
    return ss.str();
}

//#define DISABLE_LOGGING
#ifndef DISABLE_LOGGING
#define debug_printf(...) printf(__VA_ARGS__)
#endif

static sample_t previousFrame[NixonFrameSize];
static sample_t scratch[NixonFrameSize];

static sf::Mutex recording_queue_mutex;
static std::vector<std::vector<sample_t>> recording_queue;

struct NixonSoundRecorder : public sf::SoundRecorder {
    
    boost::circular_buffer<sample_t> buff;
    
    bool areRecording;
    std::vector<sample_t> recording;
    int silentFrames;
        
    NixonSoundRecorder() : buff(1024*1024), areRecording(false) { }
    
    
// Scanning
    virtual bool OnProcessSamples(const sf::Int16* samples, std::size_t samplesCount) {
        
        // Add to the buffer
        for (int i = 0; i < samplesCount; i++) {
            buff.push_back(samples[i]);
        }
        
        // Process frames until we can't
        while (popFrame());
        
        return true;
    }
    
    bool popFrame() {
        // Take NixonFrameSize frames
        size_t s = buff.size();
        if (s < NixonFrameSize)
            return false;
        
        // Copy the first NixonFrameSize elements into scratch
        for (int i = 0; i < NixonFrameSize; i++) {
            scratch[i] = buff[i];
        }
        
        // Remove the elements from the front
        buff.rresize(s - NixonFrameSize);
        
        processFrame();
        copyFrame(scratch, previousFrame);
        
        return true;
    }
    
    void processFrame() {
        int count = 0;
        for (int i = 0; i < NixonFrameSize; i++) {
            // debug_printf("..scratch[%d] = %d\n", i, (int)scratch[i]);
            if (scratch[i] >= SilenceThresholdValue || scratch[i] <= -SilenceThresholdValue)
                count++;
        }
        
        // Silent?
        bool isSilent = count < SilenceThresholdCount;
        debug_printf("Processing frame. [count %d] [SilenceThresholdValue %d] [SilenceThresholdCount %d] [is silent %d]\n", count, SilenceThresholdValue, SilenceThresholdCount, isSilent);
        
        if (!areRecording && !isSilent) {
            startRecording();
            recordFrame(previousFrame);
        }
        
        // Recording?
        if (areRecording) {
            recordFrame(scratch);
            
            if (isSilent) {
                silentFrames++;
                if (silentFrames >= NumSilentFramesBeforeStopping)
                    stopRecording();
            }
        }
    }

// Recording
    void startRecording() {
        debug_printf("Start recording\n");
        
        areRecording = true;
        silentFrames = 0;
    }
    void stopRecording() {
        debug_printf("Stop recording\n");

        areRecording = false;
        
        // In another thread
        sf::Lock lock(recording_queue_mutex);
        recording_queue.push_back(recording);
    }
    void recordFrame(sample_t* frame) {
        recording.insert(recording.end(), frame, frame + NixonFrameSize);
    }
    
    
// Writing
    static void writeRecordingsConcurrently(void* unused) {
        std::vector<std::vector<sample_t>> temp_recording_queue;
        {
            sf::Lock lock(recording_queue_mutex);
            temp_recording_queue = recording_queue;
            recording_queue.clear();
        }
        
        for (auto& rec : temp_recording_queue) {
            writeRecordingConcurrently(&rec);
        }
    }
    static void writeRecordingConcurrently(std::vector<sample_t>* rec) {
        // Do we want to save this recording?
        bool shouldWrite = false;
        
        int count = 0;
        for (int i = 0, n = rec->size(); i != n; i++) {
            count++;
        }
        
        double rsize = rec->size();
        rsize -= NixonFrameSize * NumSilentFramesBeforeStopping;
        if (rsize > 0) {
            double proportion = double(count) / rsize;
            if (proportion > 0.25) {
                shouldWrite = true;
            }
        }
        
        if (shouldWrite)
            writeRecordingToFile(rec);
    }
    static void writeRecordingToFile(const std::vector<sample_t>* rec) {
        time_t t;
        time(&t);
        
        struct tm *timeinfo;
        char buff[500];
        
        timeinfo = localtime(&t);
        strftime(buff, 500, "~/nixon/saved/%Y-%m-%d-at-%H-%M-%S", timeinfo);
        
        std::string filepath = buff;
        
        sf::SoundBuffer soundbuffer;
        soundbuffer.LoadFromSamples(&(*rec)[0], rec->size(), NixonChannelCount, NixonSampleRate);
        
        filepath = filepath + "-" + toString(rand()) + SOUND_FORMAT;
        printf("Writing to file: %s\n", filepath.c_str());
        soundbuffer.SaveToFile(filepath);
        
        // 2.0 // soundbuffer.loadFromSamples(&(*rec)[0], rec->size(), NixonChannelCount, NixonSampleRate);
        // 2.0 // soundbuffer.saveToFile(filepath + "-" + toString(rand()) + ".flac");
    }
    
    virtual bool OnStart() { return true; }
    virtual void OnStop() { /* ignore */ }
};

int main(int argc, char const *argv[]) {
    
    if (!sf::SoundRecorder::CanCapture()) {
    // 2.0 // if (!sf::SoundRecorder::isAvailable()) {
        printf("Unable to capture audio\n");
        return 1;
    }
    
    printf("Recording device available. I'm listening.\n");
    
    sf::Thread thread(NixonSoundRecorder::writeRecordingsConcurrently);
    thread.Launch();
    // 2.0 // thread.launch();
    
    NixonSoundRecorder rec;
    rec.Start();
    // 2.0 // rec.start();
    
    // Wait a while...
    while (1) {
        sf::Sleep(2);
        // 2.0 // sf::sleep(2);
    }
    
    return 0;
}