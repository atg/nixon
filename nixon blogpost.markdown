# Big Brother Raspberry Pi

My favourite thing about being a programmer is not the favourable working conditions or money, but the *superpowers*.

Aside from being a programmer, I'm also a bassist. Often when practicing I'll play something cool, but have no idea what it was. But aside from recording *every single practice session*, that's a pie in the sky...

Except, as a programmer I have *superpowers*. All I have to do is buy a raspberry pi and spend christmas day hacking.

## Introducing Nixon

**Nixon** is a sound-activated recording device.

## How to get your very own Nixon

1. Buy a [Raspberry Pi](http://www.raspberrypi.org/). All in all you'll need a pi, case, 4GB+ SD card, ethernet cable, USB audio adapter, and "tablet charger" (USB power adapter with output of at least 1 amp).
2. Download and [install Raspian](http://txfx.net/2012/12/05/raspberry-pi-headless-setup/)
3. Install `git`, `libsfml-dev` and `libboost-dev` packages.
4. `git clone https://github.com/fileability/nixon.git` in your home directory (I hardcoded the path to save time).
5. `cd nixon && bash build.sh`
6. `./nixon`
7. Play!