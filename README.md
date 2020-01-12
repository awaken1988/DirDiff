# DirDiff
[![Project Status: Abandoned – Initial development has started, but there has not yet been a stable, usable release; the project has been abandoned and the author(s) do not intend on continuing development.](https://www.repostatus.org/badges/latest/abandoned.svg)](https://www.repostatus.org/#abandoned)

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


## Bugs
- [ ] Duplicates are in some cases wrong
- [ ] Select Duplicates from Duplicate menu don't update the diff detail view
- [ ] All Filters work on the whole path string. This is a bit problematic e.g \\.git would also matches /home/user/mygit.git/data; Maybe Filters should look at every path item
- [ ] Slow load times

## TODO
- [ ] Add more default filters
- [ ] Integrate the external diff tool menu more properly
- [ ] Include more external diff tools
- [ ] Remove integrated Diff tools (for files)
- [ ] Make a summary view which shows all diff files/directories recusively in a list


