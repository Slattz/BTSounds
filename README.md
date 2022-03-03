# BTSounds

This homebrew project for the Nintendo Switch allows sound effects to be played when bluetooth audio is connected and disconnected.

## How It Works

It works by waiting for a bluetooth kernel event to be triggered, then compares how many devices are connected in order to tell if one has been (dis)connected. It then loads a short user-defined MP3 sound effect, decodes it, and then plays it using the Nintendo Switch's audio renderer.

## Installation

NOTE:  Requires a Nintendo Switch with [CFW](https://github.com/Atmosphere-NX/Atmosphere/)

* Download the latest release from [Releases](https://github.com/Slattz/BTSounds/releases/latest) and extract it onto your switch's SD card.

## Disclaimer

* BTSounds is not affiliated with, endorsed or approved by Nintendo in any way.
* BTSounds is distributed in the hope that it will be useful, but **WITHOUT ANY WARRANTY**; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. *See the GNU General Public License for more details.*
