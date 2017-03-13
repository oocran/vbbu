Vagrant.configure("2") do |config|
  # boxes at https://atlas.hashicorp.com/search.
  # nixos/nixos-16.09-x86_64
  # centos/7

  config.vm.define "vm" do |v|
    #v.vm.box = "sandstorm/debian/jessie64"
    v.vm.box = "debian/jessie64"
    #v.vm.box = "https://atlas.hashicorp.com/ARTACK/boxes/debian-jessie"
    #v.vm.network :private_network, :ip => "192.168.121.40"
    #v.vm.network :private_network, :ip => "192.168.121.43"
  end

  config.vm.provider "virtualbox" do |v|
    v.memory = 1024
    v.cpus = 1
  end

  config.vm.provider "libvirt" do |v|
    v.random_hostname = true
    v.driver = 'kvm'
    v.memory = 1024
    v.cpus = 1
    v.keymap = 'es'
    v.kvm_hidden = true
  end

  #config.vm.synced_folder "/home/howls/workspace/sdr", "/vagrant"

  #Update and upgrade
  config.vm.provision "shell", inline: <<-SHELL
  #!/bin/bash
  # -*- ENCODING: UTF-8 -*-

  sudo apt-get update && sudo apt-get upgrade -y
  sudo apt-get install build-essential -y
  sudo apt-get install git -y

  #Mandatory dependencies:
  #libfftw
  sudo apt-get install libfftw3-dev libfftw3-doc -y   
  #UHD
  sudo apt-get install libboost-all-dev libusb-1.0-0-dev python-mako doxygen python-docutils cmake build-essential -y
  sudo apt-get install libuhd-dev libuhd003 uhd-host -y
  #Optional dependencies:
  #BOOST
  sudo apt-get install libboost-system-dev libboost-test-dev libboost-thread-dev libqwt-dev libqt4-dev -y
  #srsGUI
  git clone https://github.com/suttonpd/srsgui.git
  cd srsgui
  mkdir build
  cd build
  cmake ../
  sudo make install
  cd ../.. 
  #VOLK
  git clone https://github.com/gnuradio/volk.git
  cd volk
  mkdir build
  cd build
  cmake ../
  sudo make install  
  cd ../..
  #VBBU
  git clone https://github.com/oocran/vbbu.git
  cd vbbu
  mkdir build
  cd build
  cmake ../
  sudo make install 
  chmod +x *
  path=$(pwd)
  ln -s "$path"/* /usr/bin 
  cd ../..

  sudo echo "\nnet.core.rmem_max=50000000" >> /etc/sysctl.conf
  sudo echo "\nnet.core.wmem_max=1048576" >> /etc/sysctl.conf

  sudo echo "\nroot  -   rtprio    99" >> /etc/security/limits.conf
  sudo echo "\nroot    -   memlock   unlimited" >> /etc/security/limits.conf
  SHELL
end
