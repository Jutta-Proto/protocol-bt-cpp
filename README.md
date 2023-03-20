# JURA Bluetooth Protocol
`C++` JURA Bluetooth protocol implementation for controlling a JURA coffee maker over a Bluetooth connection.

For a device to be able to connect to an JURA coffee maker via Bluetooth, usually a [Smart Control](https://uk.jura.com/en/homeproducts/accessories/SmartConnect-Main-72167) dongle is required.

![Smart Control dongle](https://user-images.githubusercontent.com/11741404/135412711-23fd1946-77bc-45db-8795-f30da4421578.png)

Most of this was done by [Reverse Engineering](#reverse-engineering) the Android APK.

## Table of Contents
1. [Protocol](#protocol)
2. [Bluetooth Characteristics](#bluetooth-characteristics)
3. [Brewing Coffee](#brewing-coffee)
4. [Building](#building)
5. [Reverse Engineering](#reverse-engineering)
6. [License and Copyright Notice](#license-and-copyright-notice)

## Protocol

### General
There are several steps of obfuscation being done by the JURA coffee maker to prevent others from reading the bare protocol or sending arbitrary commands to it.

#### Connecting to an JURA coffee maker
To connect to a JURA coffee maker via Bluetooth, a [Smart Control](https://uk.jura.com/en/homeproducts/accessories/SmartConnect-Main-72167) dongle is required.
This dongle has to be plugged into the coffee maker.
Once this has been done, we can connect to the `TT214H BlueFrog` device via Bluetooth.

#### Obtaining a key
Once connected, we have to obtain the key used for decoding and encoding the data to be sent.
This is done by analyzing the `advertisement data` or, more concretely, the `manufacturer data` found when scanning for devices.
Here the `manufacturer data` is structured as follows:
```
 0               1               2               3               
 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      key      |    bfMajVer   |    bfMinVer   |    unused     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|        articleNumber          |        machineNumber          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|        serialNumber           |       machineProdDate         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     machineProdDateUCHI       |    unused     |  statusBits   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Optional extended data starting at byte 28:
 0               1               2               3               
 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                             bfVerStr                          +
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                                                               +
|                                                               |
+                        coffeeMachineVerStr                    +
|                                                               |
+                                                               +
|                                                               |
+               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|               |                lastConnectedTabledID          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|               |
+-+-+-+-+-+-+-+-+
```

* `key`: 8 Bit. Starts at byte 0. The key is used for decoding and encoding data.
* `bfMajVer`: 8 Bit. Starts at byte 1. BlueFrog major version number.
* `bfMinVer`: 8 Bit. Starts at byte 2. BlueFrog minor version number.
* `articleNumber`: 16 Bit. Starts at byte 4. Article number in little-endian.
* `machineNumber`: 16 Bit. Starts at byte 6. Machine number in little-endian.
* `serialNumber`: 16 Bit. Starts at byte 8. Serial number in little-endian.
* `machineProdDate`: 16 Bit. Starts at byte 10. Machine production date in a special format.
* `machineProdDateUCHI`: 16 Bit. Starts at byte 12. **Probably** the steam plate production date in a special format.
* `statusBits`: 8 Bit. Starts at byte 15. Some initial status bits: 4 - supports incasso, 6 - supports master pin, 7 - supports reset
* `bfVerStr`: 8 Byte. Starts at byte 27 BlueFrog ASCII version string (optional).
* `coffeeMachineVerStr`: 17 Byte. Starts at byte 35. Coffee maker ASCII version string (optional).
* `lastConnectedTabledID`: 32 Bit. Starts at byte 51. An int representing the last connection ID.

#### Parsing dates
The following is used to parse the `machineProdDate` and `machineProdDateUCHI` dates.
```c++
void to_ymd(const std::vector<uint8_t>& data, size_t offset) {
    uint16_t date = to_uint16_t_little_endian(data, offset); // Convert two bytes (little-endian) to an unsigned short
    uint16_t year = ((date & 0xFE00) >> 9) + 1990;
    uint16_t month = (date & 0x1E0) >> 5;
    uint16_t day = date & 1F;
}
```

#### Decoding data
Once we have obtained the `key` described above, we can start decoding read data from the Bluetooth characteristics.
Have a look at [this](https://github.com/Jutta-Proto/protocol-bt-cpp/blob/0adb1ea802df13aac03262033706755f431f93b6/src/bt/ByteEncDecoder.cpp#L33-L47) implementation of the `encDecBytes`, which takes our obtained key and some data to decode or encode.
Once decoded successfully, the first byte of the resulting data has to be the `key`, otherwise the decoding failed.

#### Encoding data
When encoding data that should be written to Bluetooth characteristics, we again need the `key` we have obtained described above.
First we have to make sure we set the first byte of your data to the `key` and then feed it to [`encDecBytes`](https://github.com/Jutta-Proto/protocol-bt-cpp/blob/0adb1ea802df13aac03262033706755f431f93b6/src/bt/ByteEncDecoder.cpp#L33-L47), as we have done for decoding.

### Heartbeat
The coffee maker stays initially connected for 20 seconds. After that, it disconnects.
To prevent this, we have to send at least every 10 seconds a heartbeat to it.
The heartbeat is `0x007F80` encoded and then sent to the `P Mode` Characteristic `5a401529-ab2e-2548-c435-08c300000710`.
For example, if the key is `0x2A`, the encoded data sent should be `0x77656d` (without the `0x` ;) ).
Keep in mind, we have to set byte zero of our data that should be encoded to the key: `0x007F80` -> `0x2A7F80` -> `0x77656d`

## Bluetooth Characteristics

## Overview
Here is an overview of all the known characteristics and services exposed by the coffee maker and some additional information in case we have found out how to use them.
 
| Name | Services | Notes |
| --- | --- | --- |
| Default | `5a401523-ab2e-2548-c435-08c300000710` | Default service containing all relevant characteristics. |
| UART | `5a401623-ab2e-2548-c435-08c300000710` | Contains a TX and RX UART characteristic. |

| Name | Characteristic | Encoded |
| --- | --- | --- |
| About Machine | `5A401531-AB2E-2548-C435-08C300000710` | `false` |
| Machine Status | `5a401524-ab2e-2548-c435-08c300000710` | `true` |
| Barista Mode | `5a401530-ab2e-2548-c435-08c300000710` | `true` |
| Product Progress | `5a401527-ab2e-2548-c435-08c300000710` | `true` |
| P Mode | `5a401529-ab2e-2548-c435-08c300000710` | `true` |
| P Mode Read | `5a401538-ab2e-2548-c435-08c300000710` | `UNKNOWN` |
| Start Product | `5a401525-ab2e-2548-c435-08c300000710` | `true` |
| Statistics Command | `5A401533-ab2e-2548-c435-08c300000710` | `true` |
| Statistics Data | `5A401534-ab2e-2548-c435-08c300000710` | `UNKNOWN` |
| Update Product Statistics | `5a401528-ab2e-2548-c435-08c300000710` | `UNKNOWN` |
| UART TX | `5a401624-ab2e-2548-c435-08c300000710` | `true` |
| UART RX | `5a401625-ab2e-2548-c435-08c300000710` | `true` |

### About Machine
* `5A401531-AB2E-2548-C435-08C300000710`
* Encoded: `false`

This characteristic can only be read and provides general information about the coffee maker, like the `bfVerStr`` (8 byte, starts at byte 27) and the `coffeeMachineVerStr` (17 byte, starts at byte 35).

### Machine Status
* `5a401524-ab2e-2548-c435-08c300000710`
* Encoded: `true`

When reading from this characteristic, the received data has to be decoded. Once decoded, the first byte has to be the `key` used for decoding. Otherwise, something went wrong.
Starting from byte 1, the data represents status bits for the coffee maker.
For example, bit 0 is set in case the water tray is missing and bit 1 of the first byte in case there is not enough water.
For an exact mapping of bits to their action, we need the machine files found, for example, inside the Android app.
More about this here: [Reverse Engineering](#reverse-engineering)

### P Mode
* `5a401529-ab2e-2548-c435-08c300000710`
* Encoded: `true`
To begin with: I don't know what the "P" stands for.
Used for sending the heartbeat to the coffee maker to prevent it from disconnecting.

### Start Product
* `5a401525-ab2e-2548-c435-08c300000710`
* Encoded: `true`
Used to start preparing products.
How to brew coffee can be found here: [Brewing Coffee](#brewing-coffee)

### Barista Mode
* `5a401530-ab2e-2548-c435-08c300000710`
* Encoded: `true`

#### Writing
Used for locking and unlocking the coffee maker screen and all its buttons.
This could be used in a way where users have to authenticate first via some external service (e.g., an RFID or NFC card).
The coffee maker would always be locked and the only way to create a cup of coffee was by sending commands via Bluetooth.

**Locking**

Write `0x0001` to this characteristic to lock the coffee maker.
Encode this message like all other messages, but **do not** override the first byte at the end with the key.

Here is an example:

`0x0001` gets encoded to `0x77E0` using the key `0x2A` and then sent to the characteristic.

**Unlocking**

Write `0x0000` to this characteristic to lock the coffee maker.
Encode this message like all other messages, but **do not** override the first byte at the end with the key.

Here is an example:

`0x0000` gets encoded to `0x77E1` using the key `0x2A` and then sent to the characteristic.

### UART TX
* `5a401624-ab2e-2548-c435-08c300000710`
* Encoded: `UNKNOWN`

Probably exposes a raw TX interface for interacting directly with the coffee maker.

### UART RX
* `5a401625-ab2e-2548-c435-08c300000710`
* Encoded: `UNKNOWN`

Probably exposes a raw RX interface for interacting directly with the coffee maker.

### Statistics Command
* `5A401533-ab2e-2548-c435-08c300000710`
* Encoded: `true`

#### Writing
Allows requesting statistics like product counts and maintenance data from the coffee maker.
A command sent to this characteristic is built as follows and consists of the following 5 bytes (hexadecimal):
```
00 0001 FFFF
```

* `0x00` The first by has to be set to 0.
* `0x0001` To request the overall product statistics. To get the daily counter use `0x0010`.
* `0xFFFF` Defines the products we want to retrieve statistics for. All bits set to one forces data for all products. Selecting only specific products is done as follows:

```c++
void get_prod_stat_bits() const {
    std::array<uint8_t, 2> bArr{0};

    for (const Product& p : joe->products) {
        size_t code = p.code_to_size_t();

        code /= 4;
        size_t arrOffset = code / 8;
        assert(arrOffset < bArr.size());
        bArr[arrOffset] = (1 << (code % 8)) | (bArr[arrOffset] & 0xFF);
    }

    // The resulting bytes are now inside bArr.
}
```

#### Reading
Once data has been written to the characteristic, we can read from it after a delay of 1200 ms.
The command indicates success when the value read does not start with `0x0E`.
For example:
```
0x0EA2A2A2 -> We wrote an invalid command to the characteristic.
0xA200A2A2 -> Success, we now can read all statistics from the "Statistics Data" characteristic.
0x4200A2A2 -> Success, we now can read all statistics from the "Statistics Data" characteristic.
```

### Statistics Data
* `5A401534-ab2e-2548-c435-08c300000710`
* Encoded: `true`

This characteristic contains the statistics requested by sending a request to the `Statistics Command` characteristic.
Such a response could look as follows (hex):
```
00014E00000000002700009800000A00FFFF00000300FFFF00000900FFFF00FFFF00FFFF00FFFF00006700FFFF00FFFF00FFFF00000000000200000000FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00FFFF00000000000000000000000000FFFF00FFFF00FFFF00FFFF00000000000000000000000000FFFF00FFFF00FFFF00FFFF00000000000000000000000000FFFF00FFFF00FFFF00FFFF0000000000000000000000000000000000000000000000000000000000000000
```

It is split into multiple parts. Each part consists of six hex chars (3 bytes).
```
00014E 000000 000027 000098 00000A 00FFFF 000003 00FFFF 000009 00FFFF...
```
Each block describes the counter for a different product, except the first block.
The first block (`0x00014E` in this case) represents the total product count.
In this example, the coffee maker has produced `0x14E` (or 334 in decimal) products.

The offset of a product statistic is calculated by the product code found inside the machine file for the coffee maker.
For example, inside the `EF532V2.xml` file, we find the following information: `<PRODUCT Code="03" Name="Coffee" ...`
Code `0x03` indicates we find the product count for "Coffee" at index `0x03`.
The fourth block (counting from zero) is `0x000098`, translated to decimal, means the coffee maker has already produced 152 cups of regular "Coffee".

## Brewing Coffee
A command to brew a coffee consists of multiple parts.
Those parts depend on the machine file for the coffee maker.
The following example uses the `EF532V2.xml` file for an JURA E6 coffee maker.
More about this here: [Reverse Engineering](#reverse-engineering)

For example, we have a look at the following command (decoded) sent to the coffee maker:
```
00 03 00 04 14 0000 01 00010000000000 2A
0  1  2  3  4  5    6  7              8
```

* `0`: Will be replaced later by the `key`, when encoding.
* `1`: Product type e.g. `03` for coffee, or `04` for a cappuccino.
* `2`: Unknown
* `3`: Strength when brewing a coffee. Value between `01` and `08` with `04` being the default.
* `4`: Amount of water in seconds. 1 second equals 5 ml water.
* `5`: Unknown
* `6`: Temperature. `01` Normal, `02` High
* `7`: Unknown
* `8`: Probably some kind of checksum or the same value as the `key` at part `0`.

## Building

### Requirements
The following requirements are required to build this project.
* A C++20 compatible compiler like [gcc](https://gcc.gnu.org/) or [clang](https://clang.llvm.org/)
* The build system is written using [CMake](https://cmake.org/)
* For managing dependencies in CMake, we are using [conan](https://conan.io/)

### Machine Files
Since this lib uses the machine files provided by JURA in their Android APK, we have to extract them first.
For this, you have to perform the following steps:

1. Visit the [Google Play](https://play.google.com/store/apps/details?id=ch.toptronic.joe) page for the `J.O.E.® – Jura Operating Experience`.
2. Copy the page URL.
3. Use your favorite APK downloader to obtain the latest `J.O.E.® – Jura Operating Experience` APK. For example, one could use this page: https://apps.evozi.com/apk-downloader/?id=ch.toptronic.joe
4. Place it under `src/resources`
5. Open a terminal under `src/resources`
6. Execute the `extract_apk.sh` bash script to extract all required files. Example: `./extract_apk.sh myPathToTheJuraJoeApk.apk`
7. Done

#### Fedora
To install those dependencies on Fedora, run the following commands:
```bash
sudo dnf install -y gcc clang cmake python3 python3-pip
pip3 install --user conan==1.59.0 # conan 2.x.x is not supported right now
```

#### Raspberry Pi
To install those dependencies on a Raspberry Pi, running the [Raspberry Pi OS](https://www.raspberrypi.org/software/), run the following commands:
```bash
sudo apt install -y cmake python3 python3-pip
pip3 install --user conan==1.59.0 # conan 2.x.x is not supported right now
```
For all the other requirements, head over here: https://github.com/Jutta-Proto/hardware-pi#raspberry-pi-os

Run the following commands to build this project:
```bash
# Clone the repository:
git clone https://github.com/Jutta-Proto/protocol-bt-cpp.git
# Switch into the newly cloned repository:
cd protocol-bt-cpp
# Build the project:
mkdir build
cd build
cmake ..
cmake --build .
```

## Reverse Engineering
Most of the information found here has been discovered by reverse engineering the Android APK and spoofing the traffic between the app and dongle.

To reverse engineer the app, follow the following steps:
* Download the [`JURA J.O.E.®`](https://play.google.com/store/apps/details?id=ch.toptronic.joe&hl=de&gl=US) app from the Google Play Store. For this, you can use, for example, the [APK DOWNLOADER from  Hamidreza Moradi](https://github.com/HamidrezaMoradi/APK-Downloader).
* Once you have the APK, you can use [JADX](https://github.com/skylot/jadx) too. Most of the code is written in Java, but there are parts written in Kotlin, which will not be able to be decompiled from Java byte code.
* In JADEX have a look at the `resources/assets/machinefiles` directory, where you can find all machine files describing individual coffee makers and their available functions.

## License and Copyright Notice
This piece of software uses the following other libraries and dependencies:

### Catch2 (2.13.8)
Catch2 is mainly a unit testing framework for C++, but it also provides basic micro-benchmarking features and simple BDD macros.  
Source: https://github.com/catchorg/Catch2

<details>
  <summary>License</summary>

```
Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
```
</details>

### spdlog (1.10.0)
Very fast, header-only/compiled, C++ logging library.  
Source: https://github.com/gabime/spdlog

<details>
  <summary>License</summary>

```
The MIT License (MIT)

Copyright (c) 2016 Gabi Melman.                                       

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

-- NOTE: Third party dependency used by this software --
This software depends on the fmt lib (MIT License),
and users must comply to its license: https://github.com/fmtlib/fmt/blob/master/LICENSE.rst
```
</details>

### Fast C++ CSV Parser
Small, easy-to-use and fast header-only library for reading comma separated value (CSV) files.  
Source: https://github.com/ben-strasser/fast-cpp-csv-parser

<details>
  <summary>License</summary>

```
Copyright (c) 2015, ben-strasser
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

* Neither the name of fast-cpp-csv-parser nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```
</details>

### TinyXML-2 (9.0.0)
TinyXML-2 is a simple, small, efficient, C++ XML parser that can be easily integrated into other programs.  
Source: https://github.com/leethomason/tinyxml2

<details>
  <summary>License</summary>

```
This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
```
</details>

### Date (3.0.1)
A date and time library based on the C++11/14/17 <chrono> header.  
Source: https://github.com/HowardHinnant/date

<details>
  <summary>License</summary>

```
The source code in this project is released using the MIT License. There is no
global license for the project because each file is licensed individually with
different author names and/or dates.

If you contribute to this project, please add your name to the license of each
file you modify.  If you have already contributed to this project and forgot to
add your name to the license, please feel free to submit a new P/R to add your
name to the license in each file you modified.

For convenience, here is a copy of the MIT license found in each file except
without author names or dates:

The MIT License (MIT)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
</details>

### eventpp (master cf1ba5689d51d9aafeedfc28788f08690d8b0f40)
A event library for callbacks, event dispatcher, and event queue.
Source: https://github.com/wqking/eventpp

<details>
  <summary>License</summary>

```
eventpp library

Copyright (C) 2018 Wang Qi (wqking)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```
</details>

### gattlib (master bb90b55ddec9c1817729a507ca096c2a514d8dc3)
A library used to access Generic Attribute Profile (GATT) protocol of BLE (Bluetooth Low Energy) devices.
Source: https://github.com/labapart/gattlib
