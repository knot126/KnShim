<img src="logo/FoxthingText.png" style="width: 100%"/>

# KnShim

> **KnShim has a successor, [YipLoader](https://github.com/knot126/YipLoader), that will be further developed in the future.**

KnShim is a mod loader for Android games. It loads before the game's native library and uses a custom ELF loader and hooking library, Leaf, to provide the needed tools to modify games at runtime on even the newest versions of Android (as of Android 16).

It also provides some "out of the box" goodies for specific games and libraries, like utility functions in Lua scripts.

## Supported Games

Currently, only games by Mediocre AB are supported, and many built-in features assume they are running with one of their games loaded. It should be possible to add support for other games (especially as I've been doing work to make the shim more generic), though I don't yet have a personal interest in any other games.

| Game | Developer |
| ---- | --------- |
| *Granny Smith* (2012) | Mediocre AB |
| *Smash Hit* (2014) | Mediocre AB |

## Showcase

KnShim has been in several *Smash Hit* mods to provide features not possible in the base game. Here are some of my favourite:

* **Shatter Client** is the primary development target for KnShim. It uses KnShim's utlities to download levels from a server and test them.
* **[Smash Hit Flatbread](https://sites.google.com/view/smashhitlab/mods/shl-mods/smash-hit-flatbread)** uses the shim to create an extremely high effort shitpost that you can't not enjoy.
* **[Mod Blueprints](https://sites.google.com/view/smashhitlab/documentation/mod-blueprints)** include KnShim by default too!

## Docs

The documentation is kept on [the new Smash Hit Wiki](https://smashhit.miraheze.org/wiki/KnShim/Documentation). Feel free to contribute if you have an account. :3

## Building

Build using:

```
ndk-build
```

from the Android NDK.

Any version of the NDK not horrendously oudated should be fine. I usually build using r18.
