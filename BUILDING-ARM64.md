# Building from source dxx-rebirth

## Install WSL and chroot
1. 	Install wsl and ubuntu (use wsl2)
2. 	`sudo apt update`
3.	`sudo apt install -y apt-transport-https ca-certificates curl software-properties-common`
4.	`curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -`
5.	`sudo add-apt-repository "deb [arch=$(dpkg --print-architecture)] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"`
6.	`sudo apt install docker-ce -y`
7.	`sudo docker run --rm --privileged multiarch/qemu-user-static --reset -p yes`
8.	`sudo qemu-debootstrap --arch arm64 bookworm /mnt/data/arm64 http://deb.debian.org/debian/`

Note: Bookworm uses gcc-12. For compatibility with older systems such as ArkOS, use bullseye and git clone the dxx-rebirth-compat repository.  

Note: The folder `/mnt/data/arm64` can be modified, for example to `/mnt/data/bookworm-arm64`. This is useful if you like to maintain multiple chroots.

## Enter chroot and install dependencies
1. 	`sudo chroot /mnt/data/arm64/`
2. 	`apt -y install build-essential git wget python3 python3-pip python3-setuptools python3-wheel scons libglu1-mesa-dev pkg-config libpng-dev libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libphysfs-dev`

## Install and build dxx-rebirth
1. 	`git clone https://github.com/dxx-rebirth/dxx-rebirth` // `git clone --branch compatibility https://github.com/JeodC/dxx-rebirth-compat` (Use the compat version for ArkOS etc)
2. 	`cd dxx-rebirth` // `cd dxx-rebirth-compat`  
3. 	`scons sdl2=1 sdlmixer=1 opengl=1`
