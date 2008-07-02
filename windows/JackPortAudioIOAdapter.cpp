/*
Copyright (C) 2008 Grame

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "JackPortAudioIOAdapter.h"
#include "portaudio.h"
#include "JackError.h"

namespace Jack
{

static inline float Range(float min, float max, float val)
{
    return (val < min) ? min : ((val > max) ? max : val);
}

/*
int JackPortAudioIOAdapter::Render(const void* inputBuffer, void* outputBuffer,
                                unsigned long framesPerBuffer,
                                const PaStreamCallbackTimeInfo* timeInfo,
                                PaStreamCallbackFlags statusFlags,
                                void* userData)
{
    JackPortAudioIOAdapter* adapter = static_cast<JackPortAudioIOAdapter*>(userData);
    float** paBuffer;
    char* buffer;
    
    if (!adapter->fRunning) {
        adapter->fRunning = true;
        adapter->fFirstCallbackTime = jack_get_time();
    }
    
    double src_ratio = double(jack_get_time() - adapter->fFirstCallbackTime) / double( adapter->fCallbackTime - adapter->fFirstCallbackTime);
    jack_log("Callback resampler coeff %f", src_ratio);
    
    paBuffer = (float**)inputBuffer;
    for (int i = 0; i < adapter->fCaptureChannels; i++) {
        
        buffer = (char*)paBuffer[i];
        size_t len = jack_ringbuffer_write_space(adapter->fCaptureRingBuffer[i]);
        
        if (len < framesPerBuffer * sizeof(float)) {
            jack_error("JackPortAudioIOAdapter::Process : producer too slow, missing frames = %d", framesPerBuffer - len / sizeof(float));
            jack_ringbuffer_write(adapter->fCaptureRingBuffer[i], buffer, len);
        } else {
            jack_ringbuffer_write(adapter->fCaptureRingBuffer[i], buffer, framesPerBuffer * sizeof(float));
        }
    }
    
    paBuffer = (float**)outputBuffer;
    for (int i = 0; i < adapter->fPlaybackChannels; i++) {
        
        buffer = (char*)paBuffer[i];
        size_t len = jack_ringbuffer_read_space(adapter->fPlaybackRingBuffer[i]);
         
        if (len < framesPerBuffer * sizeof(float)) {
            jack_error("JackPortAudioIOAdapter::Process : consumer too slow, skip frames = %d", framesPerBuffer - len / sizeof(float));
            jack_ringbuffer_read(adapter->fPlaybackRingBuffer[i], buffer, len);
        } else {
            jack_ringbuffer_read(adapter->fPlaybackRingBuffer[i], buffer, framesPerBuffer * sizeof(float));
        }
    }
    
    return paContinue;
}
*/

int JackPortAudioIOAdapter::Render(const void* inputBuffer, void* outputBuffer,
                                unsigned long framesPerBuffer,
                                const PaStreamCallbackTimeInfo* timeInfo,
                                PaStreamCallbackFlags statusFlags,
                                void* userData)
{
    JackPortAudioIOAdapter* adapter = static_cast<JackPortAudioIOAdapter*>(userData);
    float** paBuffer;
    float* buffer;
    SRC_DATA src_data;
    int res;
    jack_ringbuffer_data_t ring_buffer_data[2];
    
    adapter->fLastCallbackTime = adapter->fCurCallbackTime;
    adapter->fCurCallbackTime = jack_get_time();
    
    adapter->fConsumerFilter.AddValue(adapter->fCurCallbackTime - adapter->fLastCallbackTime);
    adapter->fProducerFilter.AddValue(adapter->fDeltaTime);
    
    jack_log("JackPortAudioIOAdapter::Render delta %ld", adapter->fCurCallbackTime - adapter->fLastCallbackTime);
     
    if (!adapter->fRunning) {
        adapter->fRunning = true;
        
        paBuffer = (float**)inputBuffer;
        for (int i = 0; i < adapter->fCaptureChannels; i++) {
            buffer = (float*)paBuffer[i];
            jack_ringbuffer_read(adapter->fCaptureRingBuffer[i], (char*)buffer, framesPerBuffer * sizeof(float));
            jack_ringbuffer_read(adapter->fCaptureRingBuffer[i], (char*)buffer, framesPerBuffer * sizeof(float));
            jack_ringbuffer_read(adapter->fCaptureRingBuffer[i], (char*)buffer, framesPerBuffer * sizeof(float));
        }
        
        paBuffer = (float**)outputBuffer;
        for (int i = 0; i < adapter->fPlaybackChannels; i++) {
            buffer = (float*)paBuffer[i];
            jack_ringbuffer_write(adapter->fPlaybackRingBuffer[i], (char*)buffer, framesPerBuffer * sizeof(float));
            jack_ringbuffer_write(adapter->fPlaybackRingBuffer[i], (char*)buffer, framesPerBuffer * sizeof(float));
            jack_ringbuffer_write(adapter->fPlaybackRingBuffer[i], (char*)buffer, framesPerBuffer * sizeof(float));
        }
    }
    
    /*
    double src_ratio_output = double(adapter->fCurCallbackTime - adapter->fLastCallbackTime) / double(adapter->fDeltaTime);
    double src_ratio_input = double(adapter->fDeltaTime) / double(adapter->fCurCallbackTime - adapter->fLastCallbackTime);
    */
    jack_time_t val1 = adapter->fConsumerFilter.GetVal();
    jack_time_t val2 = adapter->fProducerFilter.GetVal();
    double src_ratio_output = double(val1) / double(val2);
    double src_ratio_input = double(val2) / double(val1);
  
    if (src_ratio_input < 0.8f || src_ratio_input > 1.2f)
        jack_error("src_ratio_input = %f", src_ratio_input);
    
    if (src_ratio_output < 0.8f || src_ratio_output > 1.2f)
        jack_error("src_ratio_output = %f", src_ratio_output);
  
    src_ratio_input = Range(0.8f, 1.2f, src_ratio_input);
    src_ratio_output = Range(0.8f, 1.2f, src_ratio_output);
    
    jack_log("Callback resampler src_ratio_input = %f src_ratio_output = %f", src_ratio_input, src_ratio_output);
    
    paBuffer = (float**)inputBuffer;
    for (int i = 0; i < adapter->fCaptureChannels; i++) {
        
        buffer = (float*)paBuffer[i];
        jack_ringbuffer_get_write_vector(adapter->fCaptureRingBuffer[i], ring_buffer_data);
        
        unsigned int available_frames = (ring_buffer_data[0].len + ring_buffer_data[1].len) / sizeof(float);
        jack_log("INPUT available = %ld", available_frames);
        
        unsigned int frames_to_read = framesPerBuffer;
        unsigned int read_frames = 0;
        
        if (available_frames < framesPerBuffer) {
             jack_error("JackPortAudioIOAdapter::Render : consumer too slow, skip frames = %d", framesPerBuffer);
        } else {
            
            for (int j = 0; j < 2; j++) {
            
                if (ring_buffer_data[j].len > 0) {
                
                    src_data.data_in = &buffer[read_frames];
                    src_data.data_out = (float*)ring_buffer_data[j].buf;
                    src_data.input_frames = frames_to_read;
                    src_data.output_frames = (ring_buffer_data[j].len / sizeof(float));
                    src_data.end_of_input = 0;
                    src_data.src_ratio = src_ratio_input;
                 
                    res = src_process(adapter->fCaptureResampler[i], &src_data);
                    if (res != 0)
                        jack_error("JackPortAudioIOAdapter::Render err = %s", src_strerror(res));
                        
                    frames_to_read -= src_data.input_frames_used;
                    read_frames += src_data.input_frames_used;
                
                    jack_log("INPUT : j = %d input_frames_used = %ld output_frames_gen = %ld", j, src_data.input_frames_used, src_data.output_frames_gen);
                    jack_ringbuffer_write_advance(adapter->fCaptureRingBuffer[i], src_data.output_frames_gen * sizeof(float));
                }
            }
            
            if (read_frames < framesPerBuffer)
                jack_error("JackPortAudioIOAdapter::Render error read_frames = %ld", read_frames);
        }
    }
    
    paBuffer = (float**)outputBuffer;
    for (int i = 0; i < adapter->fPlaybackChannels; i++) {
        
        buffer = (float*)paBuffer[i];
        jack_ringbuffer_get_read_vector(adapter->fPlaybackRingBuffer[i], ring_buffer_data);
        
        unsigned int available_frames = (ring_buffer_data[0].len + ring_buffer_data[1].len) / sizeof(float);
        jack_log("OUTPUT available = %ld", available_frames);
        
        unsigned int frames_to_write = framesPerBuffer;
        unsigned int written_frames = 0;
        
        if (available_frames < framesPerBuffer) {
             jack_error("JackPortAudioIOAdapter::Render : producer too slow, skip frames = %d", framesPerBuffer);
        } else {
            
            for (int j = 0; j < 2; j++) {
            
                if (ring_buffer_data[j].len > 0) {
                    
                    src_data.data_in = (float*)ring_buffer_data[j].buf;
                    src_data.data_out = &buffer[written_frames];
                    src_data.input_frames = ring_buffer_data[j].len / sizeof(float);
                    src_data.output_frames = frames_to_write;
                    src_data.end_of_input = 0;
                    src_data.src_ratio = src_ratio_output;
                     
                    res = src_process(adapter->fPlaybackResampler[i], &src_data);
                    if (res != 0)
                        jack_error("JackPortAudioIOAdapter::Render err = %s", src_strerror(res));
                        
                    frames_to_write -= src_data.output_frames_gen;
                    written_frames += src_data.output_frames_gen;
                    
                    jack_log("OUTPUT : j = %d input_frames_used = %ld output_frames_gen = %ld", j, src_data.input_frames_used, src_data.output_frames_gen);
                    jack_ringbuffer_read_advance(adapter->fPlaybackRingBuffer[i], src_data.input_frames_used * sizeof(float));
                }
            }
            
            if (written_frames < framesPerBuffer)
                jack_error("JackPortAudioIOAdapter::Render error written_frames = %ld", written_frames);
        }
    }
    
    return paContinue;
}
        
int JackPortAudioIOAdapter::Open()
{
    PaError err;
    PaStreamParameters inputParameters;
    PaStreamParameters outputParameters;
    PaDeviceIndex inputDevice;
    PaDeviceIndex outputDevice;
    
    if (JackIOAdapterInterface::Open() < 0)
        return -1;
    
    err = Pa_Initialize();
    if (err != paNoError) {
        jack_error("JackPortAudioIOAdapter::Pa_Initialize error = %s\n", Pa_GetErrorText(err));
        goto error;
    }
    
    jack_log("JackPortAudioIOAdapter::Pa_GetDefaultInputDevice %ld", Pa_GetDefaultInputDevice());
    jack_log("JackPortAudioIOAdapter::Pa_GetDefaultOutputDevice %ld", Pa_GetDefaultOutputDevice());
    
    jack_log("JackPortAudioIOAdapter::Open fBufferSize = %ld fSampleRate %f", fBufferSize, fSampleRate);

    inputDevice = Pa_GetDefaultInputDevice();
    outputDevice = Pa_GetDefaultOutputDevice();
    
    inputParameters.device = inputDevice;
    inputParameters.channelCount = fCaptureChannels;
    inputParameters.sampleFormat = paFloat32 | paNonInterleaved;		// 32 bit floating point output
    inputParameters.suggestedLatency = (inputDevice != paNoDevice)		// TODO: check how to setup this on ASIO
                                       ? Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency
                                       : 0;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    outputParameters.device = outputDevice;
    outputParameters.channelCount = fPlaybackChannels;
    outputParameters.sampleFormat = paFloat32 | paNonInterleaved;		// 32 bit floating point output
    outputParameters.suggestedLatency = (outputDevice != paNoDevice)	// TODO: check how to setup this on ASIO
                                        ? Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency
                                        : 0;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(&fStream,
                        (inputDevice == paNoDevice) ? 0 : &inputParameters,
                        (outputDevice == paNoDevice) ? 0 : &outputParameters,
                        fSampleRate,
                        fBufferSize,
                        paNoFlag,  // Clipping is on...
                        Render,
                        this);
    if (err != paNoError) {
        jack_error("Pa_OpenStream error = %s", Pa_GetErrorText(err));
        goto error;
    }
    
    err = Pa_StartStream(fStream);
    if (err != paNoError) {
         jack_error("Pa_StartStream error = %s", Pa_GetErrorText(err));
         goto error;
    }
    jack_log("JackPortAudioIOAdapter::Open OK");
    return 0;
     
error:
    Pa_Terminate();
    return -1;
}

int JackPortAudioIOAdapter::Close()
{
    jack_log("JackPortAudioIOAdapter::Close");
    Pa_StopStream(fStream);
    jack_log("JackPortAudioIOAdapter:: Pa_StopStream");
    Pa_CloseStream(fStream);
    jack_log("JackPortAudioIOAdapter:: Pa_CloseStream");
    Pa_Terminate();
    jack_log("JackPortAudioIOAdapter:: Pa_Terminate");
    
    return JackIOAdapterInterface::Close();
}

void JackPortAudioIOAdapter::SetBufferSize(int buffer_size)
{
    JackIOAdapterInterface::SetBufferSize(buffer_size);
    Close();
    Open();
}

} // namespace