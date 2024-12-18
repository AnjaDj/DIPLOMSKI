# 🔸 gRPC
## Build and locally install gRPC and Protocol Buffers for C++
Prerequisites<br>
 - cmake 3.16 or later (https://vitux.com/how-to-install-cmake-on-ubuntu/)
1. Configures a directory for locally installed packages and ensures the executables are easily accessible from the command line<br>
    ```bash
     ~$ export LOCAL_INSTALL_DIR=$HOME/.local
     ~$ mkdir -p $LOCAL_INSTALL_DIR
     ~$ export PATH="$LOCAL_INSTALL_DIR/bin:$PATH"
2. Install the basic tools required to build gRPC
     ```bash
     ~$ sudo apt install -y build-essential autoconf libtool pkg-config
3. Clone the grpc repo and its submodules
     ```bash
     ~$ git clone --recurse-submodules -b v1.66.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc
4. Build and locally install gRPC and Protocol Buffers
     ```bash
     ~$ cd grpc
     ~/grpc$ mkdir -p cmake/build
     ~/grpc$ pushd cmake/build
     ~grpc/cmake/build$ cmake -DgRPC_INSTALL=ON \
                              -DgRPC_BUILD_TESTS=OFF \
                              -DCMAKE_INSTALL_PREFIX=$LOCAL_INSTALL_DIR \
                              ../..
     ~grpc/cmake/build$ make -j 4
     ~grpc/cmake/build$ make install
     ~grpc/cmake/build$ popd
5. Build the project<br>
    in `~/grpc/examples/protos/` save .proto file<br>
    in `~/grpc/examples/cpp/` create new directory `myproject` containing source .cc code and CMakeLists.txt<br>
    ```bash
    ~/grpc$ cd examples/cpp/myproject
    ~/grpc/examples/cpp/myproject$ mkdir -p cmake/build
    ~/grpc/examples/cpp/myproject$ pushd cmake/build
    ~/grpc/examples/cpp/myproject/cmake/build$ cmake -DCMAKE_PREFIX_PATH=$LOCAL_INSTALL_DIR ../..
    ~/grpc/examples/cpp/myproject/cmake/build$ make -j 4
6. Run the project from the project `build` directory<br>
    ```bash
    ~/grpc/examples/cpp/myproject/cmake/build$ ./app

## Build and locally install gRPC and Protocol Buffers for Python
Prerequisites<br>
 - Python 3.7 or higher
 - pip version 9.0.1 or higher

1. Install gRPC
   ```bash
   ~$ python3 -m pip install grpcio
2. Install gRPC tools
   ```bash
   ~$ python3 -m pip install grpcio-tools
3. Build the project<br>
   create new directory `myproject` containing .proto file and source .py code<br>
5. Run the project<br>
   from project directory `myproject` in terminal run `python3 app.py`



# 🔸 From Python script to Linux service using <b>systemd</b>
A daemon (or service) is a background process that is designed to run autonomously,with little or not user intervention. Services will start automatically every time the system starts, which eliminates the need to start it manually. Scripts that collect data, represent servers or similar are ideal candidates to be configured as services and not ordinary scripts.<br>
  
1. Write Python script you want to make as service `/path/to/your_script.py`
2. Make your Python script executable
   ```bash
   ~$ chmod +x /path/to/your_script.py
4. Create systemd service file in directory `/etc/systemd/system/`.  Systemd service files need to be in `/etc/systemd/system/` DIR!
   ```bash
   ~$ sudo nano /etc/systemd/system/your_script.service
5. Add following content to your .service file
   ```bash
   [Unit]
   Description=Python Script Service
   After=network.target
   
   [Service]
   ExecStart=/usr/bin/python3 /path/to/your_script.py
   Restart=always
   User=your_user
   WorkingDirectory=/path/to/
   Environment="PATH=/usr/bin"
   
   [Install]
   WantedBy=multi-user.target

6. Reload systemd to recognise new service `your_script.service`
   ```bash
   sudo systemctl daemon-reload
8. Enable new service at system startup
   ```bash
   sudo systemctl enable your_script.service
10. Run service
    ```bash
    sudo systemctl start your_script.service
11. Managing the service
    - stop service
      ```bash
      sudo systemctl stop your_script.service
    - restart service
      ```bash
      sudo systemctl stop your_script.service
    - disable service at system startup
      ```bash
      sudo systemctl disable your_script.service


# 🔸 Cross-compilation
Cross-compilation is the process of building code for a target other than the one running the build process. If you are developing an application on standard x86_64 PC and want it to run on RPi with complitly different architecture than your PC, you need to cross-compile it. On your x86_64 Linux PC:
1. <b>Install toolchain for cross-compilation </b><br>
   The following commands will install a compiler for ARM architecture used in RPi
   ```bash
   sudo apt update
   sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf

2. <b> At the end of a .bashrc file, add following </b><br>
    ```bash
    export ARCH=arm
    export CROSS_COMPILE=arm-linux-gnueabihf

3. <b> Run `. ~/.bashrc` or log again in order to update $PATH
   
4. <b>Download Linux kernel source code </b><br>
   ```bash
   git clone --depth=1 https://github.com/raspberrypi/linux -b rpi-6.6.y
   cd linux

5. <b>Configure Linux kernel for cross-compiling</b><br>
   This will generate `.config` file for RPi3 kernel
   ```bash
   make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- bcm2709_defconfig
   
6. <b>Download booklet for RPi</b><br>
   ```bash
   rsync -avz pi@<RPi_IP>:/lib ./sysroot
   rsync -avz pi@<RPi_IP>:/usr ./sysroot
   
7. <b>Configure sysroot</b><br>
   Creating sysroot on your PC
   ```bash
   export SYSROOT=$(pwd)/sysroot
   export CC="arm-linux-gnueabihf-gcc --sysroot=$SYSROOT"
   export CXX="arm-linux-gnueabihf-g++ --sysroot=$SYSROOT"
   
8. <b>Create CMake</b><br>
   If you are using CMake add following in CMakeLists.txt or in command line
   ```bash
   cmake -DCMAKE_SYSTEM_NAME=Linux \
         -DCMAKE_SYSTEM_PROCESSOR=arm \
         -DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc \
         -DCMAKE_CXX_COMPILER=arm-linux-gnueabihf-g++ \
         -DCMAKE_SYSROOT=$SYSROOT \
         -DCMAKE_FIND_ROOT_PATH=$SYSROOT \
         -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
         -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
         -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
         -B build -S .
   
9. <b> Compile application</b><br>
   ```bash
   cmake --build build

10. <b> Transfer binaries to RPi via `scp`</b><br>
    ```bash
    scp build/executable_name pi@<RPi_IP>:/home/pi/

11. <b> Run app on RPi</b><br>
    ```bash
    ./executable_name

# 🔸 Natively build a Linux kernel on RPi 3B<br>
Complete quidance on : https://www.raspberrypi.com/documentation/computers/linux_kernel.html#install-directly-onto-the-sd-card.<br>
This guide assumes that your Raspberry Pi runs the latest version of Raspberry Pi OS
  `sudo apt install git`<br>
  `git clone --depth=1 https://github.com/raspberrypi/linux`

1. Install the build dependencies <br>
   `sudo apt install bc bison flex libssl-dev make`
2. Build configuration<br>
   In this case, Im using RPi3B and for 32bit distribution run following commands
   ```bash
   cd linux
   KERNEL=kernel7
   make bcm2709_defconfig
3. Build the 32-bit kernel<br>
   `make -j6 zImage modules dtbs`
4. Install the kernel modules onto the boot media<br>
   `sudo make -j6 modules_install`
5. Create a backup of your current kernel and install the fresh kernel image
   ```bash
   sudo cp /boot/firmware/$KERNEL.img /boot/firmware/$KERNEL-backup.img
   sudo cp arch/arm/boot/zImage /boot/firmware/$KERNEL.img
6. For kernels version 6.5 and above<br>
   `sudo cp arch/arm/boot/dts/broadcom/*.dtb /boot/firmware/`
7. Copy over the overlays and README
   ```bash
   sudo cp arch/arm/boot/dts/overlays/*.dtb* /boot/firmware/overlays/
   sudo cp arch/arm/boot/dts/overlays/README /boot/firmware/overlays/
8. Finally, run the following command to reboot your Raspberry Pi and run your freshly-compiled kernel `sudo reboot`
