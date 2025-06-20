# Super Mario Odyssey - ManHunt Multiplayer Gamemode

Welcome to the official repository for the Super Mario Odyssey Online mod! Have fun exploring kingdoms with friends, playing gamemodes, or beating the game as fast as possible! This mod is still early in development, so expect bugs and un-refined aspects as we work hard to improve it and make the mod as polished as possible.

## Features

* Explore Kingdoms together with up to 16 People
* Almost every capture in the game is synced between players
* Full 2D and Costume models syncing
* Moon Collection is shared between all players
* Custom Configuration Menu (Accessible by holding ZL and selecting any option in the pause/start menu)
	

## Installation and Usage
For the typical installation along with how to setup and use Multiplayer / ManHunt, please visit the [Super Mario Odyssey Online website](https://smoo.it).

<details>

<summary>Developer build instructions</summary>

  ### Building Prerequisites

  - [devkitPro](https://devkitpro.org/) 
  - Python 3
  - The [Keystone-Engine](https://www.keystone-engine.org/) Python Module

  ### Building

  Build has only been tested on WSL2 running Ubuntu 20.04.1.

  Just run:
  ```
  DEVKITPRO={path_to_devkitpro} make
  ```

  On Ubuntu (and other Debian-based systems), devkitPro will be installed to `/opt/devkitpro` by default:

  ```
  DEVKITPRO=/opt/devkitpro/ make
  ```

  ### Installing (Atmosphère)

  After a successful build, simply transfer the `atmosphere` folder located inside `starlight_patch_100` to the root of your switch's SD card.
</details>

## Troubleshooting

The [Super Mario Odyssey Online website](https://smoo.it) has a FAQ section that should solve many issues.
However, for any further questions or help not covered by the site, please visit the [CraftyBoss Community Discord Server](discord.gg/jYCueK2BqD) and ask in the `help`/`help-2` channel. 

---

# Contributors

- [CraftyBoss](https://github.com/craftyboss) Wrote the majority of the multiplayer code
- [Secret Dev](https://github.com/craftyboss) Wrote the majority of the new manhunt code
- [Hesmakka/LynxDev2](https://github.com/lynxdev2) Wrote the majority of compass code aswell as some other code (Original Author of the code of the 1st ManHunt he made)
- [Sanae](https://github.com/sanae6) Wrote the majority of the server code
- [Shadow](https://github.com/shadowninja108) original author of starlight, the tool used to make this entire mod possible
- [GRAnimated](https://github.com/GRAnimated)
- [Eriizer](https://www.youtube.com/channel/UCYfcjnuVTrf7pJqz2Jkm3SA) ManHunt In-Game Logo

# Credits
- [OdysseyDecomp](https://github.com/shibbo/OdysseyDecomp)
- [OdysseyReversed](https://github.com/shibbo/OdysseyReversed)
- [open-ead](https://github.com/open-ead/sead) sead Headers
