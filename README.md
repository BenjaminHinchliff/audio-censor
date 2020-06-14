# Audio Censor

A program to censor curse words in audio files

# Usage

`audiocensor.exe [insert path to .wav or .aif file here]`

it will output to a file called `out.wav` which will contain the censored audio.

# Compilation

Open the Visual Studio project and compile either of the Win32 platform binaries. It wouldn't be too difficult to compile it for Linux or Mac but I haven't written a Makefile so you'll have to do it oldschool. Or you could write your own Makefile - if you do please add a pull request so I can add it to the repo, thanks!

# How Does It Work?

It uses the pocketsphinx library to try to convert the speech in the audio file to text, then loops though all of the potential transcriptions, and for each word of the transcript it stems the word and checks if it exsits in the `bad-words.txt` wordlist. If it is in the wordlist, it finds where in the file that word starts and ends and fills it with zeros, unobtrusively removing the curse word.

# How Well Does It Work?

So-so. It does get a lot of the curse words, but it seems to bias towards male voices and will often miss more creative ones because they're harder of pocketsphinx to parse and might not be in the wordlist. It does do pretty well overall though, so I'm pretty happy. 
