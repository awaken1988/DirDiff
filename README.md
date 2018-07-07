# DirDiff
This tool compares directories and show the result side by side.

![Screenshot](screenshot_2018_07_07.png )

## How to use it
Giving 2 directories DirDiff will recursively look for added and deleted files or folders.
At this step the content of the files is not compared. If you click "compare files" an checksum 
is calculated over all files (can be time-cosuming). After that also file differences are shown.
There are also regex filter to include or exclude specific files.

## Features
* Find duplicate files (after click "compare files")
* Integrated Diff Gui (deactivated; see TODO)

## TODO
* HexEditor-Widget to show binary files (from a external Project)
* Progressbar when loading files have a strange behaviour (updating files on dialog only x seconds long; instead every x seconds)
* Hash Progressbar seems to be broken
* Integrated Diff Gui: Diff only text files otherwise applications hangs on large binaries or crash