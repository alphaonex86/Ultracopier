# About

[Ultracopier](https://ultracopier.herman-brule.com/) is free and open
source software licensed under GPLv3 that acts as a replacement for
files copy dialogs.

Main features include:
- task queue
- pause / resume
- resume unfinished jobs
- dynamic speed limitation
- collision management
- plugin support

# Dependency
- make
- gcc
- C++17 complier (due to Qt6.7)

For example on Debian based distros:

```bash
sudo apt install make gcc build-essential libssl-dev qt6-base-dev qtchooser qmake6 qt6-base-dev-tools qt6-tools-dev-tools
```

# Building

Building an all-in-one version is as easy as compiling the main Qt project:

```bash
find ./ -name '*.ts' -exec lrelease {} \;
qmake ultracopier.pro
make -j$(nproc)
```

# Run

```bash
./ultracopier
```

# Translations

Translations are provided via [Qt Linguist](http://doc.qt.io/qt-6/qtlinguist-index.html).

1. Run `lupdate ultracopier.pro` to update the translation files
2. Put your translation in `(plugins|resources)/Languages/XX/translation.ts`
3. Run `lrelease ultracopier.pro` to compile the files
4. Replace the `.qm` files in your Ultracopier release


# Plugins

Customizations in form of [Plugins](plugins/README.md) are also possible.


# Contributing
This project is hosted on [Github](https://github.com/alphaonex86/Ultracopier).
Add issues and merge requests there!

