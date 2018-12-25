# About

[Ultracopier](https://ultracopier.first-world.info/) is free and open
source software licensed under GPLv3 that acts as a replacement for
files copy dialogs.

Main features include:
- task queue
- pause / resume
- resume unfinished jobs
- dynamic speed limitation
- collision management
- plugin support


# Building

Building an all-in-one version is as easy as compiling the main Qt project:

    qmake ultracopier.pro


# Translations

Translations are provided via [Qt Linguist](http://doc.qt.io/qt-5/qtlinguist-index.html).

1. Run `lupdate ultracopier.pro` to update the translation files
2. Translate within the `.ts` files
3. Run `lrelease ultracopier.pro` to compile the files
4. Replace the `.qm` files in your Ultracopier release


# Plugins

Customizations in form of [Plugins](plugins/README.me) are also possible.


# Contributing
This project is hosted on [Github](https://github.com/alphaonex86/Ultracopier).
Add issues and merge requests there!

