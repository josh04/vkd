<div id="top"></div>

[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]

<br />
<div align="center">
  <!--<a href="https://github.com/josh04/vkd">
    <img src="images/logo.png" alt="Logo" width="80" height="80">
  </a>-->

<h1 align="center"><a href="https://github.com/josh04/vkd">vkd</a></h3>

  <p align="center">
    a vulkan node-based image and video processing platform
    <br />
    <a href="https://github.com/josh04/vkd">Docs</a>
    ·
    <a href="https://github.com/josh04/vkd/releases">Downloads</a>
    ·
    <a href="https://github.com/josh04/vkd/issues">Bugs</a>
  </p>
</div>



<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li>
      <a href="#about-the-project">About *vkd*</a>
      <ul>
        <li><a href="#built-with">Built With</a></li>
      </ul>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
      </ul>
    </li>
    <li><a href="#usage">Usage</a></li>
    <li><a href="#roadmap">Roadmap</a></li>
    <li><a href="#contributing">Contributing</a>
      <ul>
        <li><a href="#built-with">Built With</a></li>
      </ul>
    </li>
    <li><a href="#license">License</a></li>
    <li><a href="#contact">Contact</a></li>
    <!--<li><a href="#acknowledgments">Acknowledgments</a></li>-->
  </ol>
</details>

## About *vkd*

[![vkd screenshot][product-screenshot]](screenshots/vkdcustom.png)

*vkd* is a node-based video and photo editing platform implemented in Vulkan. The nodes perform work chiefly through applying Vulkan compute shaders. My initial goals were to allow simple compositing graphs for video and accurate colour operations for inverting photo negatives.

## Getting Started

To get a local copy up and running follow these example steps. To use OCIO you will need to download an OCIO configuration - in development I have the `aces_1.2` folder in my `build/bundle/release/bin` directory.

### Prerequisites

vkd can build on Windows, MacOS or Linux. On Windows, the Visual Studio 2019 compiler is required.

Most dependencies are included in the repository and will build alongside the main software. Some dependencies are provided as binaries. A zip of these binaries will be made available TBC.

### Installation

1. Install `cmake` and `ninja`
2. Clone the repository
   ```sh
   git clone --recurse-submodules https://github.com/josh04/vkd.git
   ```
3. Navigate to the build directory
   ```sh
   cd vkd/cmake
   ```
4. Configure using `cmake`
   ```sh
   cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -GNinja -DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE .
   ```
5. Build using `ninja`
   ```sh
   ninja install
   ```

## Screenshots

[![vkd screenshot 2][product-screenshot2]][product-screenshot2]

[![vkd screenshot 3][product-screenshot3]][product-screenshot3]


## Contributing

Active development is currently performed on a private development branch. If you wish to contribute a feature, feel free to fork the current master and submit a pull request.

### Third Party

*vkd* depends on

* ACES
* enkiTS
* ffmpeg
* glm
* glslang
* Imath
* dear imgui
* ktx
* libpng
* LibRaw
* OCIO
* OpenEXR
* sago PlatformFolders
* SDL2
* spirv-reflect
* tinyxml2
* zlib
* cereal
* ghc::filesystem
* gl3w
* stb_image

## License

License TBD.

## Contact

Twitter: [@vkdapp](https://twitter.com/vkdapp)

Project Link: [https://github.com/josh04/vkd](https://github.com/josh04/vkd)

<p align="right">(<a href="#top">back to top</a>)</p>



<!-- ACKNOWLEDGMENTS
## Acknowledgments

* []()
* []()
* []()

<p align="right">(<a href="#top">back to top</a>)</p> -->



<!-- MARKDOWN LINKS & IMAGES -->
<!-- https://www.markdownguide.org/basic-syntax/#reference-style-links -->
[contributors-shield]: https://img.shields.io/github/contributors/josh04/vkd.svg?style=for-the-badge
[contributors-url]: https://github.com/josh04/vkd/contributors
[forks-shield]: https://img.shields.io/github/forks/josh04/vkd.svg?style=for-the-badge
[forks-url]: https://github.com/josh04/vkd/network/members
[stars-shield]: https://img.shields.io/github/stars/josh04/vkd.svg?style=for-the-badge
[stars-url]: https://github.com/josh04/vkd/stargazers
[issues-shield]: https://img.shields.io/github/issues/josh04/vkd.svg?style=for-the-badge
[issues-url]: https://github.com/josh04/vkd/issues
[product-screenshot]: screenshots/vkdcustom.png
[product-screenshot2]: screenshots/sc2.png
[product-screenshot3]: screenshots/sc1.png
