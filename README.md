# Velocity Randomiser (VST 3)

Randomises the velocity of every incoming MIDI note-on.

Parameters (all whole numbers, 0-127, default 5):
- **Max Deviation @ Vel 127**: largest +/- change for a note at velocity 127.
- **Max Deviation @ Vel 1**: largest +/- change for a note at velocity 1.
  The allowed deviation for other velocities is interpolated in a straight line between the two.
  Equal values give the same deviation at every velocity.
- **Bypass**: events pass through unchanged.

Output velocity is always a whole number from 1 to 127 for any incoming velocity of 1 or more.

MIDI controllers (sustain pedal, mod wheel, pitch bend, aftertouch) are passed through. VST 3 hosts only
deliver these to a plug-in through MIDI-mapped parameters, so the plug-in exposes hidden parameters for
all 16 channels and re-emits each change as a MIDI controller output event.

## Build (Windows, 64-bit)
1. Install Visual Studio 2022 (C++ desktop workload) and CMake; get the VST 3 SDK (with submodules).
2. In this folder:
       mkdir build
       cd build
       cmake -G "Visual Studio 17 2022" -A x64 -Dvst3sdk_SOURCE_DIR=C:/path/to/vst3sdk ..
       cmake --build . --config Release
3. Run the SDK validator on the resulting .vst3, then load it in your host.

## Before distributing
- Edit `source/version.h` (name, company, web, email).
- Keep the IDs in `source/vr_cids.h` fixed once released.
- Optional: add `resource/win32resource.rc` (see CMakeLists.txt) and code-sign the plug-in.

## Test the maths without the SDK
    cd tests
    g++ -std=c++17 -I../source test_velocity.cpp -o test_velocity && ./test_velocity
    g++ -std=c++17 -I../source test_midi.cpp -o test_midi && ./test_midi
