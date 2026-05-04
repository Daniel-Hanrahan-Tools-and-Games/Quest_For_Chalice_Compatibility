# Quest_For_Chalice_Compatibility
Compatibility version of Quest_For_Chalice



<p>Controls: z for jump and arrow keys for directions.</p>

<h3>Prerequisites</h3>
<ol>
  <li>type this in terminal: sudo apt update</li>
  <li>type this in terminal: sudo apt install build-essential git cmake libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1mesa-dev libxcursor-dev libxinerama-dev libwayland-dev libxkbcommon-dev</li>
  <li>Download raylib repo here</li>
  <li>unzip zip file</li>
  <li>type this in terminal: mkdir build && cd build</li>
  <li>type this in terminal: cmake -DBUILD_EXAMPLES=OFF ..</li>
  <li>type this in terminal: make</li>
  <li>type this in terminal: sudo make install</li>
  <li>type this in terminal: export LUA_INC=/path/to/game directory/lua542Linux/include</li>
  <li>type this in terminal: type this in terminal: export LUA_LIB=/path/to/game directory/lua542Linux/lib</li>
</ol>

<h3>How to compile the game with gcc</h3>
<ol>
  <li>Type this in your terminal: cd /"Path To Game Folder"/Quest_For_Chalice_Compatibility</li>
  <li>Type this in terminal: gcc -O2 -Wall -DUSE_LUA -Ilua542Linux/include Quest_For_Chalice_Compatibility.c -o Quest_For_Chalice_Compatibility -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -Llua542Linux -llua54 -Wl,-rpath='$ORIGIN/lua542Linux'
</li>
  <li>Executable is now in game directory</li>
</ol>

<h3>Mod Support Info</h3>
<ul>
  <li>To load a mod file, the mod file needs to be in the game directory and needs to be called Quest_For_Chalice_Compatibility_Mod.lua in Quest_For_Chalice_Compatibility_Mod folder</li>
  <li>The mod libraries are platform specific so if you are not running linux you need to go here: https://sourceforge.net/projects/luabinaries/files/5.4.2/ and download the binaries for your platform and if the site asks you static or dynamic libraries, go to dynamic libraries, download, extract,and put all extracted files into a folder called lua542-Whatever platform you are using and change the directories for the libraries in the c++ file to lua542-Whatever platform you are using.</li>
</ul>

<a href="https://github.com/Daniel-Hanrahan-Tools-and-Games/Quest_For_Chalice_Compatibility_Mod">Example Mod Repository Page</a>

<a href="https://github.com/Daniel-Hanrahan-Tools-and-Games/Quest_For_Chalice">Original Version Repository Page</a>

<a href="https://daniel-hanrahan-tools-and-games.github.io/Quest_For_Chalice/">Original Version ReadMe Page</a>

<a href="https://github.com/Daniel-Hanrahan-Tools-and-Games/raylib">This game was made using Raylib 6</a>




CC BY-SA 4.0 and GNU GPL v3.0 Conditional Exceptions to use MPL 2.0 and CC BY-SA 4.0 or CC BY 4.0

If the following condition is met, the licensing rules for both content covered by GNU GPL v3.0 and content not covered by GNU GPL v3.0 are modified as described below:

Condition:

The developer is distributing, porting, or integrating the software with platforms or environments that impose requirements incompatible with GPL-3.0, including but not limited to:
- proprietary or non-redistributable SDKs
- confidential hardware or platform documentation
- legally required confidentiality obligations preventing full GPL redistribution
- safety-regulated or certified systems where full GPL redistribution cannot be satisfied

Effect on licensing:

- Content covered by GNU GPL v3.0: May instead be used under the Mozilla Public License 2.0.

- Content not covered by GNU GPL v3.0 (e.g., assets): Normally may be used under CC BY-SA 4.0. If ShareAlike requirements of CC BY-SA 4.0 prevent lawful distribution under the MPL alternative, developers may instead use CC BY 4.0 **solely to the extent necessary** to enable such distribution.

These exceptions apply **only when the condition above is met**.




CC BY-SA 4.0 and GNU GPL v3.0 Conditional Exceptions to use BSD-3-Clause and CC BY 4.0:

If **either** of the following conditions is met, the licensing
rules for both content covered by GNU GPL v3.0 and content not
covered by GNU GPL v3.0 are modified as described below:

Conditions:

1. The developer has made a licensing agreement with another entity
   (excluding corporate/LLC or equivalent entities) that prohibits the
   redistribution of content under copyleft licenses.

2. The developer adds assets that they do not own and uses them
   under fair use, or equivalent/similar legal arrangements, where
   the developer cannot legally contribute those assets under a
   copyleft license.

Effect on licensing:

- Content covered by GNU GPL v3.0: May instead be used under the
  BSD 3-Clause License.

- Content not covered by GNU GPL v3.0: Assets originally under CC BY-SA 4.0
  may instead be used under CC BY 4.0.

These exceptions apply **only when at least one of the conditions above is met**.





CC BY-SA 4.0 and GNU GPL v3.0 Conditional Exceptions to use PolyForm Noncommercial and CC BY-NC 4.0

The PolyForm Noncommercial License (and Creative Commons
Attribution-NonCommercial 4.0 International for non-code
content) may be used as an alternative only when the combined
work is subject to binding legal, contractual, or platform-
imposed restrictions that prohibit commercial use.

Such restrictions may arise from third-party licenses,
distribution platforms, or other enforceable legal terms that
make commercial use of the combined work not legally permitted.

Content covered by the primary license (e.g., source code or
other covered material) remains governed by that license.

Content not covered by the primary license (e.g., assets,
documentation, or other non-code materials) is governed by
CC BY-NC 4.0, unless otherwise stated.

This alternative applies only to the extent necessary to
comply with such restrictions.




CC BY-SA 4.0 and GNU GPL v3.0 Conditional Exceptions to use PolyForm Strict and CC BY-NC-ND 4.0

The PolyForm Strict License may be used as an alternative
license only when the combined work is subject to binding
legal, contractual, or platform-imposed restrictions that
require both non-commercial use and prohibit the creation of
derivative works as part of the distribution terms.

Such restrictions may arise from third-party licenses,
distribution platforms, or other enforceable legal terms that
impose both non-commercial and no-derivatives requirements on
the combined work.

Content covered by the primary license (e.g., source code or
other covered material) remains governed by that license.

Content not covered by the primary license (e.g., assets,
documentation, or other non-code materials) is governed by
Creative Commons Attribution-NonCommercial-NoDerivatives
4.0 International (CC BY-NC-ND 4.0), unless otherwise stated.

This alternative applies only to the extent necessary to
comply with such restrictions.
