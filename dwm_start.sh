#!/bin/bash
#Needed by Java apps to work correctly
touch .restartdwm
wmname LG3D 
#Autostart apps 
(compton -CGb) &
(sleep 1 && stjerm) &
#This is a loop for autostarting dwm after exit...
#If you want to exit dwm for good, kill xserver using ctrl+alt+backspace
while [ -e ~/.restartdwm ] 
do
    habak -hi ~/Pictures/Wallpapers     #Set wallpaper
    killall dwmstatus                   #Kill old dwmstatus
    sleep 0.1                           #Wait old dwmstatus to exit
    dwmstatus &                         #Start new dwmstatus
    dwm                                 #Start Dynamic Windows Manager
done
