
<div align="right">
  <details>
    <summary >🌐 Language</summary>
    <div>
      <div align="center">
        <a href="https://openaitx.github.io/view.html?user=alphaonex86&project=Ultracopier&lang=en">English</a>
        | <a href="https://openaitx.github.io/view.html?user=alphaonex86&project=Ultracopier&lang=zh-CN">简体中文</a>
        | <a href="https://openaitx.github.io/view.html?user=alphaonex86&project=Ultracopier&lang=zh-TW">繁體中文</a>
        | <a href="https://openaitx.github.io/view.html?user=alphaonex86&project=Ultracopier&lang=ja">日本語</a>
        | <a href="https://openaitx.github.io/view.html?user=alphaonex86&project=Ultracopier&lang=ko">한국어</a>
        | <a href="https://openaitx.github.io/view.html?user=alphaonex86&project=Ultracopier&lang=hi">हिन्दी</a>
        | <a href="https://openaitx.github.io/view.html?user=alphaonex86&project=Ultracopier&lang=th">ไทย</a>
        | <a href="https://openaitx.github.io/view.html?user=alphaonex86&project=Ultracopier&lang=fr">Français</a>
        | <a href="https://openaitx.github.io/view.html?user=alphaonex86&project=Ultracopier&lang=de">Deutsch</a>
        | <a href="https://openaitx.github.io/view.html?user=alphaonex86&project=Ultracopier&lang=es">Español</a>
        | <a href="https://openaitx.github.io/view.html?user=alphaonex86&project=Ultracopier&lang=it">Italiano</a>
        | <a href="https://openaitx.github.io/view.html?user=alphaonex86&project=Ultracopier&lang=ru">Русский</a>
        | <a href="https://openaitx.github.io/view.html?user=alphaonex86&project=Ultracopier&lang=pt">Português</a>
        | <a href="https://openaitx.github.io/view.html?user=alphaonex86&project=Ultracopier&lang=nl">Nederlands</a>
        | <a href="https://openaitx.github.io/view.html?user=alphaonex86&project=Ultracopier&lang=pl">Polski</a>
        | <a href="https://openaitx.github.io/view.html?user=alphaonex86&project=Ultracopier&lang=ar">العربية</a>
        | <a href="https://openaitx.github.io/view.html?user=alphaonex86&project=Ultracopier&lang=fa">فارسی</a>
        | <a href="https://openaitx.github.io/view.html?user=alphaonex86&project=Ultracopier&lang=tr">Türkçe</a>
        | <a href="https://openaitx.github.io/view.html?user=alphaonex86&project=Ultracopier&lang=vi">Tiếng Việt</a>
        | <a href="https://openaitx.github.io/view.html?user=alphaonex86&project=Ultracopier&lang=id">Bahasa Indonesia</a>
      </div>
    </div>
  </details>
</div>

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
sudo apt install make gcc build-essential libssl-dev qtbase6-dev qtchooser qt6-qmake qtbase6-dev-tools qttools6-dev-tools
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

