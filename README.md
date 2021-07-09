# ponywall
Use Bing daily wallpaper with your favourite tile WM.

## Requirements
Libraries:
 - libcurl
 - json-c

Programs:
 - feh (default wallpaper setter)

## How does it work
First, the programs reads wallpaper's expiration date from the config file. If the date is in the past, or the config file is absent, or the -f key was specified, the program requests current metadata from Bing site. Second, it downloads the wallpaper and saves the wallpaper's title into a file and a new expiration date into the config. Third, it executes a program to set the downloaded image as a wallpaper. The default command is <code>feh --bg-fill path/to/pwall.jpg</code>, it can be changed with a command line parameter.   

## Arguments
**-a**  Turns on auto mode. The program will check every 15 minutes if the current wallpaper has expired and will download a new one when.

**-c <command>**  Use the given command to set the wallpaper. You can use **%s** to substitute full path to wallpaper file.

**-f** Forces the program to update the wallpaper skipping the expiration date check. Doesn't work in auto mode.
  
**-h**  Prints some brief help.

**-m <market>**	Sets the designation of the Bing market (e.g. de-DE, fr-FR, zh-CN, etc)

**-M**	Sets auto market selection based on the LANG variable.

**-s <screen size>**  The screen size format is WIDTHxHEIGHT, for example 1366x768. The program will try to download the current wallpaper from Bing using this size. Default and maximum value of 1920x1080 is used when this parameter is skipped.
  
**-t <seconds>**  Sets the check interval for auto mode.
  
## Files
  All files are stored in the $XDG_CONFIG_HOME/ponywall directory. These files are:
  - pwall.dat An URL used to download the wallpaper and the expiration date.
  - pwall.txt The wallpaper title from Bing metadata.
  - pwall.jpg The wallpaper itself.

## Examples
  Sets the current wallpaper with the size specified.
  
  <code>ponywall -f -s 1366x768</code>
  
  Runs in background checking for the new image every hour and sets the wallpaper with its title printed on it.
  
  <code>ponywall -c 'sh ~/.config/ponywall/annotated.sh' -a -t 3600 &</code>
  
### How to print title on the wallpaper
You need imagemagick for this to work. Source for annotared.sh mentioned above

```shell
homedir=$HOME/.config/ponywall
convert -background '#00000080' -fill white -font Libertinus-Serif-Regular -pointsize 24 label:@$homedir/pwall.txt miff:- | composite -gravity south -geometry +600+32 - $homedir/pwall.jpg $homedir/pwall_composite.jpg
feh --bg-fill $homedir/pwall_composite.jpg  
```
    
  
## Build
### Manual
* gcc -o ponywall ponywall.c -O3 -s -lcurl -ljson-c
* mv ponywall /usr/local/bin
* add ponywall to your autoload *(optional step)*

### Make
Use `PREFIX=` to install to the destination other than /usr/local/bin.

* make 
* sudo make install

### Arch Linux
* makepkg -i

