# Katelistings
Generates LaTeX code listings based on Katepart syntax definitions.

Katelistings uses the syntax highlighting system of the Katepart-based family of editors to produce code listings for LaTeX. This is less elegant than lstlistings and similar packages that live inside LaTeX, but has the following benefits:
 
 - The number of supported languages is much larger than for lstlistings, since it leverages the work of the KDE community;
 - The language for writing your own syntax definitions is much more powerful and flexible than that of lstlistings;
 - The same custom syntax works both in LaTeX and in all Katepart-based editors, so the listings have the same look and feel as the code you write;
 - Your LaTeX system won't have to do any heavy lifting in the syntax highlighting, which should speed up typesetting.
 
Note that this is my own hobby project, and not affiliated with Kate. The syntax highlighting engine itself is written from scratch by me, but it is designed to work with the extensive syntax definition files published with Kate. I hope that the effort I put into piggy-backing off of their work shows how much I appreciate it.

## Installation and use
To install, clone this repository wherever you need it, and after making sure that it is executable, run
```
$ scripts/install.sh
```
This will compile the program and download all themes and syntaxes provided by Kate. After that,
```
$ katelistings -h
```
will give you a summary of how to use it.

## Custom syntax files
Follow Kate's [guidelines](https://docs.kde.org/trunk5/en/applications/katepart/highlight.html) on how to write a syntax highlighting file, and place the result somewhere under `syntaxes/` in katelisting's home folder. Finally, run
```
$ katelistings -m
```
to make katelistings recognise the new language.
