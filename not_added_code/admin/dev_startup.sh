#!/bin/bash

clear
sudo apt update
sudo apt upgrade -y
rm -rf /tmp/* -v
sudo apt autoremove -y --purge
sudo du -sh /var/cache/apt
sudo apt autoclean -y
sudo apt clean -y
rm -rf ~/.cache/thumbnails/*
rm -rf /home/root/.cache/thumbnails/*
find . -name ".DS_Store" -delete


clear
echo -e "👋 Welcome to the Pythia Development Environment!\n \n ✍ Write code in the integrated VSCode area, or use a terminal text editor like Nano, or VIM.\n \n 💤 When you're ready to push some commits to GitHub, just type ghs (git add . -v && git commit -m [commit message] -S && git push). \n \n Time and Date: $date" 
exit 0
