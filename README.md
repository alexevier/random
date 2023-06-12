## Random
random is a small unix tool to get a random file or directory.

## Usage
the usage is very simple
```
// this will print out a random file in the current directory
random

// this prints out a file/dir from a specified directory
random ~/pictures/wallpapers

// with the -r flag it can be recursive
random -r /usr/include

// and with the -f flag it will only consider files
random -f ~/video

// print the help page with
random -h
```

## Building
only make, git and gcc are required.
```
git clone https://gitlab.com/alexevier/random.git
cd random
make
```
and a "random" executable will be in the directory if the compilation was successfull
