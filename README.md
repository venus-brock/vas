## VAS - The superior* additive synthesizer.

<sub>(\*according to 9 out of 10 dentists)</sub>

VAS is a simple yet powerful additive synthesizer. Partials, envelope
generators and pitch modulators can be added or removed at will with no
arbitrary limits.

For each partial, the following parameters can be set:

- Gain
- Interval ratio (or constant frequency in Hz**)
- Which envelope generator to use
- Which pitch modulator to use

** not yet implemented

### Requirements

- jack2 / pipewire-jack
- Xlib

### Building and installing VAS

1 - Install the requirements listed above (if missing)

2 - Clone this repository

~~~
git clone https://github.com/venus-brock/vas.git
cd vas
~~~

3 - Build

If you wish to place the VAS executable in your system binaries directory
and copy the presets to your config folder, run `sudo make install`. Otherwise,
to simply build VAS, run `make`.

### TODO

- Improve UI
- Improve build process
- Handle pitch bend and sustain pedal
- Write full documentation
- Create some additional example presets
- Probably a lot more I can't think of right now
