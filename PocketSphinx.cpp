#include <pocketsphinx.h>
#include <olestem/stemming/english_stem.h>
#include <AudioFile.h>

#include <iostream>
#include <memory>
#include <algorithm>
#include <regex>
#include <string>
#include <limits>
#include <fstream>
#include <iterator>
#include <unordered_set>

#include "Deleters.h"

#define MODELDIR "./deps/pocketsphinx/model"

constexpr uint32_t SAMPLE_RATE = 16000U;
constexpr uint32_t FRAME_RATE = 100U;
constexpr uint32_t SAMPLES_PER_FRAME = SAMPLE_RATE / FRAME_RATE;

constexpr const char *WORDLIST = "bad-words.txt";

template <typename T, typename U = int16_t>
std::vector<U> convertAudioFileToNumericVector(const AudioFile<T> &in)
{
    std::vector<U> buf;
    buf.resize(in.getNumSamplesPerChannel());
    std::transform(
        in.samples[0].begin(),
        in.samples[0].end(),
        buf.begin(),
        [](double ele)
        {
            return static_cast<short>(round(ele * std::numeric_limits<U>::max()));
        }
    );

    return buf;
}

std::unordered_set<std::wstring> loadWordset(const std::string &path)
{
    std::unordered_set<std::wstring> wordset;
    std::wifstream file(path);
    std::wstring line;
    while (!file.eof())
    {
        std::getline(file, line);
        wordset.insert(line);
    }
    return wordset;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::wcerr << L"You must specify an input audio file\n";
        return 1;
    }

    std::string inputFile = argv[1];

    auto wordset = loadWordset(WORDLIST);

    std::unique_ptr<cmd_ln_t, Deleters::CmdLn> config(cmd_ln_init(nullptr, ps_args(), true,
        "-hmm", MODELDIR "/en-us/en-us",
        "-lm", MODELDIR "/en-us/en-us.lm.bin",
        "-dict", MODELDIR "/en-us/cmudict-en-us.dict",
        nullptr));
    if (!config)
    {
        std::wcerr << L"Failed to create config object, see log for details" << '\n';
        return 1;
    }

    std::unique_ptr<ps_decoder_t, Deleters::PsDecoder> ps(ps_init(config.get()));
    if (!ps)
    {
        std::wcerr << L"Failed to create recognizer, see log for details" << '\n';
        return 1;
    }

    AudioFile<double> af;
    if (!af.load(inputFile))
    {
        std::wcout << L"unable to load wav file" << '\n';
        return 1;
    }
    if (!af.isMono() || af.getSampleRate() != SAMPLE_RATE)
    {
        std::wcout << L"audio file must be mono with a sample rate of " << SAMPLE_RATE << L" per second" << '\n';
        return 1;
    }

    std::vector<int16_t> buf = convertAudioFileToNumericVector(af);

    if (ps_start_utt(ps.get()) < 0)
    {
        std::wcerr << L"Failed to signal the start of utterance" << '\n';
    }

    ps_process_raw(ps.get(), buf.data(), buf.size(), false, true);

    if (ps_end_utt(ps.get()) < 0)
    {
        std::wcerr << L"Failed to signal end of utterance" << '\n';
        return 1;
    }

    stemming::english_stem<> stemEnglish;
    // matches repition markings and action things
    std::regex frillsMatcher("[[(<].*[)>\\]]");

    size_t hypothesesCount = 0;
    size_t curseCount = 0;
    ps_nbest_t *nBestIter = ps_nbest(ps.get());
    while (nBestIter)
    {
        ps_seg_t *segIter = ps_nbest_seg(nBestIter);
        while (segIter)
        {
            std::string word(ps_seg_word(segIter));
            // remove frills
            word = std::regex_replace(word, frillsMatcher, "");
            // convert to wstring for stemming
            std::vector<wchar_t> wideBuffer(word.length() + 1);
            std::wmemset(wideBuffer.data(), 0, word.length() + 1);
            std::mbstowcs(wideBuffer.data(), word.c_str(), word.length());
            std::wstring wideWord(wideBuffer.data());
            stemEnglish(wideWord);
            if (wordset.find(wideWord) != wordset.end())
            {
                // get start and end of frame
                int startFrame = -1;
                int endFrame = -1;
                ps_seg_frames(segIter, &startFrame, &endFrame);
                size_t startSample = startFrame * SAMPLES_PER_FRAME;
                size_t endSample = endFrame * SAMPLES_PER_FRAME;
                auto start = af.samples[0].begin();
                std::fill(start + startSample, start + endSample, 0.0);
                ++curseCount;
            }
            segIter = ps_seg_next(segIter);
        }

        ++hypothesesCount;
        nBestIter = ps_nbest_next(nBestIter);
    }
    // freed automatically by exausting the iterator

    std::wcout << L"Potential transcription: " << ps_get_hyp(ps.get(), nullptr) << L'\n';
    std::wcout << L"searched " << hypothesesCount << L" transcription hypotheses and removed " << curseCount << L" potential curse words instances\n";

    af.save("out.wav");

    return 0;
}